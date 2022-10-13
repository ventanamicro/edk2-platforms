/** @file

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/TimerLib.h>
#include "CadenceXspi.h"

#define LESS(a, b)   (((a) < (b)) ? (a) : (b))
#define MORE(a, b)   (((a) > (b)) ? (a) : (b))

#define SUBSECTOR_SIZE                            0x1000
#define PAGE_SIZE                                 0x100
#define SDMA_SIZE                                 0x4000
#define WAIT_READY_TIMEOUT                        0x50000 /* 500 miliseconds*/
#define WAIT_READY_DELAY                          0x100

#define QSPI_STIG_OPCODE_READ_STATUS              0x05
#define  READ_STATUS_WIP                          BIT(0)
#define QSPI_STIG_OPCODE_MICRON_READ_FLAG_STATUS  0x70
#define  MICRON_STATUS_PROTECTION_ERROR           BIT(1)
#define  MICRON_STATUS_PROGRAM_ERROR              BIT(4)
#define  MICRON_STATUS_ERASE_ERROR                BIT(5)
#define  MICRON_STATUS_WIP                        BIT(7)
#define QSPI_STIG_OPCODE_MICRON_CLEAR_FLAG_STATUS 0x50
#define QSPI_STIG_OPCODE_WRITE_STATUS             0x01
#define QSPI_STIG_OPCODE_QUAD_FAST_READ           0x6C
#define QSPI_STIG_OPCODE_QUAD_FAST_PROGRAM        0x34
#define QSPI_STIG_OPCODE_RDID                     0x9f
#define QSPI_STIG_OPCODE_WRDIS                    0x4
#define QSPI_STIG_OPCODE_WREN                     0x6
#define QSPI_STIG_OPCODE_SUBSEC_4K_ERASE          0x21
#define QSPI_STIG_OPCODE_RESET_EN                 0x66
#define QSPI_STIG_OPCODE_RESET_MEM                0x99
#define QSPI_STIG_OPCODE_ENTER_4BYTE_ADDR_MODE    0xB7
#define QSPI_STIG_OPCODE_ENTER_QUAD_SPI_MODE      0x35
#define QSPI_STIG_RDID_CAPACITYID(x)              (((x) >> 16) & 0xff)

STATIC UINT64   mCdnsXspiBaseAddress;
STATIC UINT64   mCdnsXspiSdmaAddress;
STATIC UINT32   mNumBanks;
STATIC UINT8    mCurCs;
STATIC UINT32   mDeviceSize;
STATIC UINTN    mAddrSize;
STATIC UINT8    mAddrBusWidth;
STATIC UINT8    mOpcodeBusWidth;
STATIC UINT8    mDataBusWidth;

VOID CdnsXspiUpdateBaseAddress (IN UINT64 BaseAddress)
{
  mCdnsXspiSdmaAddress += (BaseAddress - mCdnsXspiBaseAddress);
  mCdnsXspiBaseAddress = BaseAddress;
}

UINT64 CdnsXspiGetSdmaAddress (VOID)
{
  UINT64 Addr;

  Addr = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_SDMA_ADDR1);
  Addr <<= 32;
  Addr |= MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_SDMA_ADDR0);

  return Addr;
}

STATIC VOID CdnsXspiSetInterrupts (IN BOOLEAN Enabled)
{
  UINT32 IntrEnable;

  IntrEnable = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_INTR_ENABLE_REG);
  if (Enabled) {
    IntrEnable |= CDNS_XSPI_INTR_MASK;
  } else {
    IntrEnable &= ~CDNS_XSPI_INTR_MASK;
  }
  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_INTR_ENABLE_REG, IntrEnable);
}

STATIC EFI_STATUS CdnsXspiWaitForControllerIdle (VOID)
{
  UINT32 CtrlStat;
  INT32 Timeout = 1000;
  INT32 Delay = 100;

  do {
    CtrlStat = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_CTRL_STATUS_REG);
    if ((CtrlStat & CDNS_XSPI_CTRL_BUSY) == 0) {
      return EFI_SUCCESS;
    }
    MicroSecondDelay (Delay);
    Timeout -= Delay;
  } while (Timeout > 0);

  return  EFI_DEVICE_ERROR;
}

STATIC VOID CdnsXspiTriggerCommand (IN UINT32 *CmdRegs)
{
  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_CMD_REG_5, CmdRegs[5]);
  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_CMD_REG_4, CmdRegs[4]);
  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_CMD_REG_3, CmdRegs[3]);
  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_CMD_REG_2, CmdRegs[2]);
  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_CMD_REG_1, CmdRegs[1]);
  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_CMD_REG_0, CmdRegs[0]);
}

