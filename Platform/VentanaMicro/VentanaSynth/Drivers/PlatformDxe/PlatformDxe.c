/** @file
  VENTANASYNTH DXE platform driver.

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformDxe.h"

/*
 * This GUID must match the FILE_GUID in AcpiTables.inf of each boards
 */
STATIC CONST EFI_GUID mAcpiCommonTableFile = { 0xF61AAC94, 0x5E80, 0x40C4, { 0x87, 0x32, 0xD4, 0xDD, 0x70, 0x68, 0x90, 0x51 } } ;
STATIC CONST EFI_GUID mVentanaSynthAcpiTableFile = { 0xa9b0d3e0, 0x9569, 0x4bc4, { 0xad, 0x25, 0x4d, 0x6c, 0x10, 0x9e, 0x88, 0xe1 } };

STATIC EFI_EVENT mAcpiProtocolRegistration = NULL;

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
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = LocateAndInstallAcpiFromFv (&mVentanaSynthAcpiTableFile);
  if (EFI_ERROR (Status)) {
    return;
  }

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
  EFI_STATUS  Status;

  // Install ACPI MADT
  Status = AcpiInstallMadtTable ();
  ASSERT_EFI_ERROR (Status);

  // Install ACPI RHCT
  Status = AcpiInstallRhctTable ();
  ASSERT_EFI_ERROR (Status);

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
PlatformDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_EVENT                       ReadyToBootEvent;
  EFI_EVENT                       ExitBootServicesEvent;

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
