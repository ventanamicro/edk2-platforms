## @file
#  RISC-V EFI on Ventana Micro Orbiter platform
#
#  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = Orbiter
  PLATFORM_GUID                  = F7977CEC-C924-4DA8-8DDB-27C002286F9A
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001c
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/VentanaMicro/Orbiter/Orbiter.fdf

  #
  # Enable below options may cause build error or may not work on
  # the initial version of RISC-V package
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE SECURE_BOOT_ENABLE      = FALSE
  DEFINE DEBUG_ON_SERIAL_PORT    = TRUE

  #
  # Network definition
  #
  DEFINE NETWORK_SNP_ENABLE       = FALSE
  DEFINE NETWORK_IP6_ENABLE       = FALSE
  DEFINE NETWORK_TLS_ENABLE       = TRUE
  DEFINE NETWORK_HTTP_BOOT_ENABLE = TRUE
  DEFINE NETWORK_ISCSI_ENABLE     = FALSE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS = TRUE

  #
  # UPDATE/RECOVERY definition
  #
  DEFINE CAPSULE_ENABLE           = FALSE

[BuildOptions]
  GCC:RELEASE_*_*_CC_FLAGS       = -DMDEPKG_NDEBUG
!ifdef $(SOURCE_DEBUG_ENABLE)
  GCC:*_*_RISCV64_GENFW_FLAGS    = --keepexceptiontable
!endif
  GCC:  *_*_*_DLINK_FLAGS = -z common-page-size=0x1000
  MSFT: *_*_*_DLINK_FLAGS = /ALIGN:4096

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

################################################################################
#
# Specific Platform Pcds
#
################################################################################
[PcdsFixedAtBuild]
  # Serial Port
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|1

  # Use emulator variable for now
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable | TRUE

  # Internal usage only. DO NOT UPSTREAM
  gOrbiterTokenSpaceGuid.PcdFixedRamdiskBase | 0xD0000000
  gOrbiterTokenSpaceGuid.PcdFixedRamdiskSize | 0x00600000

  # ACPI
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiExposedTableVersions|0x20
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId|"Vetana"
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x205245544942524F #ORBITER
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorId|0x4E544E56 # VNTN
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorRevision|0x000000001

  gUefiCpuPkgTokenSpaceGuid.PcdCpuCoreCrystalClockFrequency|100000000

[PcdsDynamicExDefault.common.DEFAULT]
!if $(CAPSULE_ENABLE) == TRUE
  gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareImageDescriptor|{0x0}|VOID*|0x100
  gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{0x48, 0x91, 0x23, 0xc7, 0xff, 0xe9, 0x49, 0x28, 0x83, 0x1d, 0x25, 0x98, 0x0c, 0xac, 0xd6, 0xe1}
  gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareFileGuid|{0x67, 0x96, 0x4e, 0xa2, 0x63, 0x40, 0x4e, 0x34, 0x94, 0xe5, 0xfe, 0x91, 0xc8, 0x1c, 0x18, 0xc3}
!endif
  # set PcdPciExpressBaseAddress to MAX_UINT64, which signifies that this
  # PCD and PcdPciDisableBusEnumeration above have not been assigned yet
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xFFFFFFFFFFFFFFFF
  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation|0x0

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include Silicon/VentanaMicro/VT1/VT1.dsc.inc

[LibraryClasses]
  SerialPortLib|Platform/VentanaMicro/Orbiter/Library/CspSerialPortLib/CspSerialPortLib.inf
  # Need "Real" HW real time clock
  RealTimeClockLib|EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf
  # Enable during CIH
  # RealTimeClockLib|Platform/VentanaMicro/Orbiter/Library/Pcf85063aRealTimeClockLib/Pcf85063aRealTimeClockLib.inf
  RiscVMmuLib|UefiCpuPkg/Library/BaseRiscVMmuLib/BaseRiscVMmuLib.inf

  PlatformBootManagerLib|OvmfPkg/RiscVVirt/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  PlatformBmPrintScLib|OvmfPkg/Library/PlatformBmPrintScLib/PlatformBmPrintScLib.inf
  ResetSystemLib|OvmfPkg/RiscVVirt/Library/ResetSystemLib/BaseResetSystemLib.inf
  QemuBootOrderLib|OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf
  QemuLoadImageLib|OvmfPkg/Library/GenericQemuLoadImageLib/GenericQemuLoadImageLib.inf
  QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgLibMmio.inf
  QemuFwCfgS3Lib|OvmfPkg/Library/QemuFwCfgS3Lib/BaseQemuFwCfgS3LibNull.inf
  QemuFwCfgSimpleParserLib|OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf

