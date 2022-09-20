/** @file
  Orbiter DXE platform driver.

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformDxe.h"

/*
 * This GUID must match the FILE_GUID in AcpiTables.inf of each boards
 */
STATIC CONST EFI_GUID mAcpiCommonTableFile = { 0xF61AAC94, 0x5E80, 0x40C4, { 0x87, 0x32, 0xD4, 0xDD, 0x70, 0x68, 0x90, 0x51 } } ;
STATIC CONST EFI_GUID mOrbiterAcpiTableFile = { 0x128d3f66, 0x94e1, 0x4d44, { 0x84, 0xfb, 0x91, 0x5f, 0xa4, 0x5f, 0x61, 0x30 } };

STATIC EFI_EVENT mAcpiProtocolRegistration = NULL;

STATIC EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR mSysI2cDesc[] = {
  {
    ACPI_ADDRESS_SPACE_DESCRIPTOR,                    // Desc
    sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3,   // Len
    ACPI_ADDRESS_SPACE_TYPE_MEM,                      // ResType
    0,                                                // GenFlag
    0,                                                // SpecificFlag
    32,                                               // AddrSpaceGranularity
    ORBITER_SYS_I2C_BASE,                             // AddrRangeMin
    ORBITER_SYS_I2C_BASE + ORBITER_SYS_I2C_SIZE - 1,  // AddrRangeMax
    0,                                                // AddrTranslationOffset
    ORBITER_SYS_I2C_BASE,                             // AddrLen
  }, {
    ACPI_END_TAG_DESCRIPTOR                           // Desc
  }
};

STATIC EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR mSysQspiDesc[] = {
  {
    ACPI_ADDRESS_SPACE_DESCRIPTOR,                    // Desc
    sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3,   // Len
    ACPI_ADDRESS_SPACE_TYPE_MEM,                      // ResType
    0,                                                // GenFlag
    0,                                                // SpecificFlag
    32,                                               // AddrSpaceGranularity
    ORBITER_SYS_QSPI_BASE,                            // AddrRangeMin
    ORBITER_SYS_QSPI_BASE + ORBITER_SYS_QSPI_SIZE - 1,// AddrRangeMax
    0,                                                // AddrTranslationOffset
    ORBITER_SYS_QSPI_BASE,                            // AddrLen
  }, {
    ACPI_END_TAG_DESCRIPTOR                           // Desc
  }
};

STATIC
EFI_STATUS
RegisterDevice (
  IN  EFI_GUID                            *TypeGuid,
  IN  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Desc,
  OUT EFI_HANDLE                          *Handle
  )
{
  NON_DISCOVERABLE_DEVICE             *Device;
  EFI_STATUS                          Status;

  Device = (NON_DISCOVERABLE_DEVICE *)AllocateZeroPool (sizeof (*Device));
  if (Device == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Device->Type = TypeGuid;
  Device->DmaType = NonDiscoverableDeviceDmaTypeCoherent;
  Device->Resources = Desc;

  Status = gBS->InstallMultipleProtocolInterfaces (Handle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid, Device,
                  NULL);
  if (EFI_ERROR (Status)) {
    goto FreeDevice;
  }
  return EFI_SUCCESS;

FreeDevice:
  FreePool (Device);

  return Status;
}

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

  Status = LocateAndInstallAcpiFromFv (&mOrbiterAcpiTableFile);
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
PlatformDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      Handle;
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

  //
  // Register Sys I2C
  //
  Handle = NULL;
  Status = RegisterDevice (&gOrbiterNonDiscoverableRuntimeSysI2cMasterGuid,
             mSysI2cDesc, &Handle);
  ASSERT_EFI_ERROR (Status);

  //
  // Register Sys QSPI
  //
  Handle = NULL;
  Status = RegisterDevice (&gOrbiterNonDiscoverableRuntimeSysQspiMasterGuid,
             mSysQspiDesc, &Handle);
  ASSERT_EFI_ERROR (Status);

  //
  // Install the PCF85063a I2C Master protocol on this handle so the RTC driver
  // can identify it as the I2C master it can invoke directly, rather than
  // through the I2C driver stack (which cannot be used at runtime)
  //
  Status = gBS->InstallProtocolInterface (&Handle,
                  &gPcf85063aRealTimeClockLibI2cMasterProtocolGuid,
                  EFI_NATIVE_INTERFACE, NULL);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
