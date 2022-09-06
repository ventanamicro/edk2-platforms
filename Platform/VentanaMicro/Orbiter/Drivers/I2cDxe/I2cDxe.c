/** @file

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "I2cDxe.h"

//
// We cannot use Stall () or timer events at runtime, so we need to busy-wait
// for the controller to signal the completion interrupts. This value was
// arbitrarily chosen, and does not appear to produce any premature timeouts
// nor does it result in noticeable stalls in case of bus errors.
//
#define WAIT_FOR_INTERRUPT_TIMEOUT    50000
#define WAIT_FOR_INTERRUPT_DELAY      10

/**
  Set the frequency for the I2C clock line.

  This routine must be called at or below TPL_NOTIFY.

  The software and controller do a best case effort of using the specified
  frequency for the I2C bus.  If the frequency does not match exactly then
  the I2C master protocol selects the next lower frequency to avoid
  exceeding the operating conditions for any of the I2C devices on the bus.
  For example if 400 KHz was specified and the controller's divide network
  only supports 402 KHz or 398 KHz then the I2C master protocol selects 398
  KHz.  If there are not lower frequencies available, then return
  EFI_UNSUPPORTED.

  @param[in] This           Pointer to an EFI_I2C_MASTER_PROTOCOL structure
  @param[in] BusClockHertz  Pointer to the requested I2C bus clock frequency
                            in Hertz.  Upon return this value contains the
                            actual frequency in use by the I2C controller.

  @retval EFI_SUCCESS           The bus frequency was set successfully.
  @retval EFI_ALREADY_STARTED   The controller is busy with another transaction.
  @retval EFI_INVALID_PARAMETER BusClockHertz is NULL
  @retval EFI_UNSUPPORTED       The controller does not support this frequency.

**/
STATIC
EFI_STATUS
EFIAPI
I2cSetBusFrequency (
  IN CONST EFI_I2C_MASTER_PROTOCOL   *This,
  IN OUT UINTN                       *BusClockHertz
  )
{
  I2C_MASTER              *I2c;
  UINT32                  Val32;
  UINT64                  Temp;
  UINT64                  Fscl, ActualFscl, BestFscl;
  UINTN                   DivA, DivB, CalcDivA, CalcDivB;
  UINT32                  LastError, CurError;

  I2c = I2C_FROM_THIS (This);

  if (BusClockHertz == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Fscl = *BusClockHertz;
  /* calculate (divisor_a+1) x (divisor_b+1) */
  Temp = REFCLK_RATE / (22 * Fscl);

  /*
   * If the calculated value is negative or 0, the fscl input is out of
   * range. Return error.
   */
  if (!Temp || (Temp > (CDNS_I2C_DIVA_MAX * CDNS_I2C_DIVB_MAX))) {
    return EFI_INVALID_PARAMETER;
  }

  LastError = MAX_UINT32;
  for (DivA = 0; DivA < CDNS_I2C_DIVA_MAX; DivA++) {
    DivB = DIV_ROUND_UP(REFCLK_RATE, 22 * Fscl * (DivA + 1));

    if ((DivB < 1) || (DivB > CDNS_I2C_DIVB_MAX)) {
      continue;
    }
    DivB--;

    ActualFscl = REFCLK_RATE / (22 * (DivA + 1) * (DivB + 1));

    if (ActualFscl > Fscl) {
      continue;
    }

    CurError = ((ActualFscl > Fscl) ? (ActualFscl - Fscl) :
        (Fscl - ActualFscl));

    if (LastError > CurError) {
      CalcDivA = DivA;
      CalcDivB = DivB;
      BestFscl = ActualFscl;
      LastError = CurError;
    }
  }

  Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET);
  Val32 &= ~(CDNS_I2C_CR_DIVA_MASK | CDNS_I2C_CR_DIVB_MASK);
  Val32 |= ((CalcDivA << CDNS_I2C_CR_DIVA_SHIFT) |
            (CalcDivB << CDNS_I2C_CR_DIVB_SHIFT));
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);

  return EFI_SUCCESS;
}

