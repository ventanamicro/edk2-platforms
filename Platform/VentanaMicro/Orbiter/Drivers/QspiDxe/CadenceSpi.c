/** @file

  Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
  Copyright (c) 2019, INTNel Corporation. All rights reserved.
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include "CadenceSpi.h"

#define LESS(a, b)   (((a) < (b)) ? (a) : (b))
#define MORE(a, b)   (((a) > (b)) ? (a) : (b))

STATIC UINT32  mQspiDeviceSize;
STATIC UINT32  mCadQspiCs;
STATIC UINT64  mCadBaseAddress;

STATIC
INTN CadQspiIdle (VOID)
{
  return (MmioRead32 (mCadBaseAddress + CAD_QSPI_CFG)
            & CAD_QSPI_CFG_IDLE) >> 31;
}

STATIC
INTN CadQspiSetBaudrateDiv (UINT32 Div)
{
  UINT32 Val;
  if (Div > 0xf) {
    return CAD_INVALID;
  }

  Val = MmioRead32 (mCadBaseAddress + CAD_QSPI_CFG);
  Val &= CAD_QSPI_CFG_BAUDDIV_MSK;
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_CFG, Val | CAD_QSPI_CFG_BAUDDIV(Div));

  return 0;
}

STATIC
INTN CadQspiConfigureDevSize (UINT32 AddrBytes,
    UINT32 BytesPerDev, UINT32 BytesPerBlock)
{

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_DEVSZ,
                CAD_QSPI_DEVSZ_ADDR_BYTES(AddrBytes) |
                CAD_QSPI_DEVSZ_BYTES_PER_PAGE(BytesPerDev) |
                CAD_QSPI_DEVSZ_BYTES_PER_BLOCK(BytesPerBlock));

  return 0;
}

STATIC
INTN CadQspiSetReadConfig (
    UINT32 Opcode, UINT32 InstrType,
    UINT32 AddrType, UINT32 DataType,
    UINT32 ModeBit, UINT32 DummyClkCycle)
{
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_DEVRD,
                CAD_QSPI_DEV_OPCODE(Opcode) |
                CAD_QSPI_DEV_INST_TYPE(InstrType) |
                CAD_QSPI_DEV_ADDR_TYPE(AddrType) |
                CAD_QSPI_DEV_DATA_TYPE(DataType) |
                CAD_QSPI_DEV_MODE_BIT(ModeBit) |
                CAD_QSPI_DEV_DUMMY_CLK_CYCLE(DummyClkCycle));

  return 0;
}

STATIC
INTN CadQspiSetWriteConfig (
    UINT32 Opcode, UINT32 AddrType,
    UINT32 DataType, UINT32 DummyClkCycle)
{
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_DEVWR,
                CAD_QSPI_DEV_OPCODE(Opcode) |
                CAD_QSPI_DEV_ADDR_TYPE(AddrType) |
                CAD_QSPI_DEV_DATA_TYPE(DataType) |
                CAD_QSPI_DEV_DUMMY_CLK_CYCLE(DummyClkCycle));

  return 0;
}

STATIC
INTN CadQspiTimingConfig(
    UINT32 Clkphase, UINT32 Clkpol, UINT32 Csda,
    UINT32 Csdads, UINT32 Cseot, UINT32 Cssot,
    UINT32 Rddatacap)
{
  UINT32 Cfg = MmioRead32 (mCadBaseAddress + CAD_QSPI_CFG);

  Cfg &= CAD_QSPI_CFG_SELCLKPHASE_CLR_MSK & CAD_QSPI_CFG_SELCLKPOL_CLR_MSK;
  Cfg |= CAD_QSPI_SELCLKPHASE(Clkphase) | CAD_QSPI_SELCLKPOL(Clkpol);

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_CFG, Cfg);
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_DELAY,
                CAD_QSPI_DELAY_CSSOT(Cssot) | CAD_QSPI_DELAY_CSEOT(Cseot) |
                CAD_QSPI_DELAY_CSDADS(Csdads) | CAD_QSPI_DELAY_CSDA(Csda));

  return 0;
}

STATIC
INTN CadQspiStigCmdHelper(INTN Cs, UINT32 Cmd)
{
  UINT32 Count = 0;
  UINT32 Reg;

  /* chip select */
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_CFG,
                (MmioRead32 (mCadBaseAddress + CAD_QSPI_CFG)
                & CAD_QSPI_CFG_CS_MSK) | CAD_QSPI_CFG_CS(Cs));

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_FLASHCMD, Cmd);
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_FLASHCMD,
                Cmd | CAD_QSPI_FLASHCMD_EXECUTE);

  do {
    Reg = MmioRead32 (mCadBaseAddress +
                        CAD_QSPI_FLASHCMD);
    if (!(Reg & CAD_QSPI_FLASHCMD_EXECUTE_STAT)) {
      break;
    }
    Count++;
  } while (Count < CAD_QSPI_COMMAND_TIMEOUT);

  if (Count >= CAD_QSPI_COMMAND_TIMEOUT) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Error sending QSPI command %x, timed out\n",
        Cmd));
    return CAD_QSPI_ERROR;
  }

  return 0;
}

