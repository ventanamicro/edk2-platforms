/** @file

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CADENCE_XSPI_H
#define CADENCE_XSPI_H

#ifndef BIT
#define BIT(nr)                                   (1 << (nr))
#endif

#ifndef BITS_PER_LONG
#define BITS_PER_LONG                             64
#endif

#ifndef GENMASK
#define GENMASK(h, l) \
              (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif

#ifndef FIELD_GET
#define FIELD_GET(_mask, _reg)						\
	({								\
		(typeof(_mask))(((_reg) & (_mask)) >> (__builtin_ffsll(_mask) - 1));	\
	})
#endif

#ifndef FIELD_PREP
#define FIELD_PREP(_mask, _val)						\
	({								\
		((typeof(_mask))(_val) << (__builtin_ffsll(_mask) - 1)) & (_mask);	\
	})
#endif

#define CDNS_XSPI_MAGIC_NUM_VALUE                 0x6522
#define CDNS_XSPI_MAX_BANKS                       8

/*
 * Note: below are additional auxiliary registers to
 * configure XSPI controller pin-strap settings
 */

/* PHY DQ timing register */
#define CDNS_XSPI_CCP_PHY_DQ_TIMING                0x0000

/* PHY DQS timing register */
#define CDNS_XSPI_CCP_PHY_DQS_TIMING               0x0004

/* PHY gate loopback control register */
#define CDNS_XSPI_CCP_PHY_GATE_LPBCK_CTRL          0x0008

/* PHY DLL slave control register */
#define CDNS_XSPI_CCP_PHY_DLL_SLAVE_CTRL           0x0010

/* DLL PHY control register */
#define CDNS_XSPI_DLL_PHY_CTRL                     0x1034

/* Command registers */
#define CDNS_XSPI_CMD_REG_0                        0x0000
#define CDNS_XSPI_CMD_REG_1                        0x0004
#define CDNS_XSPI_CMD_REG_2                        0x0008
#define CDNS_XSPI_CMD_REG_3                        0x000C
#define CDNS_XSPI_CMD_REG_4                        0x0010
#define CDNS_XSPI_CMD_REG_5                        0x0014

/* Command status registers */
#define CDNS_XSPI_CMD_STATUS_REG                   0x0044

/* Controller status register */
#define CDNS_XSPI_CTRL_STATUS_REG                  0x0100
#define  CDNS_XSPI_INIT_COMPLETED                  BIT(16)
#define  CDNS_XSPI_INIT_LEGACY                     BIT(9)
#define  CDNS_XSPI_INIT_FAIL                       BIT(8)
#define  CDNS_XSPI_CTRL_BUSY                       BIT(7)

/* Controller interrupt status register */
#define CDNS_XSPI_INTR_STATUS_REG                  0x0110
#define  CDNS_XSPI_STIG_DONE                       BIT(23)
#define  CDNS_XSPI_SDMA_ERROR                      BIT(22)
#define  CDNS_XSPI_SDMA_TRIGGER                    BIT(21)
#define  CDNS_XSPI_CMD_IGNRD_EN                    BIT(20)
#define  CDNS_XSPI_DDMA_TERR_EN                    BIT(18)
#define  CDNS_XSPI_CDMA_TREE_EN                    BIT(17)
#define  CDNS_XSPI_CTRL_IDLE_EN                    BIT(16)

#define CDNS_XSPI_TRD_COMP_INTR_STATUS             0x0120
#define CDNS_XSPI_TRD_ERR_INTR_STATUS              0x0130
#define CDNS_XSPI_TRD_ERR_INTR_EN                  0x0134

/* Controller interrupt enable register */
#define CDNS_XSPI_INTR_ENABLE_REG                  0x0114
#define  CDNS_XSPI_INTR_EN                         BIT(31)
#define  CDNS_XSPI_STIG_DONE_EN                    BIT(23)
#define  CDNS_XSPI_SDMA_ERROR_EN                   BIT(22)
#define  CDNS_XSPI_SDMA_TRIGGER_EN                 BIT(21)

#define CDNS_XSPI_INTR_MASK      (CDNS_XSPI_INTR_EN | \
                                  CDNS_XSPI_STIG_DONE_EN  | \
                                  CDNS_XSPI_SDMA_ERROR_EN | \
                                  CDNS_XSPI_SDMA_TRIGGER_EN)

