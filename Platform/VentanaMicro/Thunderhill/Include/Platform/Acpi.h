/** @file
  ACPI table definition of THUNDERHILL platform.

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_H_
#define ACPI_H_

#define THUNDERHILL_ACPI_HEADER(Signature, Type, Revision) {              \
    Signature,                          /* UINT32  Signature */       \
    sizeof (Type),                      /* UINT32  Length */          \
    Revision,                           /* UINT8   Revision */        \
    0,                                  /* UINT8   Checksum */        \
    { 'V', 'N', 'T', 'A', 'N', 'A' },   /* UINT8   OemId[6] */        \
    0x20414E41544E4556ULL,              /* UINT64  OemTableId */      \
    1,                                  /* UINT32  OemRevision */     \
    0x4E544E56,                         /* UINT32  CreatorId */       \
    1                                   /* UINT32  CreatorRevision */ \
  }

#define EFI_ACPI_6_5_RISCV_RINTC                    0x18
#define EFI_ACPI_6_5_RISCV_IMSIC                    0x19
#define EFI_ACPI_6_5_RISCV_APLIC                    0x1A
#define EFI_ACPI_6_5_RISCV_PLIC                     0x1B

#pragma pack (1)

///
/// RISC-V Hart Local Interrupt Controller
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     Reserved;
  UINT32    Flags;
  UINT64    HardId;
  UINT32    ACPIProcessorId;
  UINT32    ExternalIntCtrlId;
  UINT64    ImsicBaseAddress;
  UINT32    ImsicBaseSize;
} EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE;

#define EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE_VERSION  1
#define EFI_ACPI_6_5_RISCV_RINTC_FLAG_ENABLE        1

///
/// RISC-V Incoming MSI Controller
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     Reserved;
  UINT32    Flags;
  UINT16    NumIdentities;
  UINT16    NumGuestIdentities;
  UINT8     GuestIndexBit;
  UINT8     HardIndexBit;
  UINT8     GroupIndexBit;
  UINT8     GroupIndexShift;
} EFI_ACPI_6_5_RISCV_IMSIC_STRUCTURE;

#define EFI_ACPI_6_5_RISCV_IMSIC_STRUCTURE_VERSION  1

///
/// RISC-V Advanced Platform Level Interrupt Controller (APLIC)
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     Id;
  UINT32    Flags;
  UINT64    Hid;
  UINT16    NumIDCs;
  UINT16    NumSources;
  UINT32    GlobalIntBase;
  UINT64    BaseAddress;
  UINT32    BaseSize;
} EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE;

#define EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE_VERSION 1
///
/// RISC-V Platform Level Interrupt Controller (PLIC)
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     Reserved;
  UINT32    Id;
  UINT64    Hid;
  UINT16    NumSources;
  UINT16    MaxPriority;
  UINT32    Flags;
  UINT32    BaseSize;
  UINT64    BaseAddress;
  UINT32    Gsiv;
} EFI_ACPI_6_5_RISCV_PLIC_STRUCTURE;


#define EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE        0
#define EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE        1
#define EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE  65535

///
/// RISC-V Hart RHCT ISA Node Structure
///
typedef struct {
  UINT16    Type;
  UINT16    Length;
  UINT16    Revision;
  UINT16    IsaLength;
  UINT32    IsaString[0];
} EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE;

#define EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE_VERSION 1

///
/// RISC-V Hart RHCT CMO Node Structure
///
typedef struct {
  UINT16    Type;
  UINT16    Length;
  UINT16    Revision;
  UINT8     Reserved;
  UINT8     CbomBlockSize;
  UINT8     CbopBlockSize;
  UINT8     CbozBlockSize;
} EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE;

#define EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE_VERSION 1

///
/// RISC-V Hart RHCT Hart Info Structure
///
typedef struct {
  UINT16    Type;
  UINT16    Length;
  UINT16    Revision;
  UINT16    NumOffsets;
  UINT32    ACPICpuUid;
  UINT32    Offsets[0];
} EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE;

#define EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_VERSION 1

///
/// RISC-V Hart Capabilities Table (RHCT)
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         Reserved;
  UINT64                         TimerFreq;
  UINT32                         NumNodes;
  UINT32                         NodeOffset;
  UINT32                         Nodes[0];
} EFI_ACPI_6_5_RISCV_RHCT_TABLE_HEADER;

#define EFI_ACPI_6_5_RISCV_RHCT_TABLE_REVISION 1