/**
  Reset the I2C controller and configure it for use

  This routine must be called at or below TPL_NOTIFY.

  The I2C controller is reset.  The caller must call SetBusFrequence() after
  calling Reset().

  @param[in]     This       Pointer to an EFI_I2C_MASTER_PROTOCOL structure.

  @retval EFI_SUCCESS         The reset completed successfully.
  @retval EFI_ALREADY_STARTED The controller is busy with another transaction.
  @retval EFI_DEVICE_ERROR    The reset operation failed.

**/
STATIC
EFI_STATUS
EFIAPI
I2cReset (
  IN CONST EFI_I2C_MASTER_PROTOCOL *This
  )
{
  I2C_MASTER        *I2c;
  UINT32            Val32;

  I2c = I2C_FROM_THIS (This);

  /* Disable the interrupts */
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_IDR_OFFSET, CDNS_I2C_IXR_ALL_INTR_MASK);
  /* Clear the hold bit and fifos */
  Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET);
  Val32 &= ~CDNS_I2C_CR_HOLD;
	Val32 |= CDNS_I2C_CR_CLR_FIFO;
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);
  /* Update the transfercount register to zero */
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET, 0);
  /* Clear the interrupt status register */
  Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET);
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET, Val32);
  /* Clear the status register */
  Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_SR_OFFSET);
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_SR_OFFSET, Val32);

  /* Init */
  Val32 = CDNS_I2C_CR_MASTER_EN_MASK;
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);

  /*
   * Cadence I2C controller has a bug wherein it generates
   * invalid read transaction after HW timeout in master receiver mode.
   * HW timeout is not used by this driver and the interrupt is disabled.
   * But the feature itself cannot be disabled. Hence maximum value
   * is written to this register to reduce the chances of error.
   */
  MmioWrite32 (I2c->MmioBase + CDNS_I2C_TIME_OUT_OFFSET, CDNS_I2C_TIMEOUT_MAX);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
WaitForInterrupt (
  IN  I2C_MASTER        *I2c,
  IN  UINT32            Flag
  )
{
  UINT32   Val32;
  UINTN    Timeout = WAIT_FOR_INTERRUPT_TIMEOUT;
  UINTN    Delay = WAIT_FOR_INTERRUPT_DELAY;

  do {
    Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET);
    if (Val32 & CDNS_I2C_IXR_ERR_INTR_MASK) {
      DEBUG ((DEBUG_ERROR,
            "%a: Error interrupt - %X\n", __FUNCTION__, Val32));
      return  EFI_DEVICE_ERROR;
    }
    if (Val32 & Flag){
      return EFI_SUCCESS;
    }
    MicroSecondDelay (Delay);
    Timeout -= Delay;
  } while (Timeout);

  return EFI_DEVICE_ERROR;
}

STATIC
VOID
ClearBusHold (
  IN  I2C_MASTER        *I2c
)
{
  UINT32      Val32;

  Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET);
  if (Val32 & CDNS_I2C_CR_HOLD) {
    Val32 &= ~CDNS_I2C_CR_HOLD;
    MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);
  }
}

