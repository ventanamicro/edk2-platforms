/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>
#include "SmbiosPlatform.h"

// Type1 Data
#define PRODUCT_NAME_TEMPLATE "Thunderhill\0"
#define SYS_VERSION_TEMPLATE  "00001\0"
#define SERIAL_TEMPLATE       "123456789ABCDEFF123456789ABCDEFF\0"
#define SKU_TEMPLATE          "FEDCBA9876543211FEDCBA9876543211\0"
#define FAMILY_TEMPLATE       "VT1\0"

#define TYPE1_ADDITIONAL_STRINGS                  \
  MANUFACTURER_TEMPLATE /* Manufacturer */  \
  PRODUCT_NAME_TEMPLATE /* Product Name */  \
  SYS_VERSION_TEMPLATE  /* Version */       \
  SERIAL_TEMPLATE       /* Serial Number */ \
  SKU_TEMPLATE          /* SKU Number */    \
  FAMILY_TEMPLATE       /* Family */

#define TYPE2_ADDITIONAL_STRINGS                   \
  MANUFACTURER_TEMPLATE /* Manufacturer */   \
  PRODUCT_NAME_TEMPLATE /* Product Name */   \
  "Version Not Set\0"              /* Version */        \
  "Serial Not Set\0"    /* Serial */         \
  "Base of Chassis\0"   /* board location */ \
  "FF\0"                /* Version */        \
  "FF\0"                /* Version */

#define CHASSIS_VERSION_TEMPLATE    "None               \0"
#define CHASSIS_SERIAL_TEMPLATE     "Serial Not Set     \0"
#define CHASSIS_ASSET_TAG_TEMPLATE  "Asset Tag Not Set  \0"

#define TYPE3_ADDITIONAL_STRINGS                 \
  MANUFACTURER_TEMPLATE      /* Manufacturer */ \
  CHASSIS_VERSION_TEMPLATE   /* Version */      \
  CHASSIS_SERIAL_TEMPLATE    /* Serial  */      \
  CHASSIS_ASSET_TAG_TEMPLATE /* Asset Tag */    \
  SKU_TEMPLATE               /* SKU Number */

#define TYPE13_ADDITIONAL_STRINGS      \
  "en|US|iso8859-1\0"

#define TYPE41_ADDITIONAL_STRINGS      \
  "Onboard VGA\0"

#define ADDITIONAL_STR_INDEX_1    0x01
#define ADDITIONAL_STR_INDEX_2    0x02
#define ADDITIONAL_STR_INDEX_3    0x03
#define ADDITIONAL_STR_INDEX_4    0x04
#define ADDITIONAL_STR_INDEX_5    0x05
#define ADDITIONAL_STR_INDEX_6    0x06

//
// Type definition and contents of the default SMBIOS table.
// This table covers only the minimum structures required by
// the SMBIOS specification (section 6.2, version 3.0)
//
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE0 Base;
  CHAR8              Strings[sizeof (TYPE0_ADDITIONAL_STRINGS)];
} SMBIOS_PLATFORM_TYPE0;

typedef struct {
  SMBIOS_TABLE_TYPE1 Base;
  CHAR8              Strings[sizeof (TYPE1_ADDITIONAL_STRINGS)];
} SMBIOS_PLATFORM_TYPE1;

typedef struct {
  SMBIOS_TABLE_TYPE2 Base;
  CHAR8              Strings[sizeof (TYPE2_ADDITIONAL_STRINGS)];
} SMBIOS_PLATFORM_TYPE2;

typedef struct {
  SMBIOS_TABLE_TYPE3 Base;
  CHAR8              Strings[sizeof (TYPE3_ADDITIONAL_STRINGS)];
} SMBIOS_PLATFORM_TYPE3;

typedef struct {
  SMBIOS_TABLE_TYPE8 Base;
} SMBIOS_PLATFORM_TYPE8;

typedef struct {
  SMBIOS_TABLE_TYPE9 Base;
} SMBIOS_PLATFORM_TYPE9;

typedef struct {
  SMBIOS_TABLE_TYPE11 Base;
  CHAR8               Strings[sizeof (TYPE11_ADDITIONAL_STRINGS)];
} SMBIOS_PLATFORM_TYPE11;

typedef struct {
  SMBIOS_TABLE_TYPE13 Base;
  CHAR8               Strings[sizeof (TYPE13_ADDITIONAL_STRINGS)];
} SMBIOS_PLATFORM_TYPE13;

