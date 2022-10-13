/** @file

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "QspiDxe.h"

/**
  Read data from the flash part.

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[out] Buffer              The Pointer to caller-allocated buffer containing the dada received.
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
QspiFlashRead (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  IN     UINT32               Address,
  IN     UINT32               ByteCount,
  OUT    UINT8                *Buffer
  )
{
  UINT32      RegionBase;
  UINT32      RegionSize;
  EFI_STATUS  Status;

  if (!This) {
    return EFI_INVALID_PARAMETER;
  }

  Status = QspiFlashGetRegion (This, FlashRegionType, &RegionBase, &RegionSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Address >= RegionSize || (Address + ByteCount) >= RegionSize) {
    DEBUG ((DEBUG_VERBOSE, "%a: Address 0x%X and size 0x%X is out of range of %d (0x%X - 0x%X) \n",
        __FUNCTION__, Address, ByteCount, FlashRegionType, RegionBase, RegionSize));
    return EFI_INVALID_PARAMETER;
  }

  Status = CdnsXspiRead ((VOID *)Buffer, RegionBase + Address, ByteCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to read bytes from flash %r\n",
        __FUNCTION__, Status));
  }

  return Status;
}

/**
  Write data to the flash part.

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[in] Buffer               Pointer to caller-allocated buffer containing the data sent during the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
QspiFlashWrite (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  IN     UINT32               Address,
  IN     UINT32               ByteCount,
  IN     UINT8                *Buffer
  )
{
  UINT32      RegionBase;
  UINT32      RegionSize;
  EFI_STATUS  Status;

  if (!This) {
    return EFI_INVALID_PARAMETER;
  }

  Status = QspiFlashGetRegion (This, FlashRegionType, &RegionBase, &RegionSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Address >= RegionSize || (Address + ByteCount) >= RegionSize) {
    DEBUG ((DEBUG_VERBOSE, "%a: Address 0x%X and size 0x%X is out of range of %d (0x%X - 0x%X) \n",
        __FUNCTION__, Address, ByteCount, FlashRegionType, RegionBase, RegionSize));
    return EFI_INVALID_PARAMETER;
  }

  Status = CdnsXspiUpdate ((VOID *)Buffer, RegionBase + Address, ByteCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to write bytes to flash %r\n",
        __FUNCTION__, Status));
  }

  return Status;
}

/**
  Erase some area on the flash part.

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
QspiFlashErase (
  IN     SPI_FLASH_PROTOCOL     *This,
  IN     FLASH_REGION_TYPE      FlashRegionType,
  IN     UINT32                 Address,
  IN     UINT32                 ByteCount
  )
{
  UINT32      RegionBase;
  UINT32      RegionSize;
  EFI_STATUS  Status;

  if (!This) {
    return EFI_INVALID_PARAMETER;
  }

  Status = QspiFlashGetRegion (This, FlashRegionType, &RegionBase, &RegionSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Address >= RegionSize || (Address + ByteCount) >= RegionSize) {
    DEBUG ((DEBUG_VERBOSE, "%a: Address 0x%X and size 0x%X is out of range of %d (0x%X - 0x%X) \n",
        __FUNCTION__, Address, ByteCount, FlashRegionType, RegionBase, RegionSize));
    return EFI_INVALID_PARAMETER;
  }

  Status = CdnsXspiErase (RegionBase + Address, ByteCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to erase %r\n",
        __FUNCTION__, Status));
  }

  return Status;
}

/**
  Get the SPI region base and size, based on the enum type

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for for the base address which is listed in the Descriptor.
  @param[out] BaseAddress         The Flash Linear Address for the Region 'n' Base
  @param[out] RegionSize          The size for the Region 'n'

  @retval EFI_SUCCESS             Read success
  @retval EFI_INVALID_PARAMETER   Invalid region type given
  @retval EFI_DEVICE_ERROR        The region is not used
**/
EFI_STATUS
QspiFlashGetRegion (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  OUT    UINT32               *BaseAddress,
  OUT    UINT32               *RegionSize
  )
{
  if (!This || !BaseAddress || !RegionSize) {
    return EFI_INVALID_PARAMETER;
  }

  switch (FlashRegionType) {
  case FlashRegionBios:
    *BaseAddress = FixedPcdGet32 (PcdQspiBiosRegionBase);
    *RegionSize = FixedPcdGet32 (PcdQspiBiosRegionSize);
    break;
  case FlashRegionNv:
    *BaseAddress = FixedPcdGet32 (PcdQspiNvRegionBase);
    *RegionSize = FixedPcdGet32 (PcdQspiNvRegionSize);
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
QspiVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  QSPI_PRIVATE      *Qspi = Context;

  EfiConvertPointer (0x0, (VOID **)&Qspi->SpiFlashProtocol.FlashRead);
  EfiConvertPointer (0x0, (VOID **)&Qspi->SpiFlashProtocol.FlashWrite);
  EfiConvertPointer (0x0, (VOID **)&Qspi->SpiFlashProtocol.FlashErase);
  EfiConvertPointer (0x0, (VOID **)&Qspi->MmioBase);
  CdnsXspiUpdateBaseAddress (Qspi->MmioBase);
}

EFI_STATUS
QspiInit (
  IN      EFI_HANDLE        DriverBindingHandle,
  IN      EFI_HANDLE        ControllerHandle
  )
{
  EFI_STATUS                        Status;
  NON_DISCOVERABLE_DEVICE           *Dev;
  QSPI_PRIVATE                      *Qspi;

  Status = gBS->OpenProtocol (ControllerHandle,
                              &gEdkiiNonDiscoverableDeviceProtocolGuid,
                              (VOID **)&Dev, DriverBindingHandle,
                              ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Allocate Resources
  Qspi = AllocateRuntimeZeroPool (sizeof (QSPI_PRIVATE));

  if (Qspi == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseProtocol;
  }

  Qspi->Signature                            = QSPI_SIGNATURE;
  Qspi->SpiFlashProtocol.Revision            = SPI_FLASH_SERVICES_REVISION;
  Qspi->SpiFlashProtocol.FlashRead           = QspiFlashRead;
  Qspi->SpiFlashProtocol.FlashWrite          = QspiFlashWrite;
  Qspi->SpiFlashProtocol.FlashErase          = QspiFlashErase;
  Qspi->SpiFlashProtocol.FlashGetRegion      = QspiFlashGetRegion;
  Qspi->MmioBase                             = Dev->Resources[0].AddrRangeMin;
  Qspi->MmioSize                             = Dev->Resources[0].AddrLen;
  Qspi->Dev                                  = Dev;

  // Initialize SPi
  if (EFI_ERROR (CdnsXspiInit (Qspi->MmioBase))) {
    DEBUG ((DEBUG_ERROR, "%a: failed to initialize QSPI controller\n",
            __FUNCTION__));
    Status = EFI_DEVICE_ERROR;
    goto CloseProtocol;
  }

  // Declare the controller as EFI_MEMORY_RUNTIME
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  Qspi->MmioBase,
                  Qspi->MmioSize,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: failed to add memory space - %r\n",
      __FUNCTION__, Status));
  }

  Status = gDS->SetMemorySpaceAttributes (
                  Qspi->MmioBase,
                  Qspi->MmioSize,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
  if (EFI_ERROR (Status)) {
    goto FreeDevice;
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
                  QspiVirtualNotifyEvent, Qspi,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Qspi->VirtualAddressChangeEvent);
  if (EFI_ERROR (Status)) {
    goto FreeDevice;
  }

  CopyGuid (&Qspi->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Qspi->DevicePath.MmioBase = Qspi->MmioBase;
  SetDevicePathNodeLength (&Qspi->DevicePath.Vendor,
    sizeof (Qspi->DevicePath) - sizeof (Qspi->DevicePath.End));
  SetDevicePathEndNode (&Qspi->DevicePath.End);

  Status = gBS->InstallMultipleProtocolInterfaces (&ControllerHandle,
                  &gOrbiterSpiFlashProtocolGuid, &Qspi->SpiFlashProtocol,
                  &gEfiDevicePathProtocolGuid, &Qspi->DevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    goto CloseEvent;
  }
  return EFI_SUCCESS;

CloseEvent:
  gBS->CloseEvent (Qspi->VirtualAddressChangeEvent);

FreeDevice:
  FreePool (Qspi);

CloseProtocol:
  gBS->CloseProtocol (ControllerHandle,
                      &gEdkiiNonDiscoverableDeviceProtocolGuid,
                      DriverBindingHandle,
                      ControllerHandle);
  return Status;
}

EFI_STATUS
QspiRelease (
  IN      EFI_HANDLE        DriverBindingHandle,
  IN      EFI_HANDLE        ControllerHandle
  )
{
  QSPI_PRIVATE              *Qspi;
  SPI_FLASH_PROTOCOL        *SpiFlash;
  EFI_STATUS                Status;

  Status = gBS->HandleProtocol (ControllerHandle,
                                &gOrbiterSpiFlashProtocolGuid,
                                (VOID **)&SpiFlash);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Qspi = QSPI_FROM_THIS (SpiFlash);

  Status = gBS->UninstallMultipleProtocolInterfaces (ControllerHandle,
                  &gOrbiterSpiFlashProtocolGuid, SpiFlash,
                  &gEfiDevicePathProtocolGuid, &Qspi->DevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseEvent (Qspi->VirtualAddressChangeEvent);

  Status = gBS->CloseProtocol (ControllerHandle,
                               &gEdkiiNonDiscoverableDeviceProtocolGuid,
                               DriverBindingHandle,
                               ControllerHandle);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->FreePool (Qspi);

  return EFI_SUCCESS;
}