STATIC EFI_STATUS CdnsXspiCheckCommandStatus (VOID)
{
  EFI_STATUS Status;
  UINT32 CmdStatus;

  CmdStatus = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_CMD_STATUS_REG);

  if (CmdStatus & CDNS_XSPI_CMD_STATUS_COMPLETED) {
    if ((CmdStatus & CDNS_XSPI_CMD_STATUS_FAILED) != 0) {
      if (CmdStatus & CDNS_XSPI_CMD_STATUS_DQS_ERROR) {
        DEBUG ((DEBUG_ERROR,
          "CdnsXspi: Incorrect DQS pulses detected\n"));
        Status = EFI_DEVICE_ERROR;
      }
      if (CmdStatus & CDNS_XSPI_CMD_STATUS_CRC_ERROR) {
        DEBUG ((DEBUG_ERROR,
          "CdnsXspi: CRC error received\n"));
        Status = EFI_DEVICE_ERROR;
      }
      if (CmdStatus & CDNS_XSPI_CMD_STATUS_BUS_ERROR) {
        DEBUG ((DEBUG_ERROR,
          "CdnsXspi: Error resp on system DMA interface\n"));
        Status = EFI_DEVICE_ERROR;
      }
      if (CmdStatus & CDNS_XSPI_CMD_STATUS_INV_SEQ_ERROR) {
        DEBUG ((DEBUG_ERROR,
          "CdnsXspi: Invalid command sequence detected\n"));
        Status = EFI_DEVICE_ERROR;
      }
    }
  } else {
    DEBUG ((DEBUG_ERROR,
          "CdnsXspi: Fatal err - command not completed\n"));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

STATIC EFI_STATUS CdnsXspiWaitComplete (VOID)
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINT32        IrqStatus;
  INTN          Delay = 10;

  do {
    IrqStatus = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_INTR_STATUS_REG);
    if (IrqStatus) {
      MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_INTR_STATUS_REG, IrqStatus);
    }
    if (IrqStatus &
        (CDNS_XSPI_SDMA_ERROR | CDNS_XSPI_SDMA_TRIGGER |
        CDNS_XSPI_STIG_DONE)) {
      if (IrqStatus & CDNS_XSPI_SDMA_ERROR) {
        DEBUG ((DEBUG_ERROR,
            "CdnsXspi: Slave DMA transaction error\n"));
        Status = EFI_DEVICE_ERROR;
        break;
      }
      if (IrqStatus & CDNS_XSPI_SDMA_TRIGGER) {
        break;
      }
      if (IrqStatus & CDNS_XSPI_STIG_DONE) {
        break;
      }
    }
    MicroSecondDelay (Delay);
  } while (1);

  return Status;
}

STATIC EFI_STATUS CdnsXspiSendStigCommand (
  IN UINT64 Addr, IN UINT32 AddrNumByte,
  IN UINT16 Opcode, IN BOOLEAN Read,
  IN OUT UINT8 *Data, IN UINT32 DataNumByte,
  IN UINT32 DummyNumBase, IN UINT32 DataPhase
  )
{
  UINT32          CmdRegs[6];
  UINT32          SdmaSize, SdmaTrdInfo;
  UINT8           SdmaDir;
  EFI_STATUS      Status;

  Status = CdnsXspiWaitForControllerIdle ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DataPhase && !DataNumByte) {
    return EFI_INVALID_PARAMETER;
  }

  MmioWrite32 (mCdnsXspiBaseAddress + CDNS_XSPI_CTRL_CONFIG_REG, FIELD_PREP(CDNS_XSPI_CTRL_WORK_MODE, CDNS_XSPI_WORK_MODE_STIG));

  CdnsXspiSetInterrupts (TRUE);

  SetMem ((VOID *) CmdRegs, 0, sizeof(CmdRegs));
  CmdRegs[1] = CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_1 (Addr, DataPhase);
  CmdRegs[2] = CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_2 (Addr);
  CmdRegs[3] = CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_3 (Addr, Opcode, AddrNumByte);
  CmdRegs[4] = CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_4 (mAddrBusWidth, mOpcodeBusWidth, mCurCs);

  CdnsXspiTriggerCommand (CmdRegs);

  if (DataPhase) {
    CmdRegs[0] = CDNS_XSPI_STIG_DONE_FLAG;
    CmdRegs[1] = CDNS_XSPI_CMD_FLD_DSEQ_CMD_1 ();
    CmdRegs[2] = CDNS_XSPI_CMD_FLD_DSEQ_CMD_2 (DataNumByte);
    CmdRegs[3] = CDNS_XSPI_CMD_FLD_DSEQ_CMD_3 (DataNumByte, DummyNumBase);
    CmdRegs[4] = CDNS_XSPI_CMD_FLD_DSEQ_CMD_4 (mDataBusWidth, Read, mCurCs);

    CdnsXspiTriggerCommand (CmdRegs);

    Status = CdnsXspiWaitComplete ();
    if (EFI_ERROR (Status)) {
      CdnsXspiSetInterrupts (FALSE);
      return Status;
    }
    SdmaSize = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_SDMA_SIZE_REG);
    SdmaTrdInfo = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_SDMA_TRD_INFO_REG);
    SdmaDir = FIELD_GET(CDNS_XSPI_SDMA_DIR, SdmaTrdInfo);
    switch (SdmaDir) {
    case CDNS_XSPI_SDMA_DIR_READ:
      CopyMem ((VOID *)Data, (VOID *)mCdnsXspiSdmaAddress, SdmaSize);
      break;
    case CDNS_XSPI_SDMA_DIR_WRITE:
      CopyMem ((VOID *)mCdnsXspiSdmaAddress, (VOID *)Data, SdmaSize);
      break;
    }
  }

  Status = CdnsXspiWaitComplete ();
  CdnsXspiSetInterrupts (FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CdnsXspiCheckCommandStatus ();
  return Status;;
}

