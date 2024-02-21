/** @file
  An instance of PCI Host Bridge Library.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include "Platform/MemoryMap.h"

#define IO_SIZE                     0x10000

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16  *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH PciRootBridgeDevicePathTemplate = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID (0x0A08), // PCIe
    0
  }, {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

STATIC PCI_ROOT_BRIDGE  *mRPList = NULL;
STATIC PCI_ROOT_BRIDGE  mPciRootBridgeTemplate = {
  0 /* fill in */,                                // Segment
  0,                                              // Supports
  0,                                              // Attributes
  TRUE,                                           // DmaAbove4G
  FALSE,                                          // NoExtendedConfigSpace
  FALSE,                                          // ResourceAssigned
  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |          // AllocationAttributes
  EFI_PCI_HOST_BRIDGE_MEM64_DECODE,
  {
    // Bus
    0,
    0x7F
  }, {
    // Io
     0,/* Min - fill in */
     0 /* Max - fill in */
  }, {
    // Mem
     0,/* Min - fill in */
     0 /* Max - fill in */
  }, {
    // MemAbove4G
     MAX_UINT64,
     0/* Max - fill in */
  }, {
    // PMem
    MAX_UINT64,
    0
  }, {
    // PMemAbove4G
    MAX_UINT64,
    0
  },
  NULL
};

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN  *Count
  )
{
  EFI_PHYSICAL_ADDRESS  EcamBase[] = { THUNDERHILL_PCIE0_ECAM, THUNDERHILL_PCIE1_ECAM };
  UINT64                EcamSize[] = { THUNDERHILL_PCIE0_ECAM_SIZE, THUNDERHILL_PCIE1_ECAM_SIZE };
  EFI_PHYSICAL_ADDRESS  Mmio32Base[] = { THUNDERHILL_PCIE0_MMIO32, THUNDERHILL_PCIE1_MMIO32 };
  UINT32                Mmio32Size[] = { THUNDERHILL_PCIE0_MMIO32_SIZE, THUNDERHILL_PCIE1_MMIO32_SIZE };
  EFI_STATUS            Status;
  UINT32                Idx;

  if (!Count) {
    return NULL;
  }

  *Count = sizeof (EcamBase) / sizeof (EFI_PHYSICAL_ADDRESS);
  mRPList = (PCI_ROOT_BRIDGE *) AllocateZeroPool (*Count * sizeof (mPciRootBridgeTemplate));
  for (Idx = 0; Idx < *Count ; Idx++) {
    CopyMem ((VOID *) &mRPList[Idx], (VOID *)&mPciRootBridgeTemplate, sizeof (mPciRootBridgeTemplate));
    mRPList[Idx].Segment = Idx;

    /*
     * Register Ecam MMIO
     */
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    EcamBase[Idx],
                    EcamSize[Idx],
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR (Status);

    Status = gDS->SetMemorySpaceAttributes (
                    EcamBase[Idx],
                    EcamSize[Idx],
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR (Status);

    /*
     * Register MMIO32
     */
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    Mmio32Base[Idx],
                    Mmio32Size[Idx],
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR (Status);

    Status = gDS->SetMemorySpaceAttributes (
                    Mmio32Base[Idx],
                    Mmio32Size[Idx],
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR (Status);

    /*
     * No need to Register MMIO64 since there is no 32bit MMIO in TH
     */

    /*
     * Some devices need IO resource to work correctly.
     * For RISCV, IO access is the same of MMIO so reserve IO_SIZE from MMIO32 for it
     */
    mRPList[Idx].Io.Base = Mmio32Base[Idx];
    mRPList[Idx].Io.Limit = Mmio32Base[Idx] + IO_SIZE - 1;
    mRPList[Idx].Io.Translation = 0;

    mRPList[Idx].Mem.Base = Mmio32Base[Idx];
    mRPList[Idx].Mem.Limit =  Mmio32Base[Idx] + Mmio32Size[Idx] - 1;
    mRPList[Idx].Mem.Translation = 0;

    mRPList[Idx].DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)AllocateZeroPool (sizeof (PciRootBridgeDevicePathTemplate));
    CopyMem ((VOID *)mRPList[Idx].DevicePath, (VOID *) &PciRootBridgeDevicePathTemplate, sizeof (PciRootBridgeDevicePathTemplate));
    ((EFI_PCI_ROOT_BRIDGE_DEVICE_PATH *)mRPList[Idx].DevicePath)->AcpiDevicePath.UID = (UINT32)(Idx << 16);
  }

  return mRPList;
}

/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE  *Bridges,
  UINTN            Count
  )
{
  UINT32 Idx;

  if (!Bridges || !Count) {
    return;
  }

  for (Idx = 0; Idx < Count; Idx++) {
    FreePool ((VOID *)Bridges[Idx].DevicePath);
  }

  FreePool ((VOID *) mRPList);
  mRPList = NULL;
}

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE  HostBridgeHandle,
  VOID        *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  UINTN                              RootBridgeIndex;

  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor      = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for ( ; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (
        Descriptor->ResType <
        (sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr) /
         sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr[0])
        )
        );
      DEBUG ((
        DEBUG_ERROR,
        " %s: Length/Alignment = 0x%lx / 0x%lx\n",
        mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
        Descriptor->AddrLen,
        Descriptor->AddrRangeMax
        ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((
          DEBUG_ERROR,
          "     Granularity/SpecificFlag = %ld / %02x%s\n",
          Descriptor->AddrSpaceGranularity,
          Descriptor->SpecificFlag,
          ((Descriptor->SpecificFlag &
            EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
            ) != 0) ? L" (Prefetchable)" : L""
          ));
      }
    }

    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                                                       (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                                                       );
  }
}
