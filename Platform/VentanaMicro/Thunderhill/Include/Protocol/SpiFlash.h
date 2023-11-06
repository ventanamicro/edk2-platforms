/** @file

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_FLASH_PROTOCOL_H_
#define SPI_FLASH_PROTOCOL_H_

//
// Extern the GUID for protocol users.
//
extern EFI_GUID  gEfiSpiFlashProtocolGuid;

//
// SPI protocol data structures and definitions
//

typedef struct _SPI_FLASH_PROTOCOL SPI_FLASH_PROTOCOL;

/**
  Flash Region Type
**/
typedef enum {
  FlashRegionBios,
  FlashRegionNv,
  FlashRegionMax
} FLASH_REGION_TYPE;

//
// Protocol member functions
//

/**
  Read data from the flash part.

  @param[in] This                 Pointer to the SPI_FLASH_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[out] Buffer              The Pointer to caller-allocated buffer containing the dada received.
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
typedef
EFI_STATUS
(EFIAPI *SPI_FLASH_READ) (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  IN     UINT32               Address,
  IN     UINT32               ByteCount,
  OUT    UINT8                *Buffer
  );

/**
  Write data to the flash part.

  @param[in] This                 Pointer to the SPI_FLASH_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[in] Buffer               Pointer to caller-allocated buffer containing the data sent during the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
typedef
EFI_STATUS
(EFIAPI *SPI_FLASH_WRITE) (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  IN     UINT32               Address,
  IN     UINT32               ByteCount,
  IN     UINT8                *Buffer
  );

/**
  Erase some area on the flash part.

  @param[in] This                 Pointer to the SPI_FLASH_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
typedef
EFI_STATUS
(EFIAPI *SPI_FLASH_ERASE) (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  IN     UINT32               Address,
  IN     UINT32               ByteCount
  );

/**
  Get the SPI region base and size, based on the enum type

  @param[in] This                 Pointer to the SPI_FLASH_PROTOCOL instance.
  @param[in] FlashRegionType      The Flash Region type for for the base address which is listed in the Descriptor.
  @param[out] BaseAddress         The Flash Linear Address for the Region 'n' Base
  @param[out] RegionSize          The size for the Region 'n'

  @retval EFI_SUCCESS             Read success
  @retval EFI_INVALID_PARAMETER   Invalid region type given
  @retval EFI_DEVICE_ERROR        The region is not used
**/
typedef
EFI_STATUS
(EFIAPI *SPI_GET_REGION_ADDRESS) (
  IN     SPI_FLASH_PROTOCOL   *This,
  IN     FLASH_REGION_TYPE    FlashRegionType,
  OUT    UINT32               *BaseAddress,
  OUT    UINT32               *RegionSize
  );

/**
  These protocols/PPI allows a platform module to perform SPI operations through the
  SPI Host Controller Interface.
**/
typedef struct _SPI_FLASH_PROTOCOL {
  /**
    This member specifies the revision of this structure. This field is used to
    indicate backwards compatible changes to the protocol.
  **/
  UINT8                         Revision;
  SPI_FLASH_READ                FlashRead;          ///< Read data from the flash part.
  SPI_FLASH_WRITE               FlashWrite;         ///< Write data to the flash part.
  SPI_FLASH_ERASE               FlashErase;         ///< Erase some area on the flash part.
  SPI_GET_REGION_ADDRESS        FlashGetRegion;
} SPI_FLASH_PROTOCOL;

/**
  SPI PPI/PROTOCOL revision number

  Revision 1:   Initial version
**/
#define SPI_FLASH_SERVICES_REVISION   1

#endif /* SPI_FLASH_PROTOCOL_H_ */