EFI_STATUS CdnsXspiInit (IN UINT64 BaseAddress)
{
  UINT32      CtrlVer;
  UINT32      CtrlFeatures;
  UINT16      MagicNum;
  EFI_STATUS  Status = 0;
  UINT32      CapCode;
  UINT32      DecodedCap;

  mCdnsXspiBaseAddress = BaseAddress;

  CtrlVer = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_CTRL_VERSION_REG);
  MagicNum = FIELD_GET (CDNS_XSPI_MAGIC_NUM, CtrlVer);
  if (MagicNum != CDNS_XSPI_MAGIC_NUM_VALUE) {
    DEBUG ((DEBUG_ERROR, 
      "CdnsXspi: Incorrect XSPI magic number: %x, expected: %x\n",
      MagicNum, CDNS_XSPI_MAGIC_NUM_VALUE));
    return EFI_INVALID_PARAMETER;
  }
  mCdnsXspiSdmaAddress = CdnsXspiGetSdmaAddress ();

  CtrlFeatures = MmioRead32 (mCdnsXspiBaseAddress + CDNS_XSPI_CTRL_FEATURES_REG);
  mNumBanks = FIELD_GET (CDNS_XSPI_NUM_BANKS, CtrlFeatures);
  DEBUG ((DEBUG_INFO,
      "CdnsXspi: Number of bank supported by HW:%d\n", mNumBanks));
  mCurCs = 0;
  mDeviceSize = 0;

  CdnsXspiSetInterrupts(FALSE);

  /* Read device ID and setup operation settings */
  /* Select single SPI mode before we know the device supports QSPI protocol*/
  mAddrBusWidth = CDNS_XSPI_STIG_IO_MODE_SINGLE;
  mOpcodeBusWidth = CDNS_XSPI_STIG_IO_MODE_SINGLE;
  mDataBusWidth = CDNS_XSPI_STIG_IO_MODE_SINGLE;
   Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_RDID, TRUE, (UINT8 *)&CapCode, 3, 0, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
          "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_RDID));
    return EFI_DEVICE_ERROR;
  }

  /*
   * NOTE: The Size code seems to be a form of BCD (binary coded decimal).
   * The first nibble is the 10's digit and the second nibble is the 1's
   * digit in the number of bytes.
   *
   * Capacity ID samples:
   * 0x15 :   16 Mb =>   2 MiB => 1 << 21 ; BCD=15
   * 0x16 :   32 Mb =>   4 MiB => 1 << 22 ; BCD=16
   * 0x17 :   64 Mb =>   8 MiB => 1 << 23 ; BCD=17
   * 0x18 :  128 Mb =>  16 MiB => 1 << 24 ; BCD=18
   * 0x19 :  256 Mb =>  32 MiB => 1 << 25 ; BCD=19
   * 0x1a
   * 0x1b
   * 0x1c
   * 0x1d
   * 0x1e
   * 0x1f
   * 0x20 :  512 Mb =>  64 MiB => 1 << 26 ; BCD=20
   * 0x21 : 1024 Mb => 128 MiB => 1 << 27 ; BCD=21
   */

  CapCode = QSPI_STIG_RDID_CAPACITYID (CapCode);

  if (!(((CapCode >> 4) > 0x9) || ((CapCode & 0xf) > 0x9))) {
    DecodedCap = ((CapCode >> 4) * 10) +
                  (CapCode & 0xf);
    mDeviceSize = 1 << (DecodedCap + 6);
    DEBUG ((DEBUG_INFO,
          "CdnsXspi: SPI NOR size detected: 0x%02x\n",
            mDeviceSize));
  } else {
    DEBUG ((DEBUG_ERROR,
          "CdnsXspi: Invalid CapacityID enCountered: 0x%02x\n",
            CapCode));
    return EFI_DEVICE_ERROR;
  }

  /* FIXME: Supports multiple devices. For now only supports MT25QL01GBBB */
  /* Enter 4byte address and Quad SPI mode*/
  Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_ENTER_4BYTE_ADDR_MODE, FALSE, NULL, 0, 0, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
          "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_ENTER_4BYTE_ADDR_MODE));
    return EFI_DEVICE_ERROR;
  }
  Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_ENTER_QUAD_SPI_MODE, FALSE, NULL, 0, 0, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
          "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_ENTER_QUAD_SPI_MODE));
    return EFI_DEVICE_ERROR;
  }
  mAddrBusWidth = CDNS_XSPI_STIG_IO_MODE_QUAD;
  mOpcodeBusWidth = CDNS_XSPI_STIG_IO_MODE_QUAD;
  mDataBusWidth = CDNS_XSPI_STIG_IO_MODE_QUAD;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS CdnsXspiWriteDisable (VOID)
{
  EFI_STATUS Status;

  Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_WRDIS, 0, NULL, 0, 0, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
            "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_WRDIS));
  }

  return Status;
}

