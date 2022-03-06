/** @file
The module to pass the device tree to DXE via HOB.

Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RiscVFirmwareContextLib.h>
#include <Library/PcdLib.h>

#include <libfdt.h>

#include <Guid/FdtHob.h>

EFI_STATUS QemuGetMemoryInfo(
			     VOID *DeviceTreeBase
			     )
{
	INT32         Node, Prev;
	CONST CHAR8   *Type;
	INT32         Len;
	CONST UINT64  *RegProp;
	RETURN_STATUS PcdStatus;
	UINT64        CurBase, CurSize;
	UINT64        NewBase = 0, NewSize = 0;

	// Look for the lowest memory node
	for (Prev = 0;; Prev = Node) {
	  Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
	  if (Node < 0) {
	    break;
	  }

	  // Check for memory node
	  Type = fdt_getprop (DeviceTreeBase, Node, "device_type", &Len);
	  if (Type && AsciiStrnCmp (Type, "memory", Len) == 0) {
	    // Get the 'reg' property of this node. For now, we will assume
	    // two 8 byte quantities for base and size, respectively.
	    RegProp = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
	    if (RegProp != 0 && Len == (2 * sizeof (UINT64))) {

	      CurBase = fdt64_to_cpu (ReadUnaligned64 (RegProp));
	      CurSize = fdt64_to_cpu (ReadUnaligned64 (RegProp + 1));

	      DEBUG ((DEBUG_INFO, "%a: System RAM @ 0x%lx - 0x%lx\n",
		__FUNCTION__, CurBase, CurBase + CurSize - 1));

	      if (NewBase > CurBase || NewBase == 0) {
		NewBase = CurBase;
		NewSize = CurSize;
	      }
	    } else {
	      DEBUG ((DEBUG_ERROR, "%a: Failed to parse FDT memory node\n",
		__FUNCTION__));
	    }
	  }
	}

	DEBUG ((DEBUG_INFO, "QemuGetMemoryInfo: NewSize = 0x%lx, NewBase = 0x%lx\n",
		NewSize, NewBase));
	PcdStatus = PcdSet64S (PcdSystemMemorySize, NewSize);
	ASSERT_RETURN_ERROR (PcdStatus);
	PcdStatus = PcdSet64S (PcdSystemMemoryBase, NewBase);
	ASSERT_RETURN_ERROR (PcdStatus);

	return EFI_SUCCESS;
}

/**
  The entrypoint of the module, it will pass the FDT via a HOB.

  @param  FileHandle             Handle of the file being invoked.
  @param  PeiServices            Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            The address of FDT is passed in HOB.
          EFI_UNSUPPORTED        Can't locate FDT.
**/
EFI_STATUS
EFIAPI
PeimPassFdt (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  VOID                                *FdtPointer;
  VOID                                *Base;
  VOID                                *NewBase;
  UINTN                               FdtSize;
  UINTN                               FdtPages;
  UINT64                              *FdtHobData;
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  *FirmwareContext;

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);

  if (FirmwareContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: OpenSBI Firmware Context is NULL\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  FdtPointer = (VOID *)FirmwareContext->FlattenedDeviceTree;
  if (FdtPointer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid FDT pointer\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_ERROR, "%a: Build FDT HOB - FDT at address: 0x%x \n", __FUNCTION__, FdtPointer));
  Base = FdtPointer;
  ASSERT (Base != NULL);
  ASSERT (fdt_check_header (Base) == 0);

  FdtSize  = fdt_totalsize (Base);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  fdt_open_into (Base, NewBase, EFI_PAGES_TO_SIZE (FdtPages));
  QemuGetMemoryInfo(NewBase);

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)NewBase;

  return EFI_SUCCESS;
}