STATIC
INTN CadQspiStigCmd (UINT32 Opcode, UINT32 Dummy)
{
  if (Dummy > ((1 << CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES_MAX) - 1)) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Faulty Dummy bytes\n"));
    return -1;
  }

  return CadQspiStigCmdHelper (mCadQspiCs,
              CAD_QSPI_FLASHCMD_OPCODE(Opcode) |
              CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES(Dummy));
}

STATIC
INTN CadQspiStigReadCmd (
    UINT32 Opcode, UINT32 Dummy,
    UINT32 NumBytes, UINT32 *Output)
{
  UINT32 Cmd;

  if (Dummy > ((1 << CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES_MAX) - 1)) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Faulty Dummy byes\n"));
    return -1;
  }

  if ((NumBytes > 8) || (NumBytes == 0)) {
    return -1;
  }

  Cmd = CAD_QSPI_FLASHCMD_OPCODE(Opcode) |
          CAD_QSPI_FLASHCMD_ENRDDATA(1) |
          CAD_QSPI_FLASHCMD_NUMRDDATABYTES(NumBytes - 1) |
          CAD_QSPI_FLASHCMD_ENCMDADDR(0) |
          CAD_QSPI_FLASHCMD_ENMODEBIT(0) |
          CAD_QSPI_FLASHCMD_NUMADDRBYTES(0) |
          CAD_QSPI_FLASHCMD_ENWRDATA(0) |
          CAD_QSPI_FLASHCMD_NUMWRDATABYTES(0) |
          CAD_QSPI_FLASHCMD_NUMDUMMYBYTES(Dummy);

  if (CadQspiStigCmdHelper (mCadQspiCs, Cmd)) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Failed to send stig cmd\n"));
    return -1;
  }

  Output[0] = MmioRead32 (mCadBaseAddress + CAD_QSPI_FLASHCMD_RDDATA0);

  if (NumBytes > 4) {
    Output[1] = MmioRead32 (mCadBaseAddress +
                      CAD_QSPI_FLASHCMD_RDDATA1);
  }

  return 0;
}

STATIC
INTN CadQspiStigWrCmd (
    UINT32 Opcode, UINT32 Dummy, 
    UINT32 NumBytes, UINT32 *Input)
{
  UINT32 Cmd;

  if (Dummy > ((1 << CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES_MAX) - 1)) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Faulty Dummy byes\n"));
    return -1;
  }

  if ((NumBytes > 8) || (NumBytes == 0)) {
    return -1;
  }

  Cmd = CAD_QSPI_FLASHCMD_OPCODE(Opcode) |
          CAD_QSPI_FLASHCMD_ENRDDATA(0) |
          CAD_QSPI_FLASHCMD_NUMRDDATABYTES(0) |
          CAD_QSPI_FLASHCMD_ENCMDADDR(0) |
          CAD_QSPI_FLASHCMD_ENMODEBIT(0) |
          CAD_QSPI_FLASHCMD_NUMADDRBYTES(0) |
          CAD_QSPI_FLASHCMD_ENWRDATA(1) |
          CAD_QSPI_FLASHCMD_NUMWRDATABYTES(NumBytes - 1) |
          CAD_QSPI_FLASHCMD_NUMDUMMYBYTES(Dummy);

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_FLASHCMD_WRDATA0, Input[0]);

  if (NumBytes > 4) {
    MmioWrite32 (mCadBaseAddress + CAD_QSPI_FLASHCMD_WRDATA1, Input[1]);
  }

  return CadQspiStigCmdHelper (mCadQspiCs, Cmd);
}