typedef struct {
  SMBIOS_TABLE_TYPE41 Base;
  CHAR8               Strings[sizeof (TYPE41_ADDITIONAL_STRINGS)];
} SMBIOS_PLATFORM_TYPE41;

#pragma pack()

// Type 0 BIOS information
STATIC SMBIOS_PLATFORM_TYPE0 mSmbiosDefaultType0 = {
  {
    {                                   // Header
      EFI_SMBIOS_TYPE_BIOS_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE0),      // UINT8 Length, The length of the structure's string-set is not included.
      SMBIOS_HANDLE_PI_RESERVED,
    },

    ADDITIONAL_STR_INDEX_1,     // SMBIOS_TABLE_STRING       Vendor
    ADDITIONAL_STR_INDEX_2,     // SMBIOS_TABLE_STRING       BiosVersion
    0,                          // UINT16                    BiosSegment
    ADDITIONAL_STR_INDEX_3,     // SMBIOS_TABLE_STRING       BiosReleaseDate
    0,                          // UINT8                     BiosSize

    // MISC_BIOS_CHARACTERISTICS BiosCharacteristics
    {
      0,0,0,0,0,0,
      1, // PCI supported
      0,
      1, // PNP supported
      0,
      1, // BIOS upgradable
      0, 0, 0,
      0, // Boot from CD
      1, // selectable boot
    },

    // BIOSCharacteristicsExtensionBytes[2]
    {
      0,
      0,
    },

    0,     // UINT8                     SystemBiosMajorRelease
    0,     // UINT8                     SystemBiosMinorRelease

    // If the system does not have field upgradeable embedded controller
    // firmware, the value is 0FFh
    0xFF,  // UINT8                     EmbeddedControllerFirmwareMajorRelease
    0xFF   // UINT8                     EmbeddedControllerFirmwareMinorRelease
  },

  // Text strings (unformatted area)
  TYPE0_ADDITIONAL_STRINGS
};

// Type 1 System information
STATIC SMBIOS_PLATFORM_TYPE1 mSmbiosDefaultType1 = {
  {
    { // Header
      EFI_SMBIOS_TYPE_SYSTEM_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE1),
      SMBIOS_HANDLE_PI_RESERVED,
    },

    ADDITIONAL_STR_INDEX_1,                                                     // Manufacturer
    ADDITIONAL_STR_INDEX_2,                                                     // Product Name
    ADDITIONAL_STR_INDEX_3,                                                     // Version
    ADDITIONAL_STR_INDEX_4,                                                     // Serial Number
    { 0x12345678, 0x9ABC, 0xDEFF, { 0x12,0x34,0x56,0x78,0x9A,0xBC,0xDE,0xFF }}, // UUID
    SystemWakeupTypePowerSwitch,                                                // Wakeup type
    ADDITIONAL_STR_INDEX_5,                                                     // SKU Number
    ADDITIONAL_STR_INDEX_6,                                                     // Family
  },

  // Text strings (unformatted)
  TYPE1_ADDITIONAL_STRINGS
};

