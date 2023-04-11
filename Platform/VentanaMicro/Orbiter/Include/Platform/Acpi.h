/** @file
  ACPI table definition of Orbiter platform.

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_H_
#define ACPI_H_

#define ORBITER_ACPI_HEADER(Signature, Type, Revision) {              \
    Signature,                          /* UINT32  Signature */       \
    sizeof (Type),                      /* UINT32  Length */          \
    Revision,                           /* UINT8   Revision */        \
    0,                                  /* UINT8   Checksum */        \
    { 'V', 'E', 'T', 'A', 'N', 'A' },   /* UINT8   OemId[6] */        \
    0x205245544942524FULL,              /* UINT64  OemTableId */      \
    1,                                  /* UINT32  OemRevision */     \
    0x4E544E56,                         /* UINT32  CreatorId */       \
    1                                   /* UINT32  CreatorRevision */ \
  }

#define EFI_ACPI_6_5_RISCV_RINTC                    0x18
#define EFI_ACPI_6_5_RISCV_IMSIC                    0x19
#define EFI_ACPI_6_5_RISCV_APLIC                    0x1A
#define EFI_ACPI_6_5_RISCV_PLIC                     0x1B

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
  UINT8     Reserved;
  UINT32    Id;
  UINT64    Hid;
  UINT32    NumIDCs;
  UINT32    GlobalIntBase;
  UINT64    BaseAddress;
  UINT32    BaseSize;
  UINT16    NumSources;
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
  UINT16    CbomBlockSize;
  UINT16    CbopBlockSize;
  UINT16    CbozBlockSize;
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

#endif /* ACPI_H_ */
