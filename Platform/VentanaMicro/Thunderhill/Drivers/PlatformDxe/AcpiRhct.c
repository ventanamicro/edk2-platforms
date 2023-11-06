/** @file
  THUNDERHILL DXE platform driver.

  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformDxe.h"

#define ISA_MAX_STRING      1024

#pragma pack (1)
typedef struct {
  EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE      HartInfoHeader;
  UINT32                                                IsaOffset;
  UINT32                                                CmoOffset;
} EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_DATA;
#pragma pack ()

STATIC UINT64  *mDeviceTree = NULL;

EFI_ACPI_6_5_RISCV_RHCT_TABLE_HEADER mRhctHeaderTemplate = 
{
  THUNDERHILL_ACPI_HEADER (
    EFI_ACPI_6_5_RISCV_RHCT_TABLE_SIGNATURE,
    0, // Fill in
    EFI_ACPI_6_5_RISCV_RHCT_TABLE_REVISION
  ),
  0, // Reserved
  0, // Timer freq, fill in
  0, // Number of nodes, fill in
  0, // Offset of first node, fill in
};

EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE mRhctIsaTemplate = 
{
  EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE, // Type
  0,   // Length, fill in
  EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE_VERSION, // Revision
  0,   // ISA length, fill in
};

EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE mRhctCmoTemplate = 
{
  EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE, // Type
  sizeof (EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE), // Length;
  EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE_VERSION, // Revision
  0, // CBOM Block Size, fill in
  0, // CBOP Block Size, fill in
  0, // CBOZ Block Size, fill in
};

EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_DATA mRhctHartInfoTemplate =
{
  {
    EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE,
    sizeof (EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE) + sizeof (UINT32) * 2, // 2 nodes
    EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_VERSION,
    2,
    0, // AcpiUid, fill in
  },
  0, // ISA offset, fill in
  0, // CMO offset, fill in
};

STATIC
UINTN
RhctIsaGenerate (EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE *IsaPtr)
{
  CONST UINT64                          *Prop;
  INT32                                 Len, Node;
  VOID                                  *FdtPointer;
  UINTN                                 SizeGenerated;

  SizeGenerated = 0;
  FdtPointer = mDeviceTree;

  // Get the node offset of cpus node
  Node = fdt_path_offset (FdtPointer, "/cpus");
  if (Node < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No cpus node found @ 0x%p\n",
      __func__,
      FdtPointer
      ));
    return 0;
  }
  Node = fdt_first_subnode (FdtPointer, Node);
  for (; Node >= 0; Node = fdt_next_subnode (FdtPointer, Node)) {
    if (fdt_node_check_compatible  (FdtPointer, Node, "riscv") == 0) {
      Prop = fdt_getprop (FdtPointer, Node, "riscv,isa", &Len);
      if (!Prop) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse cpu node: riscv,isa\n",
          __func__
          ));
        return 0;
      }
      ASSERT (Len < ISA_MAX_STRING);
      mRhctIsaTemplate.IsaLength = Len - 1;
      CopyMem ((VOID *)IsaPtr, (VOID *)&mRhctIsaTemplate, sizeof (mRhctIsaTemplate));
      CopyMem ((VOID *)IsaPtr + sizeof (mRhctIsaTemplate), (VOID *)Prop, Len);
      if (Len % 2) {
        // Adding zero padding
        Len++;
      }
      IsaPtr->Length = sizeof (mRhctIsaTemplate) + Len;
      SizeGenerated += sizeof (mRhctIsaTemplate) + Len;
      // All cpu nodes should have the same ISA string, so only process 1 one node.
      break;
    }
  }

  return SizeGenerated;
}

STATIC
UINT32
RhctCmoGetBlockSize (UINT32 Val)
{
  UINT32 Ret = 0;

  while (Val > 1) {
    Ret++;
    Val >>= 1;
  }

  return Ret;
}

STATIC
UINTN
RhctCmoGenerate (EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE *CmoPtr)
{
  CONST UINT64                          *Prop;
  INT32                                 Len, Node;
  VOID                                  *FdtPointer;
  UINTN                                 SizeGenerated;

  SizeGenerated = 0;
  FdtPointer = mDeviceTree;

  // Get the node offset of cpus node
  Node = fdt_path_offset (FdtPointer, "/cpus");
  if (Node < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No cpus node found @ 0x%p\n",
      __func__,
      FdtPointer
      ));
    return 0;
  }
  Node = fdt_first_subnode (FdtPointer, Node);
  for (; Node >= 0; Node = fdt_next_subnode (FdtPointer, Node)) {
    if (fdt_node_check_compatible  (FdtPointer, Node, "riscv") == 0) {
      Prop = fdt_getprop (FdtPointer, Node, "riscv,cbom-block-size", &Len);
      if (!Prop) {
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: Failed to parse cpu node: riscv,cbom-block-size\n",
          __func__
          ));
      } else {
        mRhctCmoTemplate.CbomBlockSize = RhctCmoGetBlockSize (fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop)));
      }
      Prop = fdt_getprop (FdtPointer, Node, "riscv,cboz-block-size", &Len);
      if (!Prop) {
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: Failed to parse cpu node: riscv,cboz-block-size\n",
          __func__
          ));
      } else {
        mRhctCmoTemplate.CbozBlockSize = RhctCmoGetBlockSize (fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop)));
      }
      CopyMem ((VOID *)CmoPtr, (VOID *)&mRhctCmoTemplate, sizeof (mRhctCmoTemplate));
      SizeGenerated += sizeof (mRhctCmoTemplate);
      // All cpu nodes should have the same CMO, so only process 1 one node.
      break;
    }
  }

  return SizeGenerated;
}

STATIC
UINTN
RhctHartInfoGenerate (
  EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_DATA *HartInfoPtr,
  UINT32  IsaNodeOffset,
  UINT32  CmoNodeOffset,
  UINT32  *NumHartInfo
)
{
  CONST UINT64                          *Prop;
  INT32                                 Len, Node;
  VOID                                  *FdtPointer;
  UINTN                                 AcpiUid;
  UINTN                                 SizeGenerated;

  SizeGenerated = 0;
  AcpiUid = 0;
  if (NumHartInfo) {
    *NumHartInfo = 0;
  }
  FdtPointer = mDeviceTree;

  // Get the node offset of cpus node
  Node = fdt_path_offset (FdtPointer, "/cpus");
  if (Node < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No cpus node found @ 0x%p\n",
      __func__,
      FdtPointer
      ));
    return 0;
  }
  Node = fdt_first_subnode (FdtPointer, Node);
  for (; Node >= 0; Node = fdt_next_subnode (FdtPointer, Node)) {
    if (fdt_node_check_compatible  (FdtPointer, Node, "riscv") == 0) {
      Prop = fdt_getprop (FdtPointer, Node, "status", &Len);
      if ((Prop) && (AsciiStrnCmp ((const CHAR8 *)Prop, "disabled", Len) == 0)) {
        // Ignore disabled node
        continue;
      }
      mRhctHartInfoTemplate.HartInfoHeader.ACPICpuUid = AcpiUid++;
      mRhctHartInfoTemplate.IsaOffset = IsaNodeOffset;
      mRhctHartInfoTemplate.CmoOffset = CmoNodeOffset;
      CopyMem ((VOID *)HartInfoPtr, (VOID *)&mRhctHartInfoTemplate, sizeof (mRhctHartInfoTemplate));
      SizeGenerated += sizeof (mRhctHartInfoTemplate);
      HartInfoPtr++;
      if (NumHartInfo) {
        (*NumHartInfo)++;
      }
    }
  }

  return SizeGenerated;
}

/*
 *  Install RHCT table.
 */