// Type 2 Baseboard
STATIC SMBIOS_PLATFORM_TYPE2 mSmbiosDefaultType2 = {
  {
    {                                        // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE2),           // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1, // Manufacturer
    ADDITIONAL_STR_INDEX_2, // Product Name
    ADDITIONAL_STR_INDEX_3, // Version
    ADDITIONAL_STR_INDEX_4, // Serial
    0,                      // Asset tag
    {1},                    // motherboard, not replaceable
    ADDITIONAL_STR_INDEX_5, // location of board
    0xFFFF,                 // chassis handle
    BaseBoardTypeMotherBoard,
    0,
    {0},
  },
  TYPE2_ADDITIONAL_STRINGS
};

// Type 3 Enclosure
STATIC CONST SMBIOS_PLATFORM_TYPE3 mSmbiosDefaultType3 = {
  {
    {                                   // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE3),      // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1,          // Manufacturer
    MiscChassisTypeRackMountChassis, // Rack-mounted chassis
    ADDITIONAL_STR_INDEX_2,          // version
    ADDITIONAL_STR_INDEX_3,          // serial
    ADDITIONAL_STR_INDEX_4,          // asset tag
    ChassisStateUnknown,             // boot chassis state
    ChassisStateSafe,                // power supply state
    ChassisStateSafe,                // thermal state
    ChassisSecurityStatusNone,       // security state
    {0,0,0,0},                       // OEM defined
    2,                               // 2U height
    2,                               // number of power cords
    0,                               // no contained elements
    3,                               // ContainedElementRecordLength;
  },
  TYPE3_ADDITIONAL_STRINGS
};

// Type 11 OEM Strings
STATIC SMBIOS_PLATFORM_TYPE11 mSmbiosDefaultType11 = {
  {
    {                               // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_OEM_STRINGS,  // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE11), // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    ADDITIONAL_STR_INDEX_1
  },
  TYPE11_ADDITIONAL_STRINGS
};

// Type 13 BIOS Language Information
STATIC SMBIOS_PLATFORM_TYPE13 mSmbiosDefaultType13 = {
  {
    {                                            // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE13),              // UINT8 Length
      SMBIOS_HANDLE_PI_RESERVED,
    },
    1,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    1,
  },
  TYPE13_ADDITIONAL_STRINGS
};

// Type 24 Hardware Security
STATIC SMBIOS_TABLE_TYPE24 mSmbiosDefaultType24 = {
  {                                    // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_HARDWARE_SECURITY, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE24),      // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  0
};

// Type 32 System Boot Information
STATIC SMBIOS_TABLE_TYPE32 mSmbiosDefaultType32 = {
  {                                          // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE32),            // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  {0, 0, 0, 0, 0, 0},
  0
};

// Type 41 Onboard Devices Extended Information
STATIC SMBIOS_PLATFORM_TYPE41 mSmbiosDefaultType41 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE41),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    1,
    0x83,  // OnBoardDeviceExtendedTypeVideo, Enabled
    1,
    4,
    2,
    0,
  },
  TYPE41_ADDITIONAL_STRINGS
};

STATIC CONST VOID *DefaultCommonTables[] =
{
  &mSmbiosDefaultType0,
  &mSmbiosDefaultType1,
  &mSmbiosDefaultType2,
  &mSmbiosDefaultType11,
  &mSmbiosDefaultType13,
  &mSmbiosDefaultType24,
  &mSmbiosDefaultType32,
  &mSmbiosDefaultType41,
  NULL
};

typedef struct {
  CHAR8 MonthNameStr[4]; // example "Jan", Compiler build date, month
  CHAR8 DigitStr[3];     // example "01", Smbios date format, month
} MonthStringDig;

STATIC MonthStringDig MonthMatch[12] = {
  { "Jan", "01" },
  { "Feb", "02" },
  { "Mar", "03" },
  { "Apr", "04" },
  { "May", "05" },
  { "Jun", "06" },
  { "Jul", "07" },
  { "Aug", "08" },
  { "Sep", "09" },
  { "Oct", "10" },
  { "Nov", "11" },
  { "Dec", "12" }
};

STATIC
VOID
ConstructBuildDate (
  OUT CHAR8 *DateBuf
  )
{
  UINTN i;

  // GCC __DATE__ format is "Feb  2 1996"
  // If the day of the month is less than 10, it is padded with a space on the left
  CHAR8 *BuildDate = __DATE__;

  // SMBIOS spec date string: MM/DD/YYYY
  CHAR8 SmbiosDateStr[sizeof (RELEASE_DATE_TEMPLATE)] = { 0 };

  SmbiosDateStr[sizeof (RELEASE_DATE_TEMPLATE) - 1] = '\0';

  SmbiosDateStr[2] = '/';
  SmbiosDateStr[5] = '/';

  // Month
  for (i = 0; i < sizeof (MonthMatch) / sizeof (MonthMatch[0]); i++) {
    if (AsciiStrnCmp (&BuildDate[0], MonthMatch[i].MonthNameStr, AsciiStrLen (MonthMatch[i].MonthNameStr)) == 0) {
      CopyMem (&SmbiosDateStr[0], MonthMatch[i].DigitStr, AsciiStrLen (MonthMatch[i].DigitStr));
      break;
    }
  }

  // Day
  CopyMem (&SmbiosDateStr[3], &BuildDate[4], 2);
  if (BuildDate[4] == ' ') {
    // day is less then 10, SAPCE filed by compiler, SMBIOS requires 0
    SmbiosDateStr[3] = '0';
  }

  // Year
  CopyMem (&SmbiosDateStr[6], &BuildDate[7], 4);

  CopyMem (DateBuf, SmbiosDateStr, AsciiStrLen (RELEASE_DATE_TEMPLATE));
}

STATIC
UINT8
GetBiosVerMajor (
  VOID
  )
{
  return (PcdGet8 (PcdSmbiosTables1MajorVersion));
}

STATIC
UINT8
GetBiosVerMinor (
  VOID
  )
{
  return (PcdGet8 (PcdSmbiosTables1MinorVersion));
}

STATIC
UINTN
GetStringPackSize (
  CHAR8 *StringPack
  )
{
  UINTN StrCount;
  CHAR8 *StrStart;

  if ((*StringPack == 0) && (*(StringPack + 1) == 0)) {
    return 0;
  }

  // String section ends in double-null (0000h)
  for (StrCount = 0, StrStart = StringPack;
       ((*StrStart != 0) || (*(StrStart + 1) != 0)); StrStart++, StrCount++)
  {
  }

  return StrCount + 2; // Included the double NULL
}

// Update String at String number to String Pack
EFI_STATUS
UpdateStringPack (
  CHAR8 *StringPack,
  CHAR8 *String,
  UINTN StringNumber
  )
{
  CHAR8 *StrStart;
  UINTN StrIndex;
  UINTN InputStrLen;
  UINTN TargetStrLen;
  UINTN BufferSize;
  CHAR8 *Buffer;

  StrStart = StringPack;
  for (StrIndex = 1; StrIndex < StringNumber; StrStart++) {
    // A string ends in 00h
    if (*StrStart == 0) {
      StrIndex++;
    }
    // String section ends in double-null (0000h)
    if ((*StrStart == 0) && (*(StrStart + 1) == 0)) {
      return EFI_NOT_FOUND;
    }
  }

  if (*StrStart == 0) {
    StrStart++;
  }

  InputStrLen = AsciiStrLen (String);
  TargetStrLen = AsciiStrLen (StrStart);
  BufferSize = GetStringPackSize (StrStart + TargetStrLen + 1);

  // Replace the String if length matched
  // OR this is the last string
  if (InputStrLen == TargetStrLen || (BufferSize == 0)) {
    CopyMem (StrStart, String, InputStrLen);
  }
  // Otherwise, buffer is needed to apply new string
  else {
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Buffer, StrStart + TargetStrLen + 1, BufferSize);
    CopyMem (StrStart, String, InputStrLen + 1);
    CopyMem (StrStart + InputStrLen + 1, Buffer, BufferSize);

    FreePool (Buffer);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UpdateSmbiosType0 (VOID)
{
  EFI_STATUS                          Status        = EFI_SUCCESS;
  MISC_BIOS_CHARACTERISTICS_EXTENSION *MiscExt      = NULL;
  CHAR8                               *ReleaseDateBuf = NULL;
  CHAR8                               *PcdReleaseDate = NULL;
  CHAR8                               BiosVersionStr[128];
  CHAR8                               *StringPack;
  CHAR8                               SizeOfFirmwareVer;
  UINT16                              *FirmwareVersionPcdPtr;

  //
  //  Update Type0 information
  //

  ReleaseDateBuf = &mSmbiosDefaultType0.Strings[0]
                   + sizeof (VENDOR_TEMPLATE) - 1
                   + sizeof (BIOS_VERSION_TEMPLATE) - 1;
  PcdReleaseDate = (CHAR8 *)PcdGetPtr (PcdSmbiosTables0BiosReleaseDate);

  if (AsciiStrnCmp (PcdReleaseDate, RELEASE_DATE_TEMPLATE, AsciiStrLen (RELEASE_DATE_TEMPLATE)) == 0) {
    // If PCD is still template date MM/DD/YYYY, use compiler date
    ConstructBuildDate (ReleaseDateBuf);
  } else {
    // PCD is updated somehow, use PCD date
    CopyMem (ReleaseDateBuf, PcdReleaseDate, AsciiStrLen (PcdReleaseDate));
  }

  /* Assume FW size not more than 16MB */
  mSmbiosDefaultType0.Base.BiosSize = (PcdGet32 (PcdFdSize) / SIZE_64KB) - 1;
  mSmbiosDefaultType0.Base.ExtendedBiosSize.Size = 0;
  mSmbiosDefaultType0.Base.ExtendedBiosSize.Unit = 0;

  // Type0 BIOS Characteristics Extension Byte 1
  MiscExt = (MISC_BIOS_CHARACTERISTICS_EXTENSION *)&(mSmbiosDefaultType0.Base.BIOSCharacteristicsExtensionBytes);

  MiscExt->BiosReserved.AcpiIsSupported = 1;

  // Type0 BIOS Characteristics Extension Byte 2
  MiscExt->SystemReserved.BiosBootSpecIsSupported = 1;
  MiscExt->SystemReserved.FunctionKeyNetworkBootIsSupported = 1;
  MiscExt->SystemReserved.UefiSpecificationSupported = 1;

  // Type0 BIOS Release
  // Decide another way: If the system does not support the use of this
  // field, the value is 0FFh
  mSmbiosDefaultType0.Base.SystemBiosMajorRelease = GetBiosVerMajor ();
  mSmbiosDefaultType0.Base.SystemBiosMinorRelease = GetBiosVerMinor ();

  //
  // Format of PcdFirmwareVersionString is
  // "(MAJOR_VER).(MINOR_VER).(BUILD) Build YYYY.MM.DD", we only need
  // "(MAJOR_VER).(MINOR_VER).(BUILD)" showed in Bios version. Using
  // space character to determine this string. Another case uses null
  // character to end while loop.
  //
  SizeOfFirmwareVer = 0;
  FirmwareVersionPcdPtr = (UINT16 *)PcdGetPtr (PcdFirmwareVersionString);
  while (*FirmwareVersionPcdPtr != ' ' && *FirmwareVersionPcdPtr != '\0') {
    SizeOfFirmwareVer++;
    FirmwareVersionPcdPtr++;
  }

  AsciiSPrint (
    BiosVersionStr,
    sizeof (BiosVersionStr),
    "TianoCore %.*s",
    SizeOfFirmwareVer,
    PcdGetPtr (PcdFirmwareVersionString)
    );
  StringPack = mSmbiosDefaultType0.Strings;

  UpdateStringPack (StringPack, BiosVersionStr, ADDITIONAL_STR_INDEX_2);

  return Status;
}

STATIC
EFI_STATUS
InstallType3Structure (
  IN EFI_SMBIOS_PROTOCOL *Smbios
  )
{
  EFI_STATUS          Status = EFI_SUCCESS;
  EFI_SMBIOS_HANDLE   SmbiosHandle;

  ASSERT (Smbios != NULL);

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER*) &mSmbiosDefaultType3)->Handle;
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosDefaultType3
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "adding SMBIOS type 3 failed\n"));
    // stop adding rather than continuing
    return Status;
  }

  // Save this handle to type 2 table
  mSmbiosDefaultType2.Base.ChassisHandle = SmbiosHandle;

  return Status;
}