STATIC
INTN CadQspiStigAddrCmd (UINT32 Opcode, UINT32 Dummy, UINT32 Addr)
{
  UINT32 Cmd;

  if (Dummy > ((1 << CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES_MAX) - 1)) {
    return -1;
  }

  Cmd = CAD_QSPI_FLASHCMD_OPCODE(Opcode) |
          CAD_QSPI_FLASHCMD_NUMDUMMYBYTES(Dummy) |
          CAD_QSPI_FLASHCMD_ENCMDADDR(1) |
          CAD_QSPI_FLASHCMD_NUMADDRBYTES(2);

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_FLASHCMD_ADDR, Addr);

  return CadQspiStigCmdHelper (mCadQspiCs, Cmd);
}

STATIC
INTN CadQspiDeviceBankSelect (UINT32 Bank)
{
  INTN Status = 0;

  Status = CadQspiStigCmd (CAD_QSPI_STIG_OPCODE_WREN, 0);
  if (Status != 0) {
    return Status;
  }

  Status = CadQspiStigWrCmd (CAD_QSPI_STIG_OPCODE_WREN_EXT_REG,
      0, 1, &Bank);
  if (Status != 0) {
    return Status;
  }

  return CadQspiStigCmd (CAD_QSPI_STIG_OPCODE_WRDIS, 0);
}

STATIC
INTN CadQspiDeviceStatus (UINT32 *Status)
{
  return CadQspiStigReadCmd (CAD_QSPI_STIG_OPCODE_RDSR, 0, 1, Status);
}

STATIC
INTN CadQspiN25qEnable (VOID)
{
  CadQspiSetReadConfig (QSPI_FAST_READ, CAD_QSPI_INST_SINGLE,
                          CAD_QSPI_ADDR_FASTREAD, CAT_QSPI_ADDR_SINGLE_IO, 1,
                          0);
  CadQspiSetWriteConfig (QSPI_WRITE, 0, 0, 0);

  return 0;
}

STATIC
INTN CadQspiN25qWaitForProgramAndErase (INTN ProgramOnly)
{
  UINT32 Status, FlagSr;
  INTN Count = 0;

  while (Count < CAD_QSPI_COMMAND_TIMEOUT) {
    Status = CadQspiDeviceStatus (&Status);
    if (Status != 0) {
      DEBUG ((DEBUG_ERROR, "CAD SPI: Error getting device Status\n"));
      return -1;
    }
    if (!CAD_QSPI_STIG_SR_BUSY(Status)) {
      break;
    }
    Count++;
  }

  if (Count >= CAD_QSPI_COMMAND_TIMEOUT) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Timed out waiting for idle\n"));
    return -1;
  }

  Count = 0;

  while (Count < CAD_QSPI_COMMAND_TIMEOUT) {
    Status = CadQspiStigReadCmd (CAD_QSPI_STIG_OPCODE_RDFLGSR,
                0, 1, &FlagSr);
    if (Status != 0) {
      DEBUG ((DEBUG_ERROR, "CAD SPI: Error waiting program and erase.\n"));
      return Status;
    }

    if ((ProgramOnly &&
          CAD_QSPI_STIG_FLAGSR_PROGRAMREADY(FlagSr)) ||
          (!ProgramOnly &&
          CAD_QSPI_STIG_FLAGSR_ERASEREADY(FlagSr))) {
      break;
    }
  }

  if (Count >= CAD_QSPI_COMMAND_TIMEOUT) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Timed out waiting for program and erase\n"));
  }

  if ((ProgramOnly && CAD_QSPI_STIG_FLAGSR_PROGRAMERROR(FlagSr)) ||
      (!ProgramOnly &&
      CAD_QSPI_STIG_FLAGSR_ERASEERROR(FlagSr))) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Error programming/erasing flash\n"));
    CadQspiStigCmd (CAD_QSPI_STIG_OPCODE_CLFSR, 0);
    return -1;
  }

  return 0;
}

