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
  # Use emulator variable for now
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable | TRUE

  # Internal usage only. DO NOT UPSTREAM
  gOrbiterTokenSpaceGuid.PcdFixedRamdiskBase | 0xD0000000
  gOrbiterTokenSpaceGuid.PcdFixedRamdiskSize | 0x00600000

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include Silicon/VentanaMicro/VT1/VT1.dsc.inc

[LibraryClasses]
  # Need "Real" HW real time clock
  RealTimeClockLib|EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf
  # Enable during CIH
  # RealTimeClockLib|Platform/VentanaMicro/Orbiter/Library/Pcf85063aRealTimeClockLib/Pcf85063aRealTimeClockLib.inf

  PlatformBootManagerLib|OvmfPkg/RiscVVirt/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  PlatformBmPrintScLib|OvmfPkg/Library/PlatformBmPrintScLib/PlatformBmPrintScLib.inf
  ResetSystemLib|OvmfPkg/RiscVVirt/Library/ResetSystemLib/BaseResetSystemLib.inf
  QemuBootOrderLib|OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf
  QemuLoadImageLib|OvmfPkg/Library/GenericQemuLoadImageLib/GenericQemuLoadImageLib.inf
  QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgLibMmio.inf
  QemuFwCfgS3Lib|OvmfPkg/Library/QemuFwCfgS3Lib/BaseQemuFwCfgS3LibNull.inf
  QemuFwCfgSimpleParserLib|OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform.
#
################################################################################
[Components]
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
