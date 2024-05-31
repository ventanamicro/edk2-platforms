/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

DefinitionBlock("", "DSDT", 1, "VNTANA", "VENTANA ", EFI_ACPI_VENTANA_OEM_REVISION)
{
  Include ("Cpu.asi")
  Include ("Pcie.asi")
  Include ("Pcie-PDRC.asi")

  Scope (\_SB)
  {
    Device (IC00)
   {
       Name (_HID, "RSCV0002") // _HID: Hardware ID
       Name (_UID, Zero)  // _UID: Unique ID
       Method(_GSB) {
           Return (0x0) // Global System Interrupt Base for this APLIC starts at 0
      }
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
          Memory32Fixed (ReadWrite,
            0x10018000,         // Address Base. FIXME: ISS/QEMU value
            0x00008000,         // Address Length
            )
      })
   }
    Device (COM0)
    {
      Name (_HID, "RSCV0003") // _HID: Hardware ID
      Name (_UID, Zero)  // _UID: Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
          Memory32Fixed (ReadWrite,
            0x10020000,         // Address Base. FIXME: ISS/QEMU value
            0x00010000,         // Address Length
            )
          Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, )
          {
            0x1,
          }
      })
      Name (_DSD, Package (0x02)  // _DSD: Device-Specific Data
      {
        ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301") /* Device Properties for _DSD */,
        Package (0x03)
        {
          Package (0x02)
          {
            "clock-frequency",
            0x5F5E100 //FIXME: HW value
          },
          Package (0x02)
          {
            "reg-shift",
            0x2
          },
          Package (0x02)
          {
            "reg-io-width",
            0x4
          }
        }
      })
    }

  } // Scope(\_SB)
}