STATIC
INTN CadQspiIndirectReadStartBank (UINT32 FlashAddr, UINT32 NumBytes)
{
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_INDRDSTADDR, FlashAddr);
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_INDRDCNT, NumBytes);
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_INDRD, CAD_QSPI_INDRD_START | CAD_QSPI_INDRD_IND_OPS_DONE);

  return 0;
}

STATIC
INTN CadQspiIndirectWriteStartBank (UINT32 FlashAddr,
          UINT32 NumBytes)
{
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_INDWRSTADDR, FlashAddr);
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_INDWRCNT, NumBytes);
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_INDWR, CAD_QSPI_INDWR_START | CAD_QSPI_INDWR_INDDONE);

  return 0;
}

STATIC
INTN CadQspiIndirectWriteFinish (VOID)
{
  return CadQspiN25qWaitForProgramAndErase (1);
}

STATIC
INTN CadQspiEnable (VOID)
{
  INTN Status;
  UINT32 Val;

  Val = MmioRead32 (mCadBaseAddress + CAD_QSPI_CFG);
  Val |= CAD_QSPI_CFG_ENABLE;
  MmioWrite32 (mCadBaseAddress + CAD_QSPI_CFG, Val);

  Status = CadQspiN25qEnable ();
  if (Status != 0) {
    return Status;
  }

  return 0;
}

STATIC
INTN CadQspiEnableSubsectorBank (UINT32 Addr)
{
  INTN Status = 0;

  Status = CadQspiStigCmd (CAD_QSPI_STIG_OPCODE_WREN, 0);
  if (Status != 0) {
    return Status;
  }

  Status = CadQspiStigAddrCmd (CAD_QSPI_STIG_OPCODE_SUBSEC_ERASE, 0,
              Addr);
  if (Status != 0) {
    return Status;
  }

  return CadQspiN25qWaitForProgramAndErase (0);
}

STATIC
INTN CadQspiEraseSubsector (UINT32 Addr)
{
  INTN Status = 0;

  Status = CadQspiDeviceBankSelect (Addr >> 24);
  if (Status != 0) {
    return Status;
  }

  return CadQspiEnableSubsectorBank (Addr);
}

/* NOT USED */
/*STATIC
INTN CadQspiEraseSector (UINT32 Addr)
{
  INTN Status = 0;

  Status = CadQspiDeviceBankSelect (Addr >> 24);
  if (Status != 0) {
    return Status;
  }

  Status = CadQspiStigCmd (CAD_QSPI_STIG_OPCODE_WREN, 0);
  if (Status != 0) {
    return Status;
  }

  Status = CadQspiStigAddrCmd (CAD_QSPI_STIG_OPCODE_SEC_ERASE, 0,
          Addr);
  if (Status != 0) {
    return Status;
  }

  return CadQspiN25qWaitForProgramAndErase(0);
}*/

