/** @file
  Common definition for ACPI table

  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_PLATFORM_H_
#define ACPI_PLATFORM_H_

#define EFI_ACPI_VENTANA_OEM_ID           'V','N','T','A','N','A'   // OEMID 6 bytes long
#define EFI_ACPI_VENTANA_OEM_TABLE_ID     SIGNATURE_64 ('O','R','B','I','T','E','R',' ') // OEM table id 8 bytes long
#define EFI_ACPI_VENTANA_OEM_REVISION     0x00000001
#define EFI_ACPI_VENTANA_CREATOR_ID       SIGNATURE_32 ('V','N','T','N')
#define EFI_ACPI_VENTANA_CREATOR_REVISION 0x00000001

#define VENTANA_ACPI_HEADER(Signature, Type, Revision) {            \
  Signature,                          /* UINT32  Signature */       \
  sizeof (Type),                      /* UINT32  Length */          \
  Revision,                           /* UINT8   Revision */        \
  0,                                  /* UINT8   Checksum */        \
  { EFI_ACPI_VENTANA_OEM_ID },        /* UINT8   OemId[6] */        \
  EFI_ACPI_VENTANA_OEM_TABLE_ID,      /* UINT64  OemTableId */      \
  EFI_ACPI_VENTANA_OEM_REVISION,      /* UINT32  OemRevision */     \
  EFI_ACPI_VENTANA_CREATOR_ID,        /* UINT32  CreatorId */       \
  EFI_ACPI_VENTANA_CREATOR_REVISION   /* UINT32  CreatorRevision */ \
  }

#ifndef NULL_GAS
#define NULL_GAS  { EFI_ACPI_6_4_SYSTEM_MEMORY,  0, 0, EFI_ACPI_6_4_UNDEFINED, 0L }
#endif

#endif /* ACPI_PLATFORM_H_ */