STATIC
EFI_STATUS CdnsXspiWriteEnable (VOID)
{
  EFI_STATUS Status ;

  Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_WREN, 0, NULL, 0, 0, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
            "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_WREN));
  }

  return Status;
}

STATIC
EFI_STATUS CdnsXspiWaitTillReady (VOID)
{
  EFI_STATUS Status = 0;
  UINT8      Reg;
  INTN       Timeout = WAIT_READY_TIMEOUT;

  do {
    MicroSecondDelay (WAIT_READY_DELAY);
    Timeout -= WAIT_READY_DELAY;
  
    Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_READ_STATUS, TRUE, &Reg, sizeof (Reg), 0, 1);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
            "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_READ_STATUS));
      return Status;
    }
    if (Reg & READ_STATUS_WIP) {
      continue;
    }

    /* Only support Micron spi nor so querry Micron status register*/
    Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_MICRON_READ_FLAG_STATUS, TRUE, &Reg, sizeof (Reg), 0, 1);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
            "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_MICRON_READ_FLAG_STATUS));
      return Status;
    }

    if (Reg & (MICRON_STATUS_ERASE_ERROR | MICRON_STATUS_PROGRAM_ERROR)) {
      if (Reg & MICRON_STATUS_PROGRAM_ERROR) {
        DEBUG ((DEBUG_ERROR,
              "CdnsXspi: [%a] Micron program error\n", __FUNCTION__));
      } else {
        DEBUG ((DEBUG_ERROR,
              "CdnsXspi: [%a] Micron erase error\n", __FUNCTION__));
      }
      if (Reg & MICRON_STATUS_PROTECTION_ERROR) {
        DEBUG ((DEBUG_ERROR,
              "CdnsXspi: [%a] Attempted to modify a protected sector\n", __FUNCTION__));
      }

      /* Clear flag */
      Status = CdnsXspiSendStigCommand (0, 0, QSPI_STIG_OPCODE_MICRON_CLEAR_FLAG_STATUS, FALSE, NULL, 0, 0, 0);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR,
              "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_MICRON_CLEAR_FLAG_STATUS));
        return Status;
      }

      /*
      * WEL bit remains set to one when an erase or page program
      * error occurs. Issue a Write Disable command to protect
      * against inadvertent writes that can possibly corrupt the
      * contents of the memory.
      */
      Status = CdnsXspiWriteDisable ();
      if (EFI_ERROR (Status)) {
        return Status;
      }

      return EFI_DEVICE_ERROR;
    }

  } while (!(Reg & MICRON_STATUS_WIP) && Timeout);

  return  EFI_SUCCESS;
}