STATIC
VOID CadQspiCalibration (UINT32 DevClk, UINT32 QspiClkMhz)
{
  INTN Status;
  UINT32 DevSclkMhz = 27; /*min value to get biggest 0xF div factor*/
  UINT32 DataCapDelay;
  UINT32 SampleRdid;
  UINT32 Rdid;
  UINT32 DivActual;
  UINT32 DivBits;
  INTN FirstPass, LastPass;

  /*1.  Set divider to bigger value (slowest SCLK)
   *2.  RDID and save the value
   */
  DivActual = (QspiClkMhz + (DevSclkMhz - 1)) / DevSclkMhz;
  DivBits = (((DivActual + 1) / 2) - 1);
  Status = CadQspiSetBaudrateDiv (0xf);

  Status = CadQspiStigReadCmd (CAD_QSPI_STIG_OPCODE_RDID,
                0, 3, &SampleRdid);
  if (Status != 0) {
    return;
  }

  /*3. Set divider to the intended frequency
   *4.  Set the read delay = 0
   *5.  RDID and check whether the value is same as item 2
   *6.  Increase read delay and compared the value against item 2
   *7.  Find the range of read delay that have same as
   *    item 2 and divide it to 2
   */
  DivActual = (QspiClkMhz + (DevClk - 1)) / DevClk;
  DivBits = (((DivActual + 1) / 2) - 1);
  Status = CadQspiSetBaudrateDiv (DivBits);
  if (Status != 0) {
    return;
  }

  DataCapDelay = 0;
  FirstPass = -1;
  LastPass = -1;

  do {
    if (Status != 0) {
      break;
    }
    Status = CadQspiStigReadCmd (CAD_QSPI_STIG_OPCODE_RDID, 0,
            3, &Rdid);
    if (Status != 0) {
      break;
    }
    if (Rdid == SampleRdid) {
      if (FirstPass == -1) {
        FirstPass = DataCapDelay;
      } else {
        LastPass = DataCapDelay;
      }
    }

    DataCapDelay++;

    MmioWrite32 (mCadBaseAddress + CAD_QSPI_RDDATACAP,
          CAD_QSPI_RDDATACAP_BYP(1) |
          CAD_QSPI_RDDATACAP_DELAY(DataCapDelay));

  } while (DataCapDelay < 0x10);

  if (FirstPass > 0) {
    DataCapDelay = FirstPass + (FirstPass - LastPass) / 2;
  }

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_RDDATACAP,
                CAD_QSPI_RDDATACAP_BYP(1) |
                CAD_QSPI_RDDATACAP_DELAY(DataCapDelay));
  CadQspiStigReadCmd (CAD_QSPI_STIG_OPCODE_RDID, 0, 3, &Rdid);
}

STATIC
INTN CadQspiIntDisable (UINT32 Mask)
{
  if (CadQspiIdle() == 0) {
    return -1;
  }

  if ((CAD_QSPI_INT_STATUS_ALL & Mask) == 0) {
    return -1;
  }

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_IRQMSK, Mask);
  return 0;
}

STATIC
INTN CadQspiIndirectPageBoundWrite (UINT32 Offset,
    UINT8 *Buffer, UINT32 Len)
{
  INTN Status = 0, Idx;
  UINT32 WriteCount, WriteCapacity, *WriteData, Space,
    WriteFillLevel, SramPartition;

  Status = CadQspiIndirectWriteStartBank (Offset, Len);
  if (Status != 0) {
    return Status;
  }

  WriteCount = 0;
  SramPartition = CAD_QSPI_SRAMPART_ADDR(MmioRead32 (mCadBaseAddress + CAD_QSPI_SRAMPART));
  WriteCapacity = (UINT32) CAD_QSPI_SRAM_FIFO_ENTRY_COUNT - SramPartition;

  while (WriteCount < Len) {
    WriteFillLevel = CAD_QSPI_SRAMFILL_INDWRPART(
                                MmioRead32 (mCadBaseAddress + CAD_QSPI_SRAMFILL));
    Space = LESS(WriteCapacity - WriteFillLevel,
        (Len - WriteCount) / sizeof(UINT32));
    WriteData = (UINT32 *)(Buffer + WriteCount);
    for (Idx = 0; Idx < Space; ++Idx) {
      MmioWrite32 (CAD_QSPIDATA_OFST, *WriteData++);
    }

    WriteCount += Space * sizeof(UINT32);
  }
  return CadQspiIndirectWriteFinish ();
}

STATIC
INTN CadQspiReadBank (UINT8 *Buffer, UINT32 Offset, UINT32 Size)
{
  INTN Status;
  UINT32 ReadCount = 0, *ReadData;
  INTN Level = 1, Count = 0, Idx;

  Status = CadQspiIndirectReadStartBank (Offset, Size);

  if (Status != 0) {
    return Status;
  }

  while (ReadCount < Size) {
    do {
      Level = CAD_QSPI_SRAMFILL_INDRDPART(
                        MmioRead32 (mCadBaseAddress + CAD_QSPI_SRAMFILL));
      ReadData = (UINT32 *)(Buffer + ReadCount);
      for (Idx = 0; Idx < Level; ++Idx) {
        *ReadData++ = MmioRead32 (CAD_QSPIDATA_OFST);
      }

      ReadCount += Level * sizeof(UINT32);
      Count++;
    } while (Level > 0);
  }

  return 0;
}