/* Controller config register */
#define CDNS_XSPI_CTRL_CONFIG_REG                  0x0230
#define  CDNS_XSPI_CTRL_WORK_MODE                  GENMASK(6, 5)
#define  CDNS_XSPI_WORK_MODE_DIRECT                0
#define  CDNS_XSPI_WORK_MODE_STIG                  1
#define  CDNS_XSPI_WORK_MODE_ACMD                  3

/* SDMA trigger transaction registers */
#define CDNS_XSPI_SDMA_SIZE_REG                    0x0240
#define CDNS_XSPI_SDMA_TRD_INFO_REG                0x0244
#define  CDNS_XSPI_SDMA_DIR                        BIT(8)
#define CDNS_XSPI_SDMA_ADDR0                       0x024c
#define CDNS_XSPI_SDMA_ADDR1                       0x0250

/* Controller features register */
#define CDNS_XSPI_CTRL_FEATURES_REG                0x0F04
#define  CDNS_XSPI_NUM_BANKS                       GENMASK(25, 24)
#define  CDNS_XSPI_DMA_DATA_WIDTH                  BIT(21)
#define  CDNS_XSPI_NUM_THREADS                     GENMASK(3, 0)

/* Controller version register */
#define CDNS_XSPI_CTRL_VERSION_REG                 0x0F00
#define  CDNS_XSPI_MAGIC_NUM                       GENMASK(31, 16)
#define  CDNS_XSPI_CTRL_REV                        GENMASK(7, 0)

/* STIG Profile 1.0 instruction fields (split into registers) */
#define CDNS_XSPI_CMD_INSTR_TYPE                   GENMASK(6, 0)
#define CDNS_XSPI_CMD_P1_R1_ADDR0                  GENMASK(31, 24)
#define CDNS_XSPI_CMD_P1_R2_ADDR1                  GENMASK(7, 0)
#define CDNS_XSPI_CMD_P1_R2_ADDR2                  GENMASK(15, 8)
#define CDNS_XSPI_CMD_P1_R2_ADDR3                  GENMASK(23, 16)
#define CDNS_XSPI_CMD_P1_R2_ADDR4                  GENMASK(31, 24)
#define CDNS_XSPI_CMD_P1_R3_ADDR5                  GENMASK(7, 0)
#define CDNS_XSPI_CMD_P1_R3_CMD                    GENMASK(23, 16)
#define CDNS_XSPI_CMD_P1_R3_NUM_ADDR_BYTES         GENMASK(30, 28)
#define CDNS_XSPI_CMD_P1_R4_ADDR_IOS               GENMASK(1, 0)
#define CDNS_XSPI_CMD_P1_R4_CMD_IOS                GENMASK(9, 8)
#define CDNS_XSPI_CMD_P1_R4_BANK                   GENMASK(14, 12)

/* STIG data sequence instruction fields (split into registers) */
#define CDNS_XSPI_CMD_DSEQ_R2_DCNT_L               GENMASK(31, 16)
#define CDNS_XSPI_CMD_DSEQ_R3_DCNT_H               GENMASK(15, 0)
#define CDNS_XSPI_CMD_DSEQ_R3_NUM_OF_DUMMY         GENMASK(25, 20)
#define CDNS_XSPI_CMD_DSEQ_R4_BANK                 GENMASK(14, 12)
#define CDNS_XSPI_CMD_DSEQ_R4_DATA_IOS             GENMASK(9, 8)
#define CDNS_XSPI_CMD_DSEQ_R4_DIR                  BIT(4)

/* STIG command status fields */
#define CDNS_XSPI_CMD_STATUS_COMPLETED             BIT(15)
#define CDNS_XSPI_CMD_STATUS_FAILED                BIT(14)
#define CDNS_XSPI_CMD_STATUS_DQS_ERROR             BIT(3)
#define CDNS_XSPI_CMD_STATUS_CRC_ERROR             BIT(2)
#define CDNS_XSPI_CMD_STATUS_BUS_ERROR             BIT(1)
#define CDNS_XSPI_CMD_STATUS_INV_SEQ_ERROR         BIT(0)

#define CDNS_XSPI_STIG_DONE_FLAG                   BIT(0)
#define CDNS_XSPI_TRD_STATUS                       0x0104

/* Helper macros for filling command registers */
#define CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_1(Addr, DataPhase) ( \
	FIELD_PREP(CDNS_XSPI_CMD_INSTR_TYPE, (DataPhase) ? \
		CDNS_XSPI_STIG_INSTR_TYPE_1 : CDNS_XSPI_STIG_INSTR_TYPE_0) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R1_ADDR0, (Addr) & 0xff))

