/** @file

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/EventGroup.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>

/**
  Notify function for event ready to boot.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
OnReadyToBootServices (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS                  Status;
  UINTN                       MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR       *MemoryMap, *Desc;
  UINTN                       MapKey;
  UINTN                       DescriptorSize;
  UINT32                      DescriptorVersion;

  EFI_PHYSICAL_ADDRESS        Addr;
  UINTN                       Idx;
  UINTN                       Pages;

  //
  // Close the event, so it will not be signalled again.
  //
  gBS->CloseEvent (Event);

  MemoryMap = NULL;
  MemoryMapSize = 0;
  Pages = 0;

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
    MemoryMap = AllocatePages (Pages);

    //
    // Get System MemoryMap
    //
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
  }
  if (EFI_ERROR (Status) || MemoryMap == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get memory map\n", __FUNCTION__));
    if (MemoryMap) {
      FreePages ((VOID *)MemoryMap, Pages);
    }
    return;
  }

  Desc = MemoryMap;
  for (Idx = 0; Idx < (MemoryMapSize / DescriptorSize); Idx++) {
    if (Desc->Type == EfiConventionalMemory &&
          ((Desc->PhysicalStart > MAX_UINT32) || (Desc->PhysicalStart + Desc->NumberOfPages * EFI_PAGE_SIZE) > MAX_UINT32)) {
      /*
       * Due to RISC-V has not supported large memory model yet, limit the memory usage for EDK2 application
       * to 32bit so relocation code within +-2GB will work.
       */
      if (Desc->PhysicalStart > MAX_UINT32) {
        Addr = Desc->PhysicalStart;
        Status = gBS->AllocatePages (AllocateAddress, EfiBootServicesData, Desc->NumberOfPages, &Addr);
      } else {
        Addr = 0x100000000ULL;
        Status = gBS->AllocatePages (AllocateAddress, EfiBootServicesData, Desc->NumberOfPages - EFI_SIZE_TO_PAGES (0x100000000 - Desc->PhysicalStart), &Addr);
      }
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to allocate boot service data at %llX\n", __FUNCTION__, Addr));
      }
    }
    Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT64)Desc + DescriptorSize);
  }

  FreePages ((VOID *)MemoryMap, Pages);
}

EFI_STATUS
EFIAPI
FixMemoryMapInitialize (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_EVENT  ReadyToBootEvent;
  EFI_STATUS Status;

  if (PcdGetBool (Limit32bitEfiApplication)) {
    Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnReadyToBootServices,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