STATIC
INTN CadQspiWriteBank (UINT32 Offset, UINT8 *Buffer, UINT32 Size)
{
  INTN Status = 0;
  UINT32 PageOffset  = Offset & (CAD_QSPI_PAGE_SIZE - 1);
  UINT32 WriteSize = LESS(Size, CAD_QSPI_PAGE_SIZE - PageOffset);

  while (Size) {
    Status = CadQspiIndirectPageBoundWrite (Offset, Buffer, WriteSize);
    if (Status != 0) {
      break;
    }

    Offset  += WriteSize;
    Buffer  += WriteSize;
    Size -= WriteSize;
    WriteSize = LESS(Size, CAD_QSPI_PAGE_SIZE);
  }

  return Status;
}

VOID CadQspiUpdateBaseAddress (UINT64 BaseAddress)
{
  mCadBaseAddress = BaseAddress;
}

INTN CadQspiInit (
        UINT64 BaseAddress, UINT32 ClkPhase,
        UINT32 ClkPol, UINT32 Csda, UINT32 Csdads,
        UINT32 Cseot, UINT32 Cssot, UINT32 Rddatacap)
{
  INTN Status = 0;
  UINT32 Rdid = 0;
  UINT32 CapCode;
  UINT32 DecodedCap;

  DEBUG ((DEBUG_INFO, "CAD SPI: Initializing Qspi\n"));

  mCadBaseAddress = BaseAddress;
  mCadQspiCs = 0;

  if (CadQspiIdle () == 0) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: device not idle\n"));
    return -1;
  }


  Status = CadQspiTimingConfig (ClkPhase, ClkPol, Csda, Csdads,
                Cseot, Cssot, Rddatacap);

  if (Status != 0) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: config set timing failure\n"));
    return Status;
  }

  MmioWrite32 (mCadBaseAddress + CAD_QSPI_REMAPADDR,
      CAD_QSPI_REMAPADDR_VALUE_SET(0));

  Status = CadQspiIntDisable (CAD_QSPI_INT_STATUS_ALL);
  if (Status != 0) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: failed disable\n"));
    return Status;
  }

  CadQspiSetBaudrateDiv (0xf);
  Status = CadQspiEnable ();
  if (Status != 0) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: failed enable\n"));
    return Status;
  }

  CadQspiCalibration (FixedPcdGet32 (PcdQspiDevClock) / 1000000, FixedPcdGet32 (PcdQspiReferenceClock) / 1000000);

  Status = CadQspiStigReadCmd (CAD_QSPI_STIG_OPCODE_RDID, 0, 3,
                                  &Rdid);

  if (Status != 0) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Error reading RDID\n"));
    return Status;
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

  CapCode = CAD_QSPI_STIG_RDID_CAPACITYID(Rdid);

  if (!(((CapCode >> 4) > 0x9) || ((CapCode & 0xf) > 0x9))) {
    DecodedCap = ((CapCode >> 4) * 10) +
                  (CapCode & 0xf);
    mQspiDeviceSize = 1 << (DecodedCap + 6);
  } else {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Invalid CapacityID enCountered: 0x%02x\n",
            CapCode));
    return -1;
  }

  CadQspiConfigureDevSize (CONFIG_QSPI_ADDR_BYTES,
        CONFIG_QSPI_BYTES_PER_DEV,
        CONFIG_BYTES_PER_BLOCK);

  DEBUG ((DEBUG_INFO, "CAD SPI: Flash Size: 0x%X\n\n", mQspiDeviceSize));

  return Status;
}