/**
  Start an I2C transaction on the host controller.

  This routine must be called at or below TPL_NOTIFY.  For synchronous
  requests this routine must be called at or below TPL_CALLBACK.

  This function initiates an I2C transaction on the controller.  To
  enable proper error handling by the I2C protocol stack, the I2C
  master protocol does not support queuing but instead only manages
  one I2C transaction at a time.  This API requires that the I2C bus
  is in the correct configuration for the I2C transaction.

  The transaction is performed by sending a start-bit and selecting the
  I2C device with the specified I2C slave address and then performing
  the specified I2C operations.  When multiple operations are requested
  they are separated with a repeated start bit and the slave address.
  The transaction is terminated with a stop bit.

  When Event is NULL, StartRequest operates synchronously and returns
  the I2C completion status as its return value.

  When Event is not NULL, StartRequest synchronously returns EFI_SUCCESS
  indicating that the I2C transaction was started asynchronously.  The
  transaction status value is returned in the buffer pointed to by
  I2cStatus upon the completion of the I2C transaction when I2cStatus
  is not NULL.  After the transaction status is returned the Event is
  signaled.

  Note: The typical consumer of this API is the I2C host protocol.
  Extreme care must be taken by other consumers of this API to prevent
  confusing the third party I2C drivers due to a state change at the
  I2C device which the third party I2C drivers did not initiate.  I2C
  platform specific code may use this API within these guidelines.

  @param[in] This           Pointer to an EFI_I2C_MASTER_PROTOCOL structure.
  @param[in] SlaveAddress   Address of the device on the I2C bus.  Set the
                            I2C_ADDRESSING_10_BIT when using 10-bit addresses,
                            clear this bit for 7-bit addressing.  Bits 0-6
                            are used for 7-bit I2C slave addresses and bits
                            0-9 are used for 10-bit I2C slave addresses.
  @param[in] RequestPacket  Pointer to an EFI_I2C_REQUEST_PACKET
                            structure describing the I2C transaction.
  @param[in] Event          Event to signal for asynchronous transactions,
                            NULL for synchronous transactions
  @param[out] I2cStatus     Optional buffer to receive the I2C transaction
                            completion status

  @retval EFI_SUCCESS           The asynchronous transaction was successfully
                                started when Event is not NULL.
  @retval EFI_SUCCESS           The transaction completed successfully when
                                Event is NULL.
  @retval EFI_ALREADY_STARTED   The controller is busy with another transaction.
  @retval EFI_BAD_BUFFER_SIZE   The RequestPacket->LengthInBytes value is too
                                large.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the
                                transaction.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL
  @retval EFI_NOT_FOUND         Reserved bit set in the SlaveAddress parameter
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the slave
                                address.  EFI_DEVICE_ERROR will be returned if
                                the controller cannot distinguish when the NACK
                                occurred.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for I2C transaction
  @retval EFI_UNSUPPORTED       The controller does not support the requested
                                transaction.

**/
STATIC
EFI_STATUS
EFIAPI
I2cStartRequest (
  IN  CONST EFI_I2C_MASTER_PROTOCOL   *This,
  IN  UINTN                           SlaveAddress,
  IN  EFI_I2C_REQUEST_PACKET          *RequestPacket,
  IN  EFI_EVENT                       Event      OPTIONAL,
  OUT EFI_STATUS                      *I2cStatus OPTIONAL
  )
{
  I2C_MASTER                  *I2c;
  UINTN                       Idx;
  EFI_I2C_OPERATION           *Op;
  UINTN                       BufIdx;
  EFI_STATUS                  Status;
  EFI_TPL                     Tpl;
  BOOLEAN                     AtRuntime;
  UINT32                      Remaining, ChunkSize;
  UINT32                      Val32;

  I2c = I2C_FROM_THIS (This);

  //
  // We can only do synchronous operations at runtime
  //
  AtRuntime = EfiAtRuntime ();
  if (AtRuntime && Event != NULL) {
    return EFI_UNSUPPORTED;
  }

  if (SlaveAddress & (~CDNS_I2C_ADDR_MASK)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!AtRuntime) {
    Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  }

  for (Idx = 0, Op = RequestPacket->Operation, Status = EFI_SUCCESS;
       Idx < RequestPacket->OperationCount && !EFI_ERROR (Status); Idx++, Op++) {
    Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_SR_OFFSET);
    if ((Val32 & CDNS_I2C_SR_BA)) {
      DEBUG ((DEBUG_INFO, "%a: bus is busy\n", __FUNCTION__));
      break;
    }

    BufIdx = 0;
    if (Op->Flags & I2C_FLAG_READ) {
      /* Put the controller in master receive mode and clear the FIFO */
      Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET);
      Val32 |= CDNS_I2C_CR_RW | CDNS_I2C_CR_CLR_FIFO;
      MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);

      if (Op->LengthInBytes > CDNS_I2C_FIFO_DEPTH) {
        Val32 |= CDNS_I2C_CR_HOLD;
        MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);
      }

      /* Clear the interrupts in interrupt status register */
      Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET);
      MmioWrite32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET, Val32);

      /*
      * The no. of bytes to receive is checked against the limit of
      * max transfer size. Set transfer size register with no of bytes
      * receive if it is less than transfer size and transfer size if
      * it is more. Enable the interrupts.
      */
      ChunkSize = Remaining = Op->LengthInBytes;
      if (ChunkSize > CDNS_I2C_TRANSFER_SIZE) {
        MmioWrite32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET, CDNS_I2C_TRANSFER_SIZE);
        ChunkSize = CDNS_I2C_TRANSFER_SIZE;
      } else {
        MmioWrite32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET, ChunkSize);
      }

      MmioWrite32 (I2c->MmioBase + CDNS_I2C_ADDR_OFFSET, SlaveAddress);
      do {
        Status = WaitForInterrupt (I2c, CDNS_I2C_IXR_NACK | CDNS_I2C_IXR_ARB_LOST |
                                        CDNS_I2C_IXR_COMP | CDNS_I2C_IXR_DATA);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN,
            "%a: Timeout waiting for interrupt - %r\n", __FUNCTION__, Status));
          break;
        }

        Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET);
        if (Val32 & (CDNS_I2C_IXR_NACK | CDNS_I2C_IXR_ARB_LOST)) {
          DEBUG ((DEBUG_ERROR,
                "%a: Nack and arbitration lost\n", __FUNCTION__));
            Status = EFI_DEVICE_ERROR;
            break;
        }
        if ((Val32 & CDNS_I2C_IXR_COMP) && !Remaining) {
          /* Clear hold  and signal completion */
          ClearBusHold (I2c);
          break;
        }

        while (MmioRead32 (I2c->MmioBase + CDNS_I2C_SR_OFFSET) & CDNS_I2C_SR_RXDV) {
          if (ChunkSize > 0) {
            Op->Buffer[BufIdx++] = MmioRead32 (I2c->MmioBase + CDNS_I2C_DATA_OFFSET);
            ChunkSize--;
            Remaining--;
            if (ChunkSize <= CDNS_I2C_FIFO_DEPTH) {
              ClearBusHold (I2c);
            }
          } else {
            DEBUG ((DEBUG_WARN,
                "%a: xfer_size reg rollover. xfer aborted!\n", __FUNCTION__));
            break;
          }
          /*
           * The controller sends NACK to the slave when transfer size
           * register reaches zero without considering the HOLD bit.
           * This workaround is implemented for large data transfers to
           * maintain transfer size non-zero while performing a large
           * receive operation.
           */
          if ((Remaining > ChunkSize) && (ChunkSize == (CDNS_I2C_FIFO_DEPTH + 1))) {
            /* wait while fifo is full */
            while (MmioRead32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET) != 1);
            ChunkSize = Remaining;
            if ((ChunkSize - CDNS_I2C_FIFO_DEPTH) > CDNS_I2C_TRANSFER_SIZE) {
              MmioWrite32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET, CDNS_I2C_TRANSFER_SIZE);
              ChunkSize = CDNS_I2C_TRANSFER_SIZE + CDNS_I2C_FIFO_DEPTH;
            } else {
              MmioWrite32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET, CDNS_I2C_FIFO_DEPTH);
            }
            break;
          }
        }
      } while (TRUE);
    } else {
      /* Put the controller in master receive mode and clear the FIFO */
      Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET);
      Val32 &= ~CDNS_I2C_CR_RW;
      Val32 |= CDNS_I2C_CR_CLR_FIFO;
      MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);

      ChunkSize = Remaining = Op->LengthInBytes;
      if (ChunkSize > CDNS_I2C_FIFO_DEPTH) {
        Val32 |= CDNS_I2C_CR_HOLD;
        MmioWrite32 (I2c->MmioBase + CDNS_I2C_CR_OFFSET, Val32);
        ChunkSize = CDNS_I2C_FIFO_DEPTH;
      }

      /* Clear the interrupts in interrupt status register */
      Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET);
      MmioWrite32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET, Val32);

      while (ChunkSize) {
        MmioWrite32 (I2c->MmioBase + CDNS_I2C_DATA_OFFSET, Op->Buffer[BufIdx++]);
        ChunkSize--;
        Remaining--;
      }

      /*
       * Clear the bus hold flag and trigger operation.
       */
      MmioWrite32 (I2c->MmioBase + CDNS_I2C_ADDR_OFFSET, SlaveAddress);
      MmioWrite32 (I2c->MmioBase + CDNS_I2C_IER_OFFSET, CDNS_I2C_ENABLED_INTR_MASK);
    
      do {
        Status = WaitForInterrupt (I2c, CDNS_I2C_IXR_NACK | CDNS_I2C_IXR_ARB_LOST |
                                        CDNS_I2C_IXR_COMP);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR,
            "%a: Timeout waiting for interrupt - %r\n", __FUNCTION__, Status));
          Status = EFI_DEVICE_ERROR;
          break;
        }

        Val32 = MmioRead32 (I2c->MmioBase + CDNS_I2C_ISR_OFFSET);
        if (Val32 & (CDNS_I2C_IXR_NACK | CDNS_I2C_IXR_ARB_LOST)) {
          DEBUG ((DEBUG_ERROR,
                "%a: Nack and arbitration lost\n", __FUNCTION__));
          Status = EFI_DEVICE_ERROR;
          break;
        }

        if ((Val32 & CDNS_I2C_IXR_COMP) && !Remaining) {
          /* Clear hold  and signal completion */
          ClearBusHold (I2c);
          break;
        }

        ChunkSize = Remaining;
        if (ChunkSize > (CDNS_I2C_FIFO_DEPTH - MmioRead32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET))) {
          ChunkSize = CDNS_I2C_FIFO_DEPTH - MmioRead32 (I2c->MmioBase + CDNS_I2C_XFER_SIZE_OFFSET);
        }
        while (ChunkSize) {
          MmioWrite32 (I2c->MmioBase + CDNS_I2C_DATA_OFFSET, Op->Buffer[BufIdx++]);
          ChunkSize--;
          Remaining--;
        }
      } while (TRUE);

    }
  }

  if (!AtRuntime) {
    gBS->RestoreTPL (Tpl);
  }

  if (Event) {
    *I2cStatus = Status;
    gBS->SignalEvent (Event);
  }
  return Status;
}

