/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

DefinitionBlock("", "DSDT", 1, "VETANA", "ORBITER ", EFI_ACPI_VENTANA_OEM_REVISION)
{
  Include ("Cpu.asi")
  Include ("Pcie.asi")

  Scope (\_SB)
  {
    Device (PLIC)
    {
      Name (_HID, "APLIC001")  // _HID: Hardware ID
      Name (_UID, Zero)  // _UID: Unique ID
      Name (_STA, 0x0B)  // _STA: Status
      Name (_CCA, One)  // _CCA: Cache Coherency Attribute
      Name (_MAT, Buffer (0x18)  // _MAT: Multiple APIC Table Entry
      {
        /* 0000 */  0x1B, 0x18, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,  // ........
        /* 0008 */  0x60, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,  // `.......
        /* 0010 */  0x00, 0x40, 0x01, 0x0D, 0x00, 0x00, 0x00, 0x00   // ........
      })
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        Memory32Fixed (ReadWrite,
          0x00000000,         // Address Base
          0x00008000,         // Address Length
          )
      })
    }

    Device (COM0)
    {
      Name (_HID, "PNP0501" /* 16550A-compatible COM Serial Port */)  // _HID: Hardware ID
      Name (_UID, Zero)  // _UID: Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
          Memory32Fixed (ReadWrite,
            0x10000000,         // Address Base. FIXME: ISS/QEMU value
            0x00000100,         // Address Length
            )
          Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, )
          {
            0x0000000A, //FIXME: ISS/QEMU value
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
            0x00384000
          }
        }
      })
    }
  } // Scope(\_SB)
}