INTN CadQspiRead (VOID *Buffer, UINT32 Offset, UINT32 Size)
{
  UINT32 BankCount, BankAddr, BankOffset, CopyLen;
  UINT8 *ReadData;
  INTN Idx, Status;

  Status = 0;

  if ((Offset >= mQspiDeviceSize) ||
      (Offset + Size - 1 >= mQspiDeviceSize) ||
      (Size == 0)) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Invalid read parameter\n"));
    return -1;
  }

  if (CAD_QSPI_INDRD_RD_STAT(MmioRead32 (mCadBaseAddress + CAD_QSPI_INDRD))) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: Read in progress\n"));
    return -1;
  }

  /*
   * BankCount : Number of Bank(s) affected, including partial Banks.
   * BankAddr  : Aligned Address of the first Bank,
   *    including partial Bank.
   * Bank_ofst  : The Offset of the Bank to read.
   *    Only used when reading the first Bank.
   */
  BankCount = CAD_QSPI_BANK_ADDR(Offset + Size - 1) -
                CAD_QSPI_BANK_ADDR(Offset) + 1;
  BankAddr  = Offset & CAD_QSPI_BANK_ADDR_MSK;
  BankOffset  = Offset & (CAD_QSPI_BANK_SIZE - 1);

  ReadData = (UINT8 *)Buffer;

  CopyLen = LESS(Size, CAD_QSPI_BANK_SIZE - BankOffset);

  for (Idx = 0; Idx < BankCount; ++Idx) {
    Status = CadQspiDeviceBankSelect (CAD_QSPI_BANK_ADDR(BankAddr));
    if (Status != 0) {
      break;
    }
    Status = CadQspiReadBank (ReadData, BankOffset, CopyLen);
    if (Status != 0) {
      break;
    }

    BankAddr += CAD_QSPI_BANK_SIZE;
    ReadData += CopyLen;
    Size -= CopyLen;
    BankOffset = 0;
    CopyLen = LESS(Size, CAD_QSPI_BANK_SIZE);
  }

  return Status;
}

INTN CadQspiErase (UINT32 Offset, UINT32 Size)
{
  INTN Status = 0;
  UINT32 SubsectorOffset  = Offset & (CAD_QSPI_SUBSECTOR_SIZE - 1);
  UINT32 EraseSize = LESS(Size, CAD_QSPI_SUBSECTOR_SIZE - SubsectorOffset);

  while (Size) {
    Status = CadQspiEraseSubsector (Offset);
    if (Status != 0) {
      break;
    }

    Offset  += EraseSize;
    Size -= EraseSize;
    EraseSize = LESS(Size, CAD_QSPI_SUBSECTOR_SIZE);
  }

  return Status;
}

INTN CadQspiWrite (VOID *Buffer, UINT32 Offset, UINT32 Size)
{
  INTN Status, Idx;
  UINT32 BankCount, BankAddr, BankOffset, CopyLen;
  UINT8 *WriteData;

  Status = 0;

  if ((Offset >= mQspiDeviceSize) ||
      (Offset + Size - 1 >= mQspiDeviceSize) ||
      (Size == 0)) {
    return -2;
  }

  if (CAD_QSPI_INDWR_RDSTAT(MmioRead32 (mCadBaseAddress + CAD_QSPI_INDWR))) {
    DEBUG ((DEBUG_ERROR, "CAD SPI: QSPI Error: Write in progress\n"));
    return -1;
  }

  BankCount = CAD_QSPI_BANK_ADDR(Offset + Size - 1) -
                CAD_QSPI_BANK_ADDR(Offset) + 1;
  BankAddr = Offset & CAD_QSPI_BANK_ADDR_MSK;
  BankOffset = Offset & (CAD_QSPI_BANK_SIZE - 1);

  WriteData = Buffer;

  CopyLen = LESS(Size, CAD_QSPI_BANK_SIZE - BankOffset);

  for (Idx = 0; Idx < BankCount; ++Idx) {
    Status = CadQspiDeviceBankSelect (CAD_QSPI_BANK_ADDR(BankAddr));
    if (Status != 0) {
      break;
    }

    Status = CadQspiWriteBank (BankOffset, WriteData, CopyLen);
    if (Status != 0) {
      break;
    }

    BankAddr += CAD_QSPI_BANK_SIZE;
    WriteData += CopyLen;
    Size -= CopyLen;
    BankOffset = 0;

    CopyLen = LESS(Size, CAD_QSPI_BANK_SIZE);
  }

  return Status;
}

INTN CadQspiUpdate (VOID *Buffer, UINT32 Offset, UINT32 Size)
{
  INTN Status;

  Status = CadQspiErase (Offset, Size);
  if (Status != 0) {
    return Status;
  }

  return CadQspiWrite (Buffer, Offset, Size);
}

VOID CadQspiReset (VOID)
{
  CadQspiStigCmd (CAD_QSPI_STIG_OPCODE_RESET_EN, 0);
  CadQspiStigCmd (CAD_QSPI_STIG_OPCODE_RESET_MEM, 0);
}