/**
   Install a whole table worth of structures

   @param  Smbios               SMBIOS protocol.
   @param  DefaultTables        A pointer to the default SMBIOS table structure.
**/
EFI_STATUS
InstallStructures (
  IN       EFI_SMBIOS_PROTOCOL *Smbios,
  IN CONST VOID                *DefaultTables[]
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  EFI_SMBIOS_HANDLE SmbiosHandle;
  UINTN             TableIndex;

  ASSERT (Smbios != NULL);

  for (TableIndex = 0; DefaultTables[TableIndex] != NULL; TableIndex++) {
    SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)DefaultTables[TableIndex])->Handle;
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER *)DefaultTables[TableIndex]
                       );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: adding %d failed\n", __FUNCTION__, TableIndex));

      // stop adding rather than continuing
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
   Install all structures from the DefaultTables structure

   @param  Smbios               SMBIOS protocol

**/
EFI_STATUS
InstallAllStructures (
  IN EFI_SMBIOS_PROTOCOL *Smbios
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  ASSERT (Smbios != NULL);

  // Update SMBIOS Type 0 tables
  UpdateSmbiosType0 ();

  // Install Type 3 table
  InstallType3Structure (Smbios);

  // Install remaining tables
  Status = InstallStructures (Smbios, DefaultCommonTables);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
   Installs SMBIOS information for ARM platforms

   @param ImageHandle     Module's image handle
   @param SystemTable     Pointer of EFI_SYSTEM_TABLE

   @retval EFI_SUCCESS    Smbios data successfully installed
   @retval Other          Smbios data was not installed

**/
EFI_STATUS
EFIAPI
SmbiosPlatformDxeEntry (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS          Status;
  EFI_SMBIOS_PROTOCOL *Smbios;

  //
  // Find the SMBIOS protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InstallAllStructures (Smbios);
  DEBUG ((DEBUG_INFO, "SmbiosPlatform install - %r\n", Status));

  return Status;
}
