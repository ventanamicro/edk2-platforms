/** @file
  Memory map definition of VENTANASYNTH platform.

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#define VENTANASYNTH_SYS_I2C_BASE              0x00000000 /* FIXME: */
#define VENTANASYNTH_SYS_I2C_SIZE              0x1000 /* FIXME: */

#define VENTANASYNTH_SYS_QSPI_BASE             0x00000000 /* FIXME: */
#define VENTANASYNTH_SYS_QSPI_APB_SIZE         0x4000
#define VENTANASYNTH_SYS_QSPI_SDMA_BASE        (VENTANASYNTH_SYS_QSPI_BASE + VENTANASYNTH_SYS_QSPI_APB_SIZE)
#define VENTANASYNTH_SYS_QSPI_SDMA_SIZE        0x4000
#define VENTANASYNTH_SYS_QSPI_SIZE             (VENTANASYNTH_SYS_QSPI_APB_SIZE + VENTANASYNTH_SYS_QSPI_SDMA_SIZE)

#define VENTANASYNTH_PCIE_ECAM                 0x30000000ULL//, 0x1040000000ULL, 0x1080000000ULL, 0x1090000000ULL
#define VENTANASYNTH_PCIE_ECAM_SIZE            0x10000000//, 0x40000000, 0x10000000, 0x10000000
#define VENTANASYNTH_PCIE_MMIO32               0x40000000//, 0xA0000000, 0xB0000000, 0xB4000000
#define VENTANASYNTH_PCIE_MMIO32_SIZE          0x40000000//, 0x10000000, 0x04000000, 0x04000000
#define VENTANASYNTH_PCIE_MMIO64               0x10000000000ULL//, 0xA0000000, 0xB0000000, 0xB4000000
#define VENTANASYNTH_PCIE_MMIO64_SIZE          0x400000000ULL//, 0x10000000, 0x04000000, 0x04000000

#endif /* MEMORY_MAP_H_ */