///
/// "RNCT" Multiple APIC Description Table
///
#define EFI_ACPI_6_5_RISCV_RHCT_TABLE_SIGNATURE  SIGNATURE_32('R', 'H', 'C', 'T')

#pragma pack ()

/** Helper macro for CPPC _CPC object initialization. Use of this macro is
    restricted to ASL file and not to TDL file.

    @param [in] DesiredPerfReg      Fastchannel address for desired performance
                                    register.
    @param [in] PerfLimitedReg      Fastchannel address for performance limited
                                    register.
    @param [in] GranularityMHz      Granularity of the performance scale.
    @param [in] HighestPerf         Highest performance in linear scale.
    @param [in] NominalPerf         Nominal performance in linear scale.
    @param [in] LowestNonlinearPerf Lowest non-linear performnce in linear
                                    scale.
    @param [in] LowestPerf          Lowest performance in linear scale.
    @param [in] RefPerf             Reference performance in linear scale.
**/
#define CPPC_PACKAGE_INIT(GranularityMHz,                                      \
  HighestPerf, NominalPerf, LowestNonlinearPerf, LowestPerf, RefPerf)          \
  {                                                                            \
    23,                                 /* NumEntries */                       \
    3,                                  /* Revision */                         \
    HighestPerf,                        /* Highest Performance */              \
    NominalPerf,                        /* Nominal Performance */              \
    LowestNonlinearPerf,                /* Lowest Nonlinear Performance */     \
    LowestPerf,                         /* Lowest Performance */               \
    /* Guaranteed Performance Register */                                      \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Desired Performance Register */                                         \
    ResourceTemplate () { Register (FFixedHW, 32, 0, 0x00000005, 3) },         \
    /* Minimum Performance Register */                                         \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Maximum Performance Register */                                         \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Performance Reduction Tolerance Register */                             \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Time Window Register */                                                 \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Counter Wraparound Time */                                              \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Reference Performance Counter Register */                               \
    ResourceTemplate () { Register (FFixedHW, 64, 0, 0x0000000B, 4) },         \
    /* Delivered Performance Counter Register */                               \
    ResourceTemplate () { Register (FFixedHW, 64, 0, 0x0000000C, 4) },         \
    /* Performance Limited Register */                                         \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* CPPC Enable Register */                                                 \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Autonomous Selection Enable Register */                                 \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Autonomous Activity Window Register */                                  \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Energy Performance Preference Register */                               \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    RefPerf,                            /* Reference Performance */            \
    (LowestPerf * GranularityMHz),      /* Lowest Frequency */                 \
    (NominalPerf * GranularityMHz),     /* Nominal Frequency */                \
  }

// Power state dependency (_PSD) for CPPC

/** Helper macro to initialize Power state dependancy (_PSD) object required
    for CPPC. Use of this macro is restricted to ASL file and not to TDL file.

    @param [in] Domain              The dependency domain number to which this
                                    P-state entry belongs.
**/
#define PSD_INIT(Domain)                                                       \
  {                                                                            \
    5,              /* Entries */                                              \
    0,              /* Revision */                                             \
    Domain,         /* Domain */                                               \
    0xFD,           /* Coord Type- SW_ANY */                                   \
    1               /* Processors */                                           \
  }

// ACPI OSC Status bits
#define OSC_STS_BIT0_RES              (1U << 0)
#define OSC_STS_FAILURE               (1U << 1)
#define OSC_STS_UNRECOGNIZED_UUID     (1U << 2)
#define OSC_STS_UNRECOGNIZED_REV      (1U << 3)
#define OSC_STS_CAPABILITY_MASKED     (1U << 4)
#define OSC_STS_MASK                  (OSC_STS_BIT0_RES          | \
                                       OSC_STS_FAILURE           | \
                                       OSC_STS_UNRECOGNIZED_UUID | \
                                       OSC_STS_UNRECOGNIZED_REV  | \
                                       OSC_STS_CAPABILITY_MASKED)

// ACPI OSC for Platform-Wide Capability
#define OSC_CAP_CPPC_SUPPORT          (1U << 5)
#define OSC_CAP_CPPC2_SUPPORT         (1U << 6)
#define OSC_CAP_PLAT_COORDINATED_LPI  (1U << 7)
#define OSC_CAP_OS_INITIATED_LPI      (1U << 8)


#endif /* ACPI_H_ */
