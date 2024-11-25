## @file
#  RISC-V EFI on Ventana Micro FPGA platform
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
  PLATFORM_NAME                  = VentanaSynth
  PLATFORM_GUID                  = F7977CEC-C924-4DA8-8DDB-27C002286F9A
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001c
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/VentanaMicro/VentanaSynth/VentanaSynth.fdf

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
  DEFINE NETWORK_SNP_ENABLE       = TRUE
  DEFINE NETWORK_IP6_ENABLE       = FALSE
  DEFINE NETWORK_TLS_ENABLE       = TRUE
  DEFINE NETWORK_HTTP_BOOT_ENABLE = TRUE
  DEFINE NETWORK_ISCSI_ENABLE     = FALSE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS = TRUE

  #
  # UPDATE/RECOVERY definition
  #
  DEFINE CAPSULE_ENABLE           = FALSE

  #
  # Disable PCI for bring up only
  #
  DEFINE TH_PCI_DISABLE           = FALSE

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
  gUefiCpuPkgTokenSpaceGuid.PcdCpuRiscVMmuMaxSatpMode|0
  # Serial Port
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x10020000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|115200
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|100000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|4
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterAccessWidth|32

  # Use emulator variable for now
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable | TRUE

  # ACPI
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiExposedTableVersions|0x20
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId|"Vetana"
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x20414E41544E4556 #VENTANA
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorId|0x4E544E56 # VNTN
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorRevision|0x000000001

  gUefiCpuPkgTokenSpaceGuid.PcdCpuCoreCrystalClockFrequency|100000000

[PcdsDynamicExDefault.common.DEFAULT]
!if $(CAPSULE_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSystemFmpCapsuleImageTypeIdGuid|{0xc7, 0x23, 0x91, 0x48, 0xe9, 0xff, 0x28, 0x49, 0x83, 0x1d, 0x25, 0x98, 0x0c, 0xac, 0xd6, 0xe1}
  gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareImageDescriptor|{0x0}|VOID*|0x100
  gEfiSignedCapsulePkgTokenSpaceGuid.PcdEdkiiSystemFirmwareFileGuid|{0xa2, 0x4e, 0x96, 0x67, 0x40, 0x63, 0x34, 0x4e, 0x94, 0xe5, 0xfe, 0x91, 0xc8, 0x1c, 0x18, 0xc3}
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
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  # Need "Real" HW real time clock
  RealTimeClockLib|EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf
  # Enable during CIH
  RiscVMmuLib|UefiCpuPkg/Library/BaseRiscVMmuLib/BaseRiscVMmuLib.inf
  RiscVFpuLib|UefiCpuPkg/Library/BaseRiscVFpuLib/BaseRiscVFpuLib.inf

  PlatformBootManagerLib|OvmfPkg/RiscVVirt/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  PlatformBmPrintScLib|OvmfPkg/Library/PlatformBmPrintScLib/PlatformBmPrintScLib.inf
  ResetSystemLib|OvmfPkg/RiscVVirt/Library/ResetSystemLib/BaseResetSystemLib.inf
  QemuBootOrderLib|OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf
  QemuLoadImageLib|OvmfPkg/Library/GenericQemuLoadImageLib/GenericQemuLoadImageLib.inf
  QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgMmioDxeLib.inf
  QemuFwCfgS3Lib|OvmfPkg/Library/QemuFwCfgS3Lib/BaseQemuFwCfgS3LibNull.inf
  QemuFwCfgSimpleParserLib|OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf
  ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf
  DxeRiscvMpxyLib|MdePkg/Library/DxeRiscvMpxyLib/DxeRiscvMpxy.inf
  DxeRiscvRasAgentClientLib|MdePkg/Library/DxeRiscvRasAgentClientLib/DxeRiscvRasAgentClientLib.inf

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
  DisplayUpdateProgressLib|MdeModulePkg/Library/DisplayUpdateProgressLibText/DisplayUpdateProgressLibText.inf
  RngLib|MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf
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
  Platform/VentanaMicro/VentanaSynth/Sec/SecMain.inf {
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
  Platform/VentanaMicro/VentanaSynth/AcpiTables/AcpiTables.inf
  Platform/VentanaMicro/VentanaSynth/Drivers/PlatformDxe/PlatformDxe.inf
  Platform/VentanaMicro/VentanaSynth/Drivers/SmbiosPlatformDxe/SmbiosPlatformDxe.inf

  #
  # Platfrom drivers
  #
  Platform/VentanaMicro/VentanaSynth/Drivers/PlatformDxe/PlatformDxe.inf
  MdeModulePkg/Universal/Acpi/AcpiHardwareErrorTableDxe/HardwareErrorSourceTableDxe.inf

  #
  # PCIe Support
  #
!if $(TH_PCI_DISABLE) == FALSE
  UefiCpuPkg/CpuMmio2Dxe/CpuMmio2Dxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      PciHostBridgeLib | Platform/VentanaMicro/VentanaSynth/Library/PciHostBridgeLib/PciHostBridgeLib.inf
      PciSegmentLib | Platform/VentanaMicro/VentanaSynth/Library/PciSegmentLibPci/PciSegmentLib.inf

  }
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
!endif

  #
  # Ethernet driver
  #
  Platform/VentanaMicro/VentanaSynth/Drivers/GigUndiDxe/GigUndiDxe.inf

  # X86Emulator
  Silicon/VentanaMicro/X86EmulatorPkg/X86Emulator.inf
  Silicon/VentanaMicro/VT1/Drivers/IommuExitDxe/IommuExitDxe.inf

!if $(CAPSULE_ENABLE) == TRUE
  # FMP image decriptor
  Platform/VentanaMicro/VentanaSynth/Capsule/SystemFirmwareDescriptor/SystemFirmwareDescriptor.inf
  Platform/VentanaMicro/VentanaSynth/Capsule/SystemFirmwareDescriptor/SystemFirmwareDescriptorSec.inf

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