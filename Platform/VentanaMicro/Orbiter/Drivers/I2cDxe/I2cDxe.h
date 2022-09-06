/** @file

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef I2C_DXE_H__
#define I2C_DXE_H__

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/TimerLib.h>

#include <Protocol/I2cMaster.h>
#include <Protocol/NonDiscoverableDevice.h>

extern EFI_COMPONENT_NAME2_PROTOCOL gI2cDriverComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL  gI2cDriverBinding;

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH              Vendor;
  UINT64                          MmioBase;
  EFI_DEVICE_PATH_PROTOCOL        End;
} I2C_DEVICE_PATH;
#pragma pack()

typedef struct {
  UINT32                          Signature;
  EFI_I2C_MASTER_PROTOCOL         I2cMaster;
  EFI_PHYSICAL_ADDRESS            MmioBase;
  I2C_DEVICE_PATH                 DevicePath;
  NON_DISCOVERABLE_DEVICE         *Dev;
  EFI_EVENT                       VirtualAddressChangeEvent;
  BOOLEAN                         Runtime;
} I2C_MASTER;

EFI_STATUS
I2cInit (
  IN      EFI_HANDLE              DriverBindingHandle,
  IN      EFI_HANDLE              ControllerHandle
  );

EFI_STATUS
I2cRelease (
  IN      EFI_HANDLE              DriverBindingHandle,
  IN      EFI_HANDLE              ControllerHandle
  );

#define I2C_SIGNATURE                 SIGNATURE_32 ('Q', 'I', '2', 'C')
#define I2C_FROM_THIS(a)              CR ((a), I2C_MASTER, \
                                         I2cMaster, I2C_SIGNATURE)

#define REFCLK_RATE                   FixedPcdGet32 (PcdI2cReferenceClock)

#define DIV_ROUND_UP(n, d)            (((n) + (d) - 1) / (d))

#ifndef BIT
#define BIT(nr) (1 << (nr))
#endif

/* Register offsets for the I2C device. */
#define CDNS_I2C_CR_OFFSET            0x00 /* Control Register, RW */
#define CDNS_I2C_SR_OFFSET            0x04 /* Status Register, RO */
#define CDNS_I2C_ADDR_OFFSET          0x08 /* I2C Address Register, RW */
#define CDNS_I2C_DATA_OFFSET          0x0C /* I2C Data Register, RW */
#define CDNS_I2C_ISR_OFFSET           0x10 /* IRQ Status Register, RW */
#define CDNS_I2C_XFER_SIZE_OFFSET     0x14 /* Transfer Size Register, RW */
#define CDNS_I2C_TIME_OUT_OFFSET      0x1C /* Time Out Register, RW */
#define CDNS_I2C_IMR_OFFSET           0x20 /* IRQ Mask Register, RO */
#define CDNS_I2C_IER_OFFSET           0x24 /* IRQ Enable Register, WO */
#define CDNS_I2C_IDR_OFFSET           0x28 /* IRQ Disable Register, WO */

/* Control Register Bit mask definitions */
#define CDNS_I2C_CR_HOLD              BIT(4) /* Hold Bus bit */
#define CDNS_I2C_CR_ACK_EN            BIT(3)
#define CDNS_I2C_CR_NEA               BIT(2)
#define CDNS_I2C_CR_MS                BIT(1)
/* Read or Write Master transfer 0 = Transmitter, 1 = Receiver */
#define CDNS_I2C_CR_RW                BIT(0)
/* 1 = Auto init FIFO to zeroes */
#define CDNS_I2C_CR_CLR_FIFO          BIT(6)
#define CDNS_I2C_CR_DIVA_SHIFT        14
#define CDNS_I2C_CR_DIVA_MASK         (3 << CDNS_I2C_CR_DIVA_SHIFT)
#define CDNS_I2C_CR_DIVB_SHIFT        8
#define CDNS_I2C_CR_DIVB_MASK         (0x3f << CDNS_I2C_CR_DIVB_SHIFT)

#define CDNS_I2C_CR_MASTER_EN_MASK  (CDNS_I2C_CR_NEA | \
                CDNS_I2C_CR_ACK_EN | \
                CDNS_I2C_CR_MS)

/* Status Register Bit mask definitions */
#define CDNS_I2C_SR_BA                BIT(8)
#define CDNS_I2C_SR_TXDV              BIT(6)
#define CDNS_I2C_SR_RXDV              BIT(5)
#define CDNS_I2C_SR_RXRW              BIT(3)

#define CDNS_I2C_DIVA_MAX             4
#define CDNS_I2C_DIVB_MAX             64

#define CDNS_I2C_TIMEOUT_MAX          0xFF

/*
 * I2C Interrupt Registers Bit mask definitions
 * All the four interrupt registers (Status/Mask/Enable/Disable) have the same
 * bit definitions.
 */
#define CDNS_I2C_IXR_ARB_LOST         BIT(9)
#define CDNS_I2C_IXR_RX_UNF           BIT(7)
#define CDNS_I2C_IXR_TX_OVF           BIT(6)
#define CDNS_I2C_IXR_RX_OVF           BIT(5)
#define CDNS_I2C_IXR_SLV_RDY          BIT(4)
#define CDNS_I2C_IXR_TO               BIT(3)
#define CDNS_I2C_IXR_NACK             BIT(2)
#define CDNS_I2C_IXR_DATA             BIT(1)
#define CDNS_I2C_IXR_COMP             BIT(0)

#define CDNS_I2C_IXR_ALL_INTR_MASK  (CDNS_I2C_IXR_ARB_LOST | \
           CDNS_I2C_IXR_RX_UNF | \
           CDNS_I2C_IXR_TX_OVF | \
           CDNS_I2C_IXR_RX_OVF | \
           CDNS_I2C_IXR_SLV_RDY | \
           CDNS_I2C_IXR_TO | \
           CDNS_I2C_IXR_NACK | \
           CDNS_I2C_IXR_DATA | \
           CDNS_I2C_IXR_COMP)

#define CDNS_I2C_IXR_ERR_INTR_MASK  (CDNS_I2C_IXR_ARB_LOST | \
           CDNS_I2C_IXR_RX_UNF | \
           CDNS_I2C_IXR_TX_OVF | \
           CDNS_I2C_IXR_RX_OVF | \
           CDNS_I2C_IXR_NACK)

#define CDNS_I2C_ENABLED_INTR_MASK  (CDNS_I2C_IXR_ARB_LOST | \
           CDNS_I2C_IXR_RX_UNF | \
           CDNS_I2C_IXR_TX_OVF | \
           CDNS_I2C_IXR_RX_OVF | \
           CDNS_I2C_IXR_NACK | \
           CDNS_I2C_IXR_DATA | \
           CDNS_I2C_IXR_COMP)

/*
 * I2C Address Register Bit mask definitions
 * Normal addressing mode uses [6:0] bits. Extended addressing mode uses [9:0]
 * bits. A write access to this register always initiates a transfer if the I2C
 * is in master mode.
 */
#define CDNS_I2C_ADDR_MASK            0x000003FF /* I2C Address Mask */

#define CDNS_I2C_FIFO_DEPTH           16
/* FIFO depth at which the DATA interrupt occurs */
#define CDNS_I2C_MAX_TRANSFER_SIZE    255
/* Transfer size in multiples of data interrupt depth */
#define CDNS_I2C_TRANSFER_SIZE        (CDNS_I2C_MAX_TRANSFER_SIZE - 3)

#endif /* I2C_DXE_H__ */
