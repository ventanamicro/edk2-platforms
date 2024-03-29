## @file
#  Definitions of Flash definition file on Thunderhill RISC-V platform
#
#  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
DEFINE BLOCK_SIZE                  = 0x1000

DEFINE FW_BASE_ADDRESS             = 0x400200000
DEFINE FW_SIZE                     = 0x00300000
DEFINE FW_BLOCKS                   = 0x300

DEFINE CODE_BASE_ADDRESS           = $(FW_BASE_ADDRESS)
DEFINE CODE_SIZE                   = 0x00240000
DEFINE CODE_BLOCKS                 = 0x240

DEFINE VARS_SIZE                    = 0x000C0000
DEFINE VARS_BLOCK_SIZE              = 0x40000
DEFINE VARS_BLOCKS                  = 0x3

#
# EFI Variable memory region.
# The total size of EFI Variable FD must include
# all of sub regions of EFI Variable
#
DEFINE VARS_OFFSET                   = $(CODE_SIZE)
DEFINE VARS_LIVE_SIZE                = 0x00040000
DEFINE VARS_FTW_WORKING_OFFSET       = $(VARS_OFFSET) + $(VARS_LIVE_SIZE)
DEFINE VARS_FTW_WORKING_SIZE         = 0x00040000
DEFINE VARS_FTW_SPARE_OFFSET         = $(VARS_FTW_WORKING_OFFSET) + $(VARS_FTW_WORKING_SIZE)
DEFINE VARS_FTW_SPARE_SIZE           = 0x00040000

SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamBase64 = 0x404FF0000
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamSize = 0x00010000

SET gThunderhillTokenSpaceGuid.PcdFdSize = $(FW_SIZE)
SET gThunderhillTokenSpaceGuid.PcdFdBaseAddress = $(CODE_BASE_ADDRESS)
