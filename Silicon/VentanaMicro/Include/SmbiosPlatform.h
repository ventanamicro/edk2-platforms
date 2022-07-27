/** @file
  Common definition for ACPI table

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_PLATFORM_H_
#define SMBIOS_PLATFORM_H_

#define VENDOR_TEMPLATE       "Ventana Micro System(R)\0"
#define BIOS_VERSION_TEMPLATE "TianoCore 0.00.00000000\0"
#define RELEASE_DATE_TEMPLATE "MM/DD/YYYY\0"

#define TYPE0_ADDITIONAL_STRINGS                    \
  VENDOR_TEMPLATE       /* Vendor */         \
  BIOS_VERSION_TEMPLATE /* BiosVersion */    \
  RELEASE_DATE_TEMPLATE /* BiosReleaseDate */

#define MANUFACTURER_TEMPLATE "Ventana Micro System(R)\0"

#define TYPE11_ADDITIONAL_STRINGS       \
  "www.ventanamicro.com\0"

#endif /* SMBIOS_PLATFORM_H_ */