STATIC CONST EFI_I2C_CONTROLLER_CAPABILITIES mI2cControllerCapabilities = {
  sizeof (EFI_I2C_CONTROLLER_CAPABILITIES),   // StructureSizeInBytes
  MAX_UINT32,                                 // MaximumReceiveBytes
  MAX_UINT32,                                 // MaximumTransmitBytes
  MAX_UINT32,                                 // MaximumTotalBytes
};

STATIC
VOID
EFIAPI
I2cVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  I2C_MASTER    *I2c = Context;

  EfiConvertPointer (0x0, (VOID **)&I2c->I2cMaster.SetBusFrequency);
  EfiConvertPointer (0x0, (VOID **)&I2c->I2cMaster.Reset);
  EfiConvertPointer (0x0, (VOID **)&I2c->I2cMaster.StartRequest);
  EfiConvertPointer (0x0, (VOID **)&I2c->I2cMaster.I2cControllerCapabilities);
  EfiConvertPointer (0x0, (VOID **)&I2c->MmioBase);
}

EFI_STATUS
I2cInit (
  IN      EFI_HANDLE        DriverBindingHandle,
  IN      EFI_HANDLE        ControllerHandle
  )
{
  EFI_STATUS                        Status;
  NON_DISCOVERABLE_DEVICE           *Dev;
  I2C_MASTER                        *I2c;
  BOOLEAN                           Runtime;

  Status = gBS->OpenProtocol (ControllerHandle,
                              &gEdkiiNonDiscoverableDeviceProtocolGuid,
                              (VOID **)&Dev, DriverBindingHandle,
                              ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Runtime = CompareGuid (Dev->Type,
                         &gOrbiterNonDiscoverableRuntimeSysI2cMasterGuid);

  // Allocate Resources
  if (Runtime) {
    I2c = AllocateRuntimeZeroPool (sizeof (I2C_MASTER));
  } else {
    I2c = AllocateZeroPool (sizeof (I2C_MASTER));
  }
  if (I2c == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseProtocol;
  }

  I2c->Signature                            = I2C_SIGNATURE;
  I2c->I2cMaster.SetBusFrequency            = I2cSetBusFrequency;
  I2c->I2cMaster.Reset                      = I2cReset;
  I2c->I2cMaster.StartRequest               = I2cStartRequest;
  I2c->I2cMaster.I2cControllerCapabilities  = &mI2cControllerCapabilities;
  I2c->MmioBase                             = Dev->Resources[0].AddrRangeMin;
  I2c->Dev                                  = Dev;

  if (Runtime) {
    I2c->Runtime = TRUE;

    // Declare the controller as EFI_MEMORY_RUNTIME
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    Dev->Resources[0].AddrRangeMin,
                    Dev->Resources[0].AddrLen,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: failed to add memory space - %r\n",
        __FUNCTION__, Status));
    }

    Status = gDS->SetMemorySpaceAttributes (
                    Dev->Resources[0].AddrRangeMin,
                    Dev->Resources[0].AddrLen,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    if (EFI_ERROR (Status)) {
      goto FreeDevice;
    }

    //
    // Register for the virtual address change event
    //
    Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
                    I2cVirtualNotifyEvent, I2c,
                    &gEfiEventVirtualAddressChangeGuid,
                    &I2c->VirtualAddressChangeEvent);
    if (EFI_ERROR (Status)) {
      goto FreeDevice;
    }
  }

  CopyGuid (&I2c->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  I2c->DevicePath.MmioBase = I2c->MmioBase;
  SetDevicePathNodeLength (&I2c->DevicePath.Vendor,
    sizeof (I2c->DevicePath) - sizeof (I2c->DevicePath.End));
  SetDevicePathEndNode (&I2c->DevicePath.End);

  Status = gBS->InstallMultipleProtocolInterfaces (&ControllerHandle,
                  &gEfiI2cMasterProtocolGuid, &I2c->I2cMaster,
                  &gEfiDevicePathProtocolGuid, &I2c->DevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    goto CloseEvent;
  }
  return EFI_SUCCESS;

CloseEvent:
  if (Runtime) {
    gBS->CloseEvent (I2c->VirtualAddressChangeEvent);
  }

FreeDevice:
  FreePool (I2c);

CloseProtocol:
  gBS->CloseProtocol (ControllerHandle,
                      &gEdkiiNonDiscoverableDeviceProtocolGuid,
                      DriverBindingHandle,
                      ControllerHandle);
  return Status;
}

EFI_STATUS
I2cRelease (
  IN      EFI_HANDLE        DriverBindingHandle,
  IN      EFI_HANDLE        ControllerHandle
  )
{
  EFI_I2C_MASTER_PROTOCOL       *I2cMaster;
  I2C_MASTER          *I2c;
  EFI_STATUS                    Status;

  Status = gBS->HandleProtocol (ControllerHandle,
                                &gEfiI2cMasterProtocolGuid,
                                (VOID **)&I2cMaster);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  I2c = I2C_FROM_THIS (I2cMaster);

  Status = gBS->UninstallMultipleProtocolInterfaces (ControllerHandle,
                  &gEfiI2cMasterProtocolGuid, I2cMaster,
                  &gEfiDevicePathProtocolGuid, &I2c->DevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (I2c->Runtime) {
    gBS->CloseEvent (I2c->VirtualAddressChangeEvent);
  }

  Status = gBS->CloseProtocol (ControllerHandle,
                               &gEdkiiNonDiscoverableDeviceProtocolGuid,
                               DriverBindingHandle,
                               ControllerHandle);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->FreePool (I2c);

  return EFI_SUCCESS;
}
