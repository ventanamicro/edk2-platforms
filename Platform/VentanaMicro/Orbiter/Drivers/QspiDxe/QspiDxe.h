/** @file

  Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
  Copyright (c) 2019, Intel Corporation. All rights reserved.
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef QSPI_DXE_H__
#define QSPI_DXE_H__

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/SpiFlash.h>
#include <Protocol/NonDiscoverableDevice.h>
#include "CadenceXspi.h"

extern EFI_COMPONENT_NAME2_PROTOCOL gQspiDriverComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL  gQspiDriverBinding;

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH              Vendor;
  UINT64                          MmioBase;
  EFI_DEVICE_PATH_PROTOCOL        End;
} QSPI_DEVICE_PATH;
#pragma pack()

typedef struct {
  UINT32                          Signature;
  SPI_FLASH_PROTOCOL              SpiFlashProtocol;
  EFI_PHYSICAL_ADDRESS            MmioBase;
  UINT64                          MmioSize;
  QSPI_DEVICE_PATH                DevicePath;
  NON_DISCOVERABLE_DEVICE         *Dev;
  EFI_EVENT                       VirtualAddressChangeEvent;
} QSPI_PRIVATE;

/**
  Read data from the flash part.

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[out] Buffer              The Pointer to caller-allocated buffer containing the dada received.
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
QspiFlashRead (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  IN     UINT32               Address,
  IN     UINT32               ByteCount,
  OUT    UINT8                *Buffer
  );

/**
  Write data to the flash part.

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[in] Buffer               Pointer to caller-allocated buffer containing the data sent during the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
QspiFlashWrite (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  IN     UINT32               Address,
  IN     UINT32               ByteCount,
  IN     UINT8                *Buffer
  );

/**
  Erase some area on the flash part.

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
QspiFlashErase (
  IN     SPI_FLASH_PROTOCOL     *This,
  IN     FLASH_REGION_TYPE      FlashRegionType,
  IN     UINT32                 Address,
  IN     UINT32                 ByteCount
  );

/**
  Get the SPI region base and size, based on the enum type

  @param[in] This                 Pointer to the SPI_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for for the base address which is listed in the Descriptor.
  @param[out] BaseAddress         The Flash Linear Address for the Region 'n' Base
  @param[out] RegionSize          The size for the Region 'n'

  @retval EFI_SUCCESS             Read success
  @retval EFI_INVALID_PARAMETER   Invalid region type given
  @retval EFI_DEVICE_ERROR        The region is not used
**/
EFI_STATUS
QspiFlashGetRegion (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  OUT    UINT32               *BaseAddress,
  OUT    UINT32               *RegionSize
  );

EFI_STATUS
QspiInit (
  IN     EFI_HANDLE           DriverBindingHandle,
  IN     EFI_HANDLE           ControllerHandle
  );

EFI_STATUS
QspiRelease (
  IN      EFI_HANDLE          DriverBindingHandle,
  IN      EFI_HANDLE          ControllerHandle
  );

#define QSPI_SIGNATURE            SIGNATURE_32 ('Q', 'S', 'P', 'I')
#define QSPI_FROM_THIS(a)         CR ((a), QSPI_PRIVATE, \
                                        SpiFlashProtocol, QSPI_SIGNATURE)

#endif /* QSPI_DXE_H__ */
