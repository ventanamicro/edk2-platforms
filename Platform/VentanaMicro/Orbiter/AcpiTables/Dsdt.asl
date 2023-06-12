/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

DefinitionBlock("", "DSDT", 1, "VNTANA", "ORBITER ", EFI_ACPI_VENTANA_OEM_REVISION)
{
  Include ("Cpu.asi")
  Include ("Pcie.asi")
  Include ("Pcie-PDRC.asi")

  Scope (\_SB)
  {
    Device (COM0)
    {
      Name (_HID, "VNTN0D01") // _HID: Hardware ID
      Name (_UID, Zero)  // _UID: Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
          Memory32Fixed (ReadWrite,
            0x4219C000,         // Address Base. FIXME: ISS/QEMU value
            0x00001000,         // Address Length
            )
          Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, )
          {
            0x00000025,
          }
      })
      Name (_DSD, Package (0x02)  // _DSD: Device-Specific Data
      {
        ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301") /* Device Properties for _DSD */,
        Package (0x01)
        {
          Package (0x02)
          {
            "clock-frequency",
            0x00384000 //FIXME: HW value
          }
        }
      })
    }
  } // Scope(\_SB)
}
