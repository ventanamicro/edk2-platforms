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
    Device (COM0)
    {
      Name (_HID, "PNP0501") // _HID: Hardware ID
      Name (_UID, Zero)  // _UID: Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
          Memory32Fixed (ReadWrite,
            0x10010000,         // Address Base. FIXME: ISS/QEMU value
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

    Device (ETH0)
    {
      Name (_HID, "XLXFFFF") // _HID: Hardware ID
      Name (_CCA, 0x0)
      Name (_UID, Zero)  // _UID: Unique ID
      Name (_CRS, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x10080000 , 0x00040000)
        Memory32Fixed (ReadWrite, 0x100C0000 , 0x00040000)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, )
        {
          5, 6, 3
        }
      })
      Name (_DSD, Package ()
      {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
          Package ()
          {
            Package () {"phy-mode", "rgmii"},
            Package () {"xlnx,rxmem", 0x1000},
            Package () {"local-mac-address", Package () { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 }}, // 6-bytes
            Package () { "compatible", "xlnx,axi-ethernet-1.00.a" },
            Package () { "phy-handle", \_SB.ETH0.MDIO.PHY1}
          },
      })
      Device (MDIO)
      {
        Name (_ADR, 0x0)
        Device (PHY1)
        {
          Name (_ADR, 0x1)
        //  Name (_DSD, Package ()
        //  {
        //    ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        //    Package ()
        //    {
        //      Package () { "compatible", "xlnx,axi-ethernet-1.00.a" }
        //    }
        //  })  
        }
      }
    }
  } // Scope(\_SB)
}
