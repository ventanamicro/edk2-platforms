/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Guid/EventGroup.h>
#include <IndustryStandard/Acpi.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include "AcpiPlatform.h"

STATIC EFI_EVENT mAcpiProtocolRegistration = NULL;

/*
 * This GUID must match the FILE_GUID in AcpiTables.inf of each boards
 */
STATIC CONST EFI_GUID mAcpiCommonTableFile = { 0xF61AAC94, 0x5E80, 0x40C4, { 0x87, 0x32, 0xD4, 0xDD, 0x70, 0x68, 0x90, 0x51 } } ;
STATIC CONST EFI_GUID mOrbiterAcpiTableFile = { 0x128d3f66, 0x94e1, 0x4d44, { 0x84, 0xfb, 0x91, 0x5f, 0xa4, 0x5f, 0x61, 0x30 } };

/**
 * Callback called when ACPI Protocol is installed
 */
STATIC VOID
AcpiProtocolNotificationEvent (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS                                   Status;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp;

  Status = LocateAndInstallAcpiFromFv (&mAcpiCommonTableFile);
  ASSERT_EFI_ERROR (Status);

  Status = LocateAndInstallAcpiFromFv (&mOrbiterAcpiTableFile);
  ASSERT_EFI_ERROR (Status);

  //
  // Find ACPI table RSD_PTR from the system table.
  //
  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, (VOID **)&Rsdp);
  if (EFI_ERROR (Status)) {
    Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **)&Rsdp);
  }

  if (!EFI_ERROR (Status) &&
      Rsdp != NULL &&
      Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION &&
      Rsdp->RsdtAddress != 0)
  {
    // RISCV Platforms must set the RSDT address to NULL
    Rsdp->RsdtAddress = 0;
  }
}

VOID
EFIAPI
InstallAcpiOnReadyToBoot (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  // Do nothing for now

  //
  // Close the event, so it will not be signalled again.
  //
  gBS->CloseEvent (Event);
}

VOID
EFIAPI
UpdateAcpiOnExitBootServices (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  // Do nothing for now

  //
  // Close the event, so it will not be signalled again.
  //
  gBS->CloseEvent (Event);
}

EFI_STATUS
EFIAPI
AcpiPlatformDxeInitialize (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_EVENT  ReadyToBootEvent;
  EFI_EVENT  ExitBootServicesEvent;
  EFI_STATUS Status;

  EfiCreateProtocolNotifyEvent (
    &gEfiAcpiTableProtocolGuid,
    TPL_CALLBACK,
    AcpiProtocolNotificationEvent,
    NULL,
    &mAcpiProtocolRegistration
    );

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  UpdateAcpiOnExitBootServices,
                  NULL,
                  &ExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InstallAcpiOnReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
