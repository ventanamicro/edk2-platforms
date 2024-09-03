/** @file
  THUNDERHILL DXE platform driver.

  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformDxe.h"

#define IMSIC_MMIO_PAGE_SHIFT     12
#define IMSIC_MMIO_PAGE_SZ        (1 << IMSIC_MMIO_PAGE_SHIFT)

STATIC UINT64  *mDeviceTree = NULL;

#define MADT_RINTC_STRUCTURE(RintcFlags, HartId, AcpiCpuUid, IntId, ImsicBase, ImsicSize) \
  { \
    EFI_ACPI_6_5_RISCV_RINTC, \
    sizeof (EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE), \
    EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE_VERSION, \
    0, \
    RintcFlags, \
    HartId, \
    AcpiCpuUid, \
    IntId, \
    ImsicBase, \
    ImsicSize \
  }

#define MADT_IMSIC_STRUCTURE(NumIdentities, NumGuestIdentities, GuestIdxBit, HartIdxBit, GroupIdxBit, GroupIdxShift) \
  { \
    EFI_ACPI_6_5_RISCV_IMSIC, \
    sizeof (EFI_ACPI_6_5_RISCV_IMSIC_STRUCTURE), \
    EFI_ACPI_6_5_RISCV_IMSIC_STRUCTURE_VERSION, \
    0,  \
    0,  \
    NumIdentities, \
    NumGuestIdentities, \
    GuestIdxBit, \
    HartIdxBit, \
    GroupIdxBit, \
    GroupIdxShift \
  }

#define MADT_APLIC_STRUCTURE(Id, Hid, NumIDCs, GlobalIntBase, BaseAddress, BaseSize, NumSources) \
  { \
    EFI_ACPI_6_5_RISCV_APLIC, \
    sizeof (EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE), \
    EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE_VERSION, \
    Id, \
    0,  \
    Hid, \
    NumIDCs, \
    NumSources, \
    GlobalIntBase, \
    BaseAddress, \
    BaseSize \
  }

EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER mMadtHeaderTemplate = 
{
  THUNDERHILL_ACPI_HEADER (
    EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
    0, // fill in
    EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION
  ),
  0, // LocalApicAddress
  0, // Flags
};

STATIC
UINTN
MadtRintcImiscGenerate (EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE *RintcPtr)
{
  EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE    RintcTemplate = MADT_RINTC_STRUCTURE(0, 0, 0, 0, 0, 0);
  EFI_ACPI_6_5_RISCV_IMSIC_STRUCTURE    ImsicTemplate = MADT_IMSIC_STRUCTURE(0, 0, 0, 0, 0, 0);
  CONST UINT64                          *Prop, *IntExtProp, *ImsicRegProp;
  UINT32                                HartId;
  INT32                                 Len, NumpHandle, Node, Prev, CpuNode, pHandle;
  INTN                                  Idx, Idx1, Idx2, Limit;
  VOID                                  *FdtPointer;
  UINT64                                ImsisBaseAddr, ImsisBaseLen;
  UINTN                                 NumIds, NumGuestIds;
  UINTN                                 GuestIdxBits, HartIdxBits;
  UINTN                                 GroupIdxBits, GroupIdxShift;
  UINTN                                 NumImsicBase, ACPICpuId, SizeGenerated;

  ACPICpuId = 0;
  SizeGenerated = 0;
  FdtPointer = mDeviceTree;

  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (FdtPointer, Prev, NULL);
    if (Node < 0) {
      break;
    }

    // Check for imsic node
    if (fdt_node_check_compatible  (FdtPointer, Node, "riscv,imsics") == 0) {
      Prop = fdt_getprop (FdtPointer, Node, "status", &Len);
      if ((Prop != 0) && (AsciiStrnCmp ((const CHAR8 *)Prop, "disabled", Len) == 0)) {
        // Ignore disabled node
        continue;
      }
      Prop = fdt_getprop (FdtPointer, Node, "riscv,num-ids", &Len);
      if (!Prop) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid num-ids\n",
          __func__
          ));
        return 0;
      }
      NumIds = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      Prop = fdt_getprop (FdtPointer, Node, "riscv,num-guest-ids", &Len);
      if (!Prop) {
        NumGuestIds = NumIds;
      } else {
        NumGuestIds = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      }
      Prop = fdt_getprop (FdtPointer, Node, "riscv,guest-index-bits", &Len);
      if (!Prop) {
        GuestIdxBits = 0;
      } else {
        GuestIdxBits = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      }
      Prop = fdt_getprop (FdtPointer, Node, "riscv,hart-index-bits", &Len);
      if (!Prop) {
        HartIdxBits = 0; // update default value later
      } else {
        HartIdxBits = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      }
      Prop = fdt_getprop (FdtPointer, Node, "riscv,group-index-bits", &Len);
      if (!Prop) {
        GroupIdxBits = 0;
      } else {
        GroupIdxBits = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      }
      Prop = fdt_getprop (FdtPointer, Node, "riscv,group-index-shift", &Len);
      if (!Prop) {
        GroupIdxShift = IMSIC_MMIO_PAGE_SHIFT * 2;
      } else {
        GroupIdxShift = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      }
      Prop = fdt_getprop (FdtPointer, Node, "reg", &Len);
      if (!Prop || ((Len / sizeof (UINT32)) % 4) ) {
        // address-cells and size-cells are always 2
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse ismic node: reg\n",
          __func__
          ));
        return 0;
      }
      ImsicRegProp = Prop;
      NumImsicBase = (Len / sizeof (UINT32)) / 4;
      IntExtProp = fdt_getprop (FdtPointer, Node, "interrupts-extended", &Len);
      if (IntExtProp == 0 || ((Len / sizeof (UINT32)) % 2)) {
        /* interrupts-extended: <phandle flag>, <phandle flag> */
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse ismic node: interrupts-extended\n",
          __func__
          ));
        return 0;
      }
      NumpHandle = (Len / sizeof (UINT32)) / 2;
      if (HartIdxBits == 0) {
        Len = NumpHandle;
        while (Len) {
          HartIdxBits++;
          Len = Len >> 1;
        }
      }
      Idx2 = 0;
      for (Idx = 0; Idx < NumImsicBase; Idx++) {
        ImsisBaseAddr = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)ImsicRegProp + Idx * 2));
        ImsisBaseLen = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)ImsicRegProp + Idx * 2 + 1));
        // Calculate the limit of number of cpu nodes this imsic can handle
        Limit = ImsisBaseLen / ((1 < GuestIdxBits) * IMSIC_MMIO_PAGE_SZ);
        for (Idx1 = 0; Idx1 < Limit && Idx2 < NumpHandle; Idx1++, Idx2++) {
          pHandle = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)IntExtProp + Idx2 * 2));
          CpuNode = fdt_node_offset_by_phandle (FdtPointer, pHandle);
          if (CpuNode < 0) {
            DEBUG ((
              DEBUG_ERROR,
              "%a: Failed to locate CPU intc phandle: %X\n",
              __func__,
              pHandle
              ));
            return 0;
          }
          CpuNode = fdt_parent_offset (FdtPointer, CpuNode);
          ASSERT (CpuNode >= 0);
          Prop = fdt_getprop (FdtPointer, CpuNode, "reg", &Len);
          if (!Prop) {
            DEBUG ((
              DEBUG_ERROR,
              "%a: Failed to get hartId at offset %d\n",
              __func__,
              CpuNode
              ));
            return 0;
          }

          HartId = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));

          /*
           * Check for disabled cpu and mark appropriately in MADT.
           */
          Prop = fdt_getprop (FdtPointer, CpuNode, "status", &Len);
          if (!Prop || (Prop &&
                        ((AsciiStrnCmp ((const CHAR8 *)Prop, "okay", Len) == 0) ||
                        (AsciiStrnCmp ((const CHAR8 *)Prop, "ok", Len) == 0)))) {
            RintcTemplate.Flags = EFI_ACPI_6_5_RISCV_RINTC_FLAG_ENABLE;
          }
          else {
            RintcTemplate.Flags = 0;
          }

          RintcTemplate.HardId = (UINT64)HartId;
          RintcTemplate.ACPIProcessorId = (UINT32)ACPICpuId++;
          RintcTemplate.ImsicBaseAddress = ImsisBaseAddr + Idx1 * ((1 << HartIdxBits) * IMSIC_MMIO_PAGE_SZ);
          RintcTemplate.ImsicBaseSize = (UINT32)((1 << HartIdxBits) * IMSIC_MMIO_PAGE_SZ);
          RintcTemplate.ExternalIntCtrlId = 0;
          CopyMem ((VOID *)RintcPtr, (VOID *)&RintcTemplate, sizeof (RintcTemplate));
          SizeGenerated += sizeof (RintcTemplate);
          RintcPtr++;
        }
      }
      // Should process only one imsic node
      break;
    }
  }

  if (!SizeGenerated) {
    // no Rintc generated
    return SizeGenerated;
  }

  ImsicTemplate.GroupIndexBit = GroupIdxBits;
  ImsicTemplate.GroupIndexShift = GroupIdxShift;
  ImsicTemplate.GuestIndexBit = GuestIdxBits;
  ImsicTemplate.HardIndexBit = HartIdxBits;
  ImsicTemplate.NumIdentities =NumGuestIds;
  ImsicTemplate.NumGuestIdentities = NumGuestIds;

  CopyMem (
    (VOID *)RintcPtr,
    &ImsicTemplate,
    sizeof (ImsicTemplate)
    );
  SizeGenerated += sizeof (ImsicTemplate);
  
  return SizeGenerated;
}