!if $(CAPSULE_ENABLE) == TRUE
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf
  EdkiiSystemCapsuleLib|SignedCapsulePkg/Library/EdkiiSystemCapsuleLib/EdkiiSystemCapsuleLib.inf
  FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
  IniParsingLib|SignedCapsulePkg/Library/IniParsingLib/IniParsingLib.inf
  PlatformFlashAccessLib|Platform/VentanaMicro/Orbiter/Capsule/Library/PlatformFlashAccessLib/PlatformFlashAccessLibDxe.inf
  DisplayUpdateProgressLib|MdeModulePkg/Library/DisplayUpdateProgressLibText/DisplayUpdateProgressLibText.inf
  RngLib|MdePkg/Library/DxeRngLib/DxeRngLib.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
!if $(CAPSULE_ENABLE) == TRUE
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeRuntimeCapsuleLib.inf
!endif

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform.
#
################################################################################
[Components]
  #
  # SEC Phase modules
  #
  Platform/VentanaMicro/Orbiter/Sec/SecMain.inf {
    <LibraryClasses>
      ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
      LzmaDecompressLib|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
      PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
      HobLib|EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
      PrePiHobListPointerLib|OvmfPkg/RiscVVirt/Library/PrePiHobListPointerLib/PrePiHobListPointerLib.inf
      MemoryAllocationLib|EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf
  }

  #
  # RISC-V Platform Components
  #
  Platform/VentanaMicro/Orbiter/AcpiTables/AcpiTables.inf
  Platform/VentanaMicro/Orbiter/Drivers/PlatformDxe/PlatformDxe.inf
  Platform/VentanaMicro/Orbiter/Drivers/SmbiosPlatformDxe/SmbiosPlatformDxe.inf

  #
  # Ramdisk for bring up. DO NOT UPSTREAM
  #
  Platform/VentanaMicro/Orbiter/Drivers/RamDiskDxe/RamDiskDxe.inf

  #
  # Platfrom drivers
  #
  Platform/VentanaMicro/Orbiter/Drivers/PlatformDxe/PlatformDxe.inf
  Platform/VentanaMicro/Orbiter/Drivers/I2cDxe/I2cDxe.inf
  Platform/VentanaMicro/Orbiter/Drivers/QspiDxe/QspiDxe.inf
  Platform/VentanaMicro/Orbiter/Drivers/FlashFvbDxe/FlashFvbDxe.inf

  #
  # PCIe Support
  #
  OvmfPkg/RiscVVirt/PciCpuIo2Dxe/PciCpuIo2Dxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      PciHostBridgeLib | Platform/VentanaMicro/Orbiter/Library/PciHostBridgeLib/PciHostBridgeLib.inf
      PciSegmentLib | Platform/VentanaMicro/Orbiter/Library/PciSegmentLibPci/PciSegmentLib.inf

  }
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf

  #
  # Ethernet driver
  #
  Platform/VentanaMicro/Orbiter/Drivers/GigUndiDxe/GigUndiDxe.inf

  # X86Emulator
  Silicon/VentanaMicro/X86EmulatorPkg/X86Emulator.inf

!if $(CAPSULE_ENABLE) == TRUE
  # FMP image decriptor
  # Platform/VentanaMicro/Orbiter/Capsule/SystemFirmwareDescriptor/SystemFirmwareDescriptor.inf

  MdeModulePkg/Universal/EsrtFmpDxe/EsrtFmpDxe.inf
  SignedCapsulePkg/Universal/SystemFirmwareUpdate/SystemFirmwareReportDxe.inf {
    <LibraryClasses>
      FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
  }
  SignedCapsulePkg/Universal/SystemFirmwareUpdate/SystemFirmwareUpdateDxe.inf {
    <LibraryClasses>
      FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
  }
  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
!endif
