/** @file
  Memory Detection for Virtual Machines.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MemDetect.c

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Register/RiscV64/RiscVEncoding.h>
#include <Library/PrePiLib.h>
#include <libfdt.h>
#include <Guid/FdtHob.h>

VOID
BuildMemoryTypeInformationHob (
  VOID
  );

/**
  Create memory range resource HOB using the memory base
  address and size.

  @param  MemoryBase     Memory range base address.
  @param  MemorySize     Memory range size.

**/
STATIC
VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Create memory range resource HOB using memory base
  address and top address of the memory range.

  @param  MemoryBase     Memory range base address.
  @param  MemoryLimit    Memory range size.

**/
STATIC
VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

/**
  Build reserved memory range resource HOB.

  @param  MemoryBase     Reserved memory range base address.
  @param  MemorySize     Reserved memory range size.

**/
STATIC
VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Publish system RAM and reserve memory regions.

**/
STATIC
VOID
InitializeRamRegions (
  EFI_PHYSICAL_ADDRESS  SystemMemoryBase,
  UINT64                SystemMemorySize
  )
{
  EFI_PHYSICAL_ADDRESS VariableBase = 0x400940000;
  UINT32 FwVarSize = 0xC0000;

  DEBUG ((
    DEBUG_INFO,
    "%a: Variable Storage Starts:0x%lx End:0x%lx\n",
    __func__,
    VariableBase,
    VariableBase + FwVarSize - 1
     ));

   DEBUG ((DEBUG_INFO, "%a: FW is in RAM\n", __func__));
   AddReservedMemoryBaseSizeHob (VariableBase, FwVarSize);
   if (VariableBase == SystemMemoryBase) {
     AddMemoryRangeHob (VariableBase + FwVarSize,
                         SystemMemoryBase + SystemMemorySize);
   } else {
      AddMemoryRangeHob (SystemMemoryBase, VariableBase);
      AddMemoryRangeHob (VariableBase + FwVarSize,
                         SystemMemoryBase + SystemMemorySize
                        );
   }
}

EFI_STATUS
AddReservedMemoryMap (
  VOID
  )
{
  EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;
  CONST UINT64                *RegProp;
  INT32                       Node;
  INT32                       SubNode;
  INT32                       Len;
  VOID                        *FdtPointer;
  EFI_PHYSICAL_ADDRESS        Addr;
  UINT64                      Size;
  INTN                        NumRsv, i;

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);

  if (FirmwareContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Firmware Context is NULL\n", __func__));
    return EFI_UNSUPPORTED;
  }

  FdtPointer = (VOID *)FirmwareContext->FlattenedDeviceTree;
  if (FdtPointer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid FDT pointer\n", __func__));
    return EFI_UNSUPPORTED;
  }

   NumRsv = fdt_num_mem_rsv(FdtPointer);
  /* Look for an existing entry and add it to the efi mem map. */
   for (i = 0; i < NumRsv; i++) {
     if (fdt_get_mem_rsv(FdtPointer, i, &Addr, &Size) != 0)
       continue;

      BuildMemoryAllocationHob (
        Addr,
        Size,
        EfiReservedMemoryType
        );
  }

  /* process reserved-memory */
  Node = fdt_subnode_offset(FdtPointer, 0, "reserved-memory");
  if (Node >= 0) {
    fdt_for_each_subnode(SubNode, FdtPointer, Node) {
      RegProp = fdt_getprop (FdtPointer, SubNode, "reg", &Len);
      if ((RegProp != 0) && (Len == (2 * sizeof (UINT64)))) {
        Addr = fdt64_to_cpu (ReadUnaligned64 (RegProp));
        Size = fdt64_to_cpu (ReadUnaligned64 (RegProp + 1));

        DEBUG ((DEBUG_INFO, "%a: Adding reserved memory regions\n", __func__));
        DEBUG ((DEBUG_INFO, "%a: Addr = 0x%llx, Size = 0x%llx\n",
                            __func__, Addr, Size));

        BuildMemoryAllocationHob (
          Addr,
          Size,
          EfiReservedMemoryType
          );
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Initialize memory hob based on the DTB information.

  @return EFI_SUCCESS     The memory hob added successfully.

**/
EFI_STATUS
MemoryPeimInitialization (
  VOID
  )
{
  EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;
  CONST UINT64                *RegProp;
  CONST CHAR8                 *Type;
  UINT64                      CurBase, CurSize;
  INT32                       Node, Prev;
  INT32                       Len;
  VOID                        *FdtPointer;

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);

  if (FirmwareContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Firmware Context is NULL\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  FdtPointer = (VOID *)FirmwareContext->FlattenedDeviceTree;
  if (FdtPointer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid FDT pointer\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  // Look for the lowest memory node
  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (FdtPointer, Prev, NULL);
    if (Node < 0) {
      break;
    }

    // Check for memory node
    Type = fdt_getprop (FdtPointer, Node, "device_type", &Len);
    if (Type && (AsciiStrnCmp (Type, "memory", Len) == 0)) {
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      RegProp = fdt_getprop (FdtPointer, Node, "reg", &Len);
      if ((RegProp != 0) && (Len == (2 * sizeof (UINT64)))) {
        CurBase = fdt64_to_cpu (ReadUnaligned64 (RegProp));
        CurSize = fdt64_to_cpu (ReadUnaligned64 (RegProp + 1));

        DEBUG ((
          DEBUG_INFO,
          "%a: System RAM @ 0x%lx - 0x%lx\n",
          __FUNCTION__,
          CurBase,
          CurBase + CurSize - 1
          ));

          InitializeRamRegions (
            CurBase,
            CurSize
            );
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse FDT memory node\n",
          __FUNCTION__
          ));
      }
    }
  }

  AddReservedMemoryMap();

  BuildMemoryTypeInformationHob ();

  return EFI_SUCCESS;
}