STATIC
UINTN
MadtAplicGenerate (EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE *AplicPtr)
{
  EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE    AplicTemplate =
          MADT_APLIC_STRUCTURE(0, 0x313030304E544E56ULL, 0, 0, 0, 0, 0); // APLIC with HID VNTN0001
  CONST UINT64                          *Prop;
  INT32                                 Len, Node, Prev;
  VOID                                  *FdtPointer;
  UINT64                                AplicBaseAddr, AplicBaseSize;
  UINT32                                NumSource;
  UINTN                                 SizeGenerated;

  SizeGenerated = 0;
  FdtPointer = mDeviceTree;
  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (FdtPointer, Prev, NULL);
    if (Node < 0) {
      break;
    }

    // Check for aplic node
    if (fdt_node_check_compatible  (FdtPointer, Node, "riscv,aplic") == 0) {
      Prop = fdt_getprop (FdtPointer, Node, "status", &Len);
      if ((Prop != 0) && (AsciiStrnCmp ((const CHAR8 *)Prop, "disabled", Len) == 0)) {
        // Ignore disabled node
        continue;
      }
      Prop = fdt_getprop (FdtPointer, Node, "reg", &Len);
      if (!Prop || (Len % 4)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse aplic node: reg\n",
          __func__
          ));
        return 0;
      }
      AplicBaseAddr = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)Prop));
      AplicBaseSize = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)Prop + 1));
      Prop = fdt_getprop (FdtPointer, Node, "riscv,num-sources", &Len);
      if (!Prop) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse aplic node: riscv,num-sources\n",
          __func__
          ));
        return 0;
      }
      NumSource = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      AplicTemplate.BaseAddress = AplicBaseAddr;
      AplicTemplate.BaseSize = AplicBaseSize;
      AplicTemplate.NumSources = NumSource + 1;
      CopyMem ((VOID *)AplicPtr, (VOID *)&AplicTemplate, sizeof (AplicTemplate));
      SizeGenerated += sizeof (AplicTemplate);
      // Should have only one APLIC
      break;
    }
  }

  return SizeGenerated;
}