#define CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_2(Addr) ( \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R2_ADDR1, ((Addr) >> 8)  & 0xFF) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R2_ADDR2, ((Addr) >> 16) & 0xFF) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R2_ADDR3, ((Addr) >> 24) & 0xFF) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R2_ADDR4, ((Addr) >> 32) & 0xFF))

#define CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_3(Addr, Opcode, AddrNumBytes) ( \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R3_ADDR5, ((Addr) >> 40) & 0xFF) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R3_CMD, (Opcode)) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R3_NUM_ADDR_BYTES, (AddrNumBytes)))

#define CDNS_XSPI_CMD_FLD_P1_INSTR_CMD_4(Log2AddrBusWidth, Log2CmdBusWidth, Chipsel) ( \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R4_ADDR_IOS, (Log2AddrBusWidth)) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R4_CMD_IOS, (Log2CmdBusWidth)) | \
	FIELD_PREP(CDNS_XSPI_CMD_P1_R4_BANK, Chipsel))

#define CDNS_XSPI_CMD_FLD_DSEQ_CMD_1() \
	FIELD_PREP(CDNS_XSPI_CMD_INSTR_TYPE, CDNS_XSPI_STIG_INSTR_TYPE_DATA_SEQ)

#define CDNS_XSPI_CMD_FLD_DSEQ_CMD_2(DataNumBytes) \
	FIELD_PREP(CDNS_XSPI_CMD_DSEQ_R2_DCNT_L, (DataNumBytes) & 0xFFFF)

#define CDNS_XSPI_CMD_FLD_DSEQ_CMD_3(DataNumBytes, DummyNumBytes) ( \
	FIELD_PREP(CDNS_XSPI_CMD_DSEQ_R3_DCNT_H, \
		((DataNumBytes) >> 16) & 0xffff) | \
	FIELD_PREP(CDNS_XSPI_CMD_DSEQ_R3_NUM_OF_DUMMY, (DummyNumBytes) * 8))

#define CDNS_XSPI_CMD_FLD_DSEQ_CMD_4(Log2DataBusWidth, Read, Chipsel) ( \
	FIELD_PREP(CDNS_XSPI_CMD_DSEQ_R4_BANK, Chipsel) | \
	FIELD_PREP(CDNS_XSPI_CMD_DSEQ_R4_DATA_IOS, \
		(Log2DataBusWidth)) | \
	FIELD_PREP(CDNS_XSPI_CMD_DSEQ_R4_DIR, \
		((Read)) ? \
		CDNS_XSPI_STIG_CMD_DIR_READ : CDNS_XSPI_STIG_CMD_DIR_WRITE))

enum CdnsXspiStigInstrType {
	CDNS_XSPI_STIG_INSTR_TYPE_0,
	CDNS_XSPI_STIG_INSTR_TYPE_1,
	CDNS_XSPI_STIG_INSTR_TYPE_DATA_SEQ = 127,
};

enum CdnsXspiSdmaDir {
	CDNS_XSPI_SDMA_DIR_READ,
	CDNS_XSPI_SDMA_DIR_WRITE,
};

enum CdnsXspiStigCmdDir {
	CDNS_XSPI_STIG_CMD_DIR_READ,
	CDNS_XSPI_STIG_CMD_DIR_WRITE,
};

enum CdnsXspiStigIoMode {
	CDNS_XSPI_STIG_IO_MODE_SINGLE,
	CDNS_XSPI_STIG_IO_MODE_DUAL,
	CDNS_XSPI_STIG_IO_MODE_QUAD,
	CDNS_XSPI_STIG_IO_MODE_OCTAL,
};

VOID CdnsXspiUpdateBaseAddress (IN UINT64 BaseAddress);
EFI_STATUS CdnsXspiInit (IN UINT64 BaseAddress);
EFI_STATUS CdnsXspiErase (UINT32 Offset, UINT32 Size);
EFI_STATUS CdnsXspiWrite (UINT8 *Buffer, UINT32 Offset, UINT32 Size);
EFI_STATUS CdnsXspiRead (UINT8 *Buffer, UINT32 Offset, UINT32 Size);
EFI_STATUS CdnsXspiUpdate (UINT8 *Buffer, UINT32 Offset, UINT32 Size);

#endif /* CADENCE_XSPI_H */
