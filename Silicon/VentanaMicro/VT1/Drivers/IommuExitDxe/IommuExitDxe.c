/** @file
*
*  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC UINTN mIommuBase;
STATIC EFI_GUID  mRiscVIommuInfoHobGuid = {
  0x070702e7, 0x1c0c, 0x483f, { 0xb6, 0x00, 0x8e, 0x18, 0x70, 0x6d, 0x86, 0x13 }
};

#define RV_IOMMU_DDTP 0x0010 
#define IOMMU_OFF 0

VOID
EFIAPI
ExitBootServicesEventIommu (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN Ddtp;

  DEBUG((DEBUG_INFO,"IOMMU ExitBootServicesEvent\n"));
  if (mIommuBase != 0) {
    Ddtp = MmioRead64 (mIommuBase + RV_IOMMU_DDTP);
    DEBUG((DEBUG_INFO, "%a: DDTP=0x%x\n", __func__, Ddtp));
    Ddtp = Ddtp | IOMMU_OFF;
    MmioWrite64 (mIommuBase + RV_IOMMU_DDTP, Ddtp);
  }
}

STATIC
EFI_STATUS
GetIommuBase(VOID)
{
  EFI_HOB_GENERIC_HEADER     *Hob;

  Hob = GetFirstGuidHob (&mRiscVIommuInfoHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    return EFI_NOT_FOUND;
  }

  mIommuBase = (UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);
  if (mIommuBase == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IommuExitDxeEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable)
{
  EFI_STATUS Status;
  EFI_EVENT  Event = NULL;

  Status = GetIommuBase();
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "%a: No IOMMU found\n", __func__));
    return EFI_NOT_FOUND;
  }

  Status = gBS->CreateEvent (
      EVT_SIGNAL_EXIT_BOOT_SERVICES,
      TPL_CALLBACK,
      ExitBootServicesEventIommu,
      NULL,
      &Event
      );

  if (EFI_ERROR(Status))
  {
    DEBUG ((DEBUG_ERROR, "[%a:%d] - CreateEvent failed: %r\n", __func__,
        __LINE__, Status));
  }

  return Status;
}

