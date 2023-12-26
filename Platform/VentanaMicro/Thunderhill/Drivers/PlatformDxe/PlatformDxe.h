/** @file
  THUNDERHILL DXE platform driver.

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PLATFORM_DXE_H__
#define PLATFORM_DXE_H__

#include <PiDxe.h>
#include <Guid/EventGroup.h>
#include <Protocol/AcpiTable.h>
#include <IndustryStandard/Acpi.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <IndustryStandard/Acpi65.h>
#include <libfdt.h>
#include <Library/HobLib.h>
#include <Platform/Acpi.h>
#include <Platform/MemoryMap.h>
#include <Platform/Soc.h>
#include <AcpiPlatform.h>

EFI_STATUS
AcpiInstallMadtTable (
  VOID
  );

EFI_STATUS
AcpiInstallRhctTable (
  VOID
  );

#endif /* PLATFORM_DXE_H */