EFI_STATUS CdnsXspiRead (UINT8 *Buffer, UINT32 Offset, UINT32 Size)
{
  EFI_STATUS    Status;
  UINT32        CopyLen;

  Status = EFI_SUCCESS;

  if ((Offset >= mDeviceSize) ||
      (Offset + Size - 1 >= mDeviceSize)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Size == 0) {
    return EFI_SUCCESS;
  }

  while (Size > 0) {
    CopyLen = LESS (Size, SDMA_SIZE - (Offset & (SDMA_SIZE - 1)));
    Status = CdnsXspiSendStigCommand (Offset, mAddrSize,
                  QSPI_STIG_OPCODE_QUAD_FAST_READ, TRUE, Buffer, Size, 0, 1);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
            "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_QUAD_FAST_READ));
      break;
    }

    Offset += CopyLen;
    Buffer += CopyLen;
    Size -= CopyLen;
    CopyLen = LESS(Size, SDMA_SIZE);
  }

  return Status;
}

EFI_STATUS CdnsXspiWrite (UINT8 *Buffer, UINT32 Offset, UINT32 Size)
{
  EFI_STATUS    Status;
  UINT32        CopyLen;

  if ((Offset >= mDeviceSize) ||
      (Offset + Size - 1 >= mDeviceSize)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Size == 0) {
    return EFI_SUCCESS;
  }

  ASSERT (PAGE_SIZE <= SDMA_SIZE);
  Status = CdnsXspiWriteEnable ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (Size > 0) {
    CopyLen = LESS (Size, PAGE_SIZE - (Offset & (PAGE_SIZE - 1)));
    Status = CdnsXspiSendStigCommand (Offset, mAddrSize, 
                      QSPI_STIG_OPCODE_QUAD_FAST_PROGRAM, FALSE, Buffer, Size, 0, 1);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
            "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_QUAD_FAST_PROGRAM));
      break;
    }
    Status = CdnsXspiWaitTillReady ();
    if (EFI_ERROR (Status)) {
      break;
    }
    Offset += CopyLen;
    Buffer += CopyLen;
    Size -= CopyLen;
    CopyLen = LESS(Size, PAGE_SIZE);
  }

  CdnsXspiWriteDisable ();
  return Status;
}

STATIC
EFI_STATUS CdnsXspiEraseSubsector (IN UINT32 Addr)
{
  EFI_STATUS Status = 0;

  Status = CdnsXspiSendStigCommand (Addr, mAddrSize, QSPI_STIG_OPCODE_SUBSEC_4K_ERASE, FALSE, NULL, 0, 0, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
          "CdnsXspi: [%a] Failed to send %d opcode\n", __FUNCTION__, QSPI_STIG_OPCODE_SUBSEC_4K_ERASE));
  }

  return CdnsXspiWaitTillReady ();
}

EFI_STATUS CdnsXspiErase (UINT32 Offset, UINT32 Size)
{
  EFI_STATUS  Status = 0;
  UINT32      SubsectorOffset;
  UINT32      EraseSize;
  UINT8       *TmpBuffer;
  BOOLEAN     CopyBack;

  TmpBuffer = (UINT8 *)AllocateZeroPool (SUBSECTOR_SIZE);
  while (Size) {
    CopyBack = FALSE;
    SubsectorOffset  = Offset & (SUBSECTOR_SIZE - 1);
    EraseSize = LESS (Size, SUBSECTOR_SIZE - SubsectorOffset);
    if (SubsectorOffset || EraseSize != SUBSECTOR_SIZE) {
      CopyBack = TRUE;
    }
    
    if (CopyBack) {
      Status = CdnsXspiRead (TmpBuffer, Offset - SubsectorOffset, SUBSECTOR_SIZE);
      if (EFI_ERROR (Status)) {
        break;
      }
    }

    Status = CdnsXspiWriteEnable ();
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = CdnsXspiEraseSubsector (Offset - SubsectorOffset);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (CopyBack) {
      Status = CdnsXspiWrite (TmpBuffer, Offset - SubsectorOffset, SubsectorOffset);
      if (EFI_ERROR (Status)) {
        break;
      }
      if (EraseSize < SUBSECTOR_SIZE) {
        Status = CdnsXspiWrite (TmpBuffer + Offset, Offset + EraseSize, SUBSECTOR_SIZE - EraseSize - SubsectorOffset);
        if (EFI_ERROR (Status)) {
          break;
        }
      }
    }

    Offset  += EraseSize;
    Size -= EraseSize;
  }

  FreePool (TmpBuffer);

  CdnsXspiWriteDisable ();
  return Status;
}

EFI_STATUS CdnsXspiUpdate (UINT8 *Buffer, UINT32 Offset, UINT32 Size)
{
  EFI_STATUS Status;

  Status = CdnsXspiErase (Offset, Size);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return CdnsXspiWrite (Buffer, Offset, Size);
}