/*
 *  Install MADT table.
 */
EFI_STATUS
AcpiInstallMadtTable (
  VOID
  )
{
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *MadtPtr;
  EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE                  *RintcPtr;
  EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE                  *AplicPtr;
  EFI_ACPI_TABLE_PROTOCOL                             *AcpiTableProtocol;
  UINTN                                               MadtTableKey  = 0;
  EFI_STATUS                                          Status;
  UINTN                                               MaxLength, Length;
  VOID                                                *Hob;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    return EFI_NOT_FOUND;
  }

  mDeviceTree = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);

  if (fdt_check_header (mDeviceTree) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No DTB found @ 0x%p\n",
      __func__,
      mDeviceTree
      ));
    return EFI_NOT_FOUND;
  }

  // Calculate maximum size
  MaxLength = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) +
         sizeof (EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE) * THUNDERHILL_NUM_CORE +
         sizeof (EFI_ACPI_6_5_RISCV_IMSIC_STRUCTURE) +
         sizeof (EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE);

  MadtPtr = (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)AllocateZeroPool (MaxLength);
  if (MadtPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (
    MadtPtr,
    &mMadtHeaderTemplate,
    sizeof (mMadtHeaderTemplate)
    );  
  Length = sizeof (mMadtHeaderTemplate);
  // Generate RINTC and IMSIC nodes
  RintcPtr = (EFI_ACPI_6_5_RISCV_RINTC_STRUCTURE *)((UINT64)MadtPtr + Length);
  Length += MadtRintcImiscGenerate (RintcPtr);

  // Generate APLIC node
  AplicPtr = (EFI_ACPI_6_5_RISCV_APLIC_STRUCTURE *)((UINT64)MadtPtr + Length);
  Length += MadtAplicGenerate (AplicPtr);

  // Update checksum
  ASSERT (MaxLength >= Length);
  MadtPtr->Header.Length = Length;
  AcpiUpdateChecksum ((UINT8 *)MadtPtr, MadtPtr->Header.Length);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)MadtPtr,
                                MadtPtr->Header.Length,
                                &MadtTableKey
                                );
  FreePool ((VOID *)MadtPtr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
