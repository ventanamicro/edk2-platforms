/** @file
  Memory map definition of Orbiter platform.

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#define ORBITER_SYS_I2C_BASE              0x00000000 /* FIXME: */
#define ORBITER_SYS_I2C_SIZE              0x1000 /* FIXME: */

#define ORBITER_SYS_QSPI_BASE             0x00000000 /* FIXME: */
#define ORBITER_SYS_QSPI_APB_SIZE         0x4000
#define ORBITER_SYS_QSPI_SDMA_BASE        (ORBITER_SYS_QSPI_BASE + ORBITER_SYS_QSPI_APB_SIZE)
#define ORBITER_SYS_QSPI_SDMA_SIZE        0x4000
#define ORBITER_SYS_QSPI_SIZE             (ORBITER_SYS_QSPI_APB_SIZE + ORBITER_SYS_QSPI_SDMA_SIZE)

#define ORBITER_PCIE_ECAM                 0x1080000000ULL, 0x1090000000ULL
#define ORBITER_PCIE_ECAM_SIZE            0x10000000, 0x10000000
#define ORBITER_PCIE_MMIO32               0xB0000000, 0xB4000000
#define ORBITER_PCIE_MMIO32_SIZE          0x04000000, 0x04000000
#define ORBITER_PCIE_MMIO64               0x6000000000ULL, 0x6400000000ULL
#define ORBITER_PCIE_MMIO64_SIZE          0x400000000ULL, 0x400000000ULL

#define ORBITER_CHIPLET0_IMSIC_BASE       0x40c00000
#define ORBITER_CHIPLET1_IMSIC_BASE       0x41c00000
#define ORBITER_CHIPLET_IMSIC_HART_SIZE   0x00010000

#define ORBITER_APLIC_BASE                0x42038000
#define ORBITER_APLIC_SIZE                0x00008000

#endif /* MEMORY_MAP_H_ */