EFI_STATUS
AcpiInstallRhctTable (
  VOID
  )
{
  EFI_ACPI_6_5_RISCV_RHCT_TABLE_HEADER                    *RhctPtr;
  EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE              *IsaPtr;
  EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE              *CmoPtr;
  EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_DATA   *HartInfoPtr;
  EFI_ACPI_TABLE_PROTOCOL                                 *AcpiTableProtocol;
  UINTN                                                   RhctTableKey  = 0;
  EFI_STATUS                                              Status;
  UINTN                                                   MaxLength, Length;
  VOID                                                    *Hob;
  INT32                                                   Node, PropLen;
  CONST UINT64                                            *Prop;
  UINT32                                                  NumHartInfo;

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

  // Get the node offset of cpus node
  Node = fdt_path_offset (mDeviceTree, "/cpus");
  if (Node < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No cpus node found @ 0x%p\n",
      __func__,
      mDeviceTree
      ));
    return EFI_NOT_FOUND;
  }
  Prop = fdt_getprop (mDeviceTree, Node, "timebase-frequency", &PropLen);
  if (Prop) {
    mRhctHeaderTemplate.TimerFreq = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
  }
 
  // Calculate maximum size
  MaxLength = sizeof (EFI_ACPI_6_5_RISCV_RHCT_TABLE_HEADER) +
         sizeof (EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE) + ISA_MAX_STRING +
         sizeof (EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE) +
         sizeof (EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_DATA) * THUNDERHILL_NUM_CORE;
  RhctPtr = (EFI_ACPI_6_5_RISCV_RHCT_TABLE_HEADER *)AllocateZeroPool (MaxLength);
  if (RhctPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (
    RhctPtr,
    &mRhctHeaderTemplate,
    sizeof (mRhctHeaderTemplate)
    );  
  Length = sizeof (mRhctHeaderTemplate);

  // Generate ISA node
  IsaPtr = (EFI_ACPI_6_5_RISCV_RHCT_ISA_NODE_STRUCTURE *)((UINT64)RhctPtr + Length);
  Length += RhctIsaGenerate (IsaPtr);

  // Generate CMO node
  CmoPtr = (EFI_ACPI_6_5_RISCV_RHCT_CMO_NODE_STRUCTURE *)((UINT64)RhctPtr + Length);
  Length += RhctCmoGenerate (CmoPtr);

  HartInfoPtr = (EFI_ACPI_6_5_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_DATA *)((UINT64)RhctPtr + Length);
  Length += RhctHartInfoGenerate (HartInfoPtr,
              (UINT64)IsaPtr - (UINT64)RhctPtr,
              (UINT64)CmoPtr - (UINT64)RhctPtr,
              &NumHartInfo);
  ASSERT(NumHartInfo <= THUNDERHILL_NUM_CORE);
  
  RhctPtr->NumNodes = 2 + NumHartInfo; // ISA + CMO + Hart nodes
  RhctPtr->NodeOffset = (UINT64)IsaPtr - (UINT64)RhctPtr; // First node offset

  // Update checksum
  ASSERT (MaxLength >= Length);
  RhctPtr->Header.Length = Length;
  AcpiUpdateChecksum ((UINT8 *)RhctPtr, RhctPtr->Header.Length);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)RhctPtr,
                                RhctPtr->Header.Length,
                                &RhctTableKey
                                );
  FreePool ((VOID *)RhctPtr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
