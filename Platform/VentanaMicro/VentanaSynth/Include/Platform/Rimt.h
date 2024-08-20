/** @file
  ACPI RISC-V IO Mapping Table (RIMT) definitions.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - RISC-V IO Mapping Table (RIMT) - https://github.com/riscv-non-isa/riscv-acpi-rimt
**/

#ifndef __RISCV_IO_REMAPPING_TABLE_H__
#define __RISCV_IO_REMAPPING_TABLE_H__

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_6_6_RISCV_RIMT_TABLE_SIGNATURE  SIGNATURE_32('R', 'I', 'M', 'T')

#define EFI_ACPI_RISCV_IO_MAPPING_TABLE_REVISION_01  0x1

#define EFI_ACPI_RIMT_TYPE_IOMMU                    0x0
#define EFI_ACPI_RIMT_TYPE_PCI_ROOT_COMPLEX         0x1
#define EFI_ACPI_RIMT_TYPE_PLATFORM_DEVICE          0x2


#define EFI_ACPI_RIMT_IOMMU_FLAG_DEVICE_TYPE       BIT0
#define EFI_ACPI_RIMT_IOMMU_FLAG_PXM_VALID         BIT1

#define EFI_ACPI_6_6_RISCV_IO_MAPPING_GSI_INTERRUPT_FLAG_LEVEL    BIT0
#define EFI_ACPI_6_6_RISCV_IO_MAPPING_GSI_INTERRUPT_FLAG_POLARITY BIT0

#pragma pack(1)

///
/// Table header
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         NumDevices;
  UINT32                         DeviceOffset;
  UINT32                         Reserved;
} EFI_ACPI_6_6_RISCV_IO_MAPPING_TABLE;

///
/// Definition for ID mapping table shared by all device types
///
typedef struct {
  UINT32    SourceIdBase;
  UINT32    NumIds;
  UINT32    DestinationIdBase;
  UINT32    DestinationOffset;
  UINT32    Flags;
} EFI_ACPI_6_6_RISCV_IO_MAPPING_ID_TABLE;

///
/// Device header definition shared by all device types
///
typedef struct {
  UINT8     Type;
  UINT8     Revision;
  UINT16    Length;
  UINT16    Reserved;
  UINT16    Identifier;
} EFI_ACPI_6_6_RISCV_IO_MAPPING_DEVICE;

///
/// GSI Interrupt Structure
///
typedef struct {
  UINT32                                  GsiNumber;
  UINT32                                  Flags;
} EFI_ACPI_6_6_RISCV_IO_MAPPING_GSI_INTERRUPT;

///
/// Device type 0: IOMMU node
///
typedef struct {
  EFI_ACPI_6_6_RISCV_IO_MAPPING_DEVICE    Header;
  UINT8                                   HardwareID[8];
  UINT64                                  BaseAddress;
  UINT32                                  Flags;
  UINT32                                  ProximityDomain;
  UINT16                                  PcieSegmentNumber;
  UINT16                                  PcieBDF;
  UINT16                                  NumGsiInterrupts;
  UINT16                                  GsiInterruptArrayOffset;
} EFI_ACPI_6_6_RISCV_IO_MAPPING_IOMMU_DEVICE;

///
/// Device type 1: PCI root complex Device
///
typedef struct {
  EFI_ACPI_6_6_RISCV_IO_MAPPING_DEVICE    Header;
  UINT32                                  Flags;
  UINT16                                  Reserved;
  UINT16                                  PcieSegmentNumber;
  UINT16                                  IdMappingArrayOffset;
  UINT16                                  NumIdMappings;
} EFI_ACPI_6_6_RISCV_IO_MAPPING_PCI_ROOT_COMPLEX_DEVICE;

///
/// Device type 2: Platform Device
///
typedef struct {
  EFI_ACPI_6_6_RISCV_IO_MAPPING_DEVICE    Header;
  UINT16                                  IdMappingArrayOffset;
  UINT16                                  NumIdMappings;
  // UINT8                                   ObjectName[];
} EFI_ACPI_6_6_RISCV_IO_MAPPING_PLATFORM_DEVICE;

#pragma pack()

#endif
