//
// Copyright (c) 2017, Linaro, Ltd. <ard.biesheuvel@linaro.org>
// Copyright (c) 2022-2023, Intel Corporation. All rights reserved.<BR>
// Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//

#include <Library/CpuExceptionHandlerLib.h>
#include "X86Emulator.h"

extern CONST UINT64 X86EmulatorThunk[];

#define INSN_C_ADDR_MASK  (-1ULL - 1)
#define INSN_ADDR_MASK    (-1ULL - 3)

STATIC
EFI_STATUS
RecoverPcFromCall (
  IN  EFI_SYSTEM_CONTEXT_RISCV64  *RiscV64Context,
  OUT EFI_PHYSICAL_ADDRESS        *ProgramCounter
  )
{
  UINT32  Insn;
  UINT64  Ra = RiscV64Context->X1;

  /*
   * SEPC always clears 0 and 1 as per IALIGN. Thus SEPC is always less
   * than PC.
   *
   * To recover PC, we look at the X1 (aka RA) register. We know
   * there was a branch to the PC value, but it could have been either
   * a compressed or a regular jalr, so we look at RA - 2 and RA - 4,
   * decode the instruction and reconstruct the PC value.
   *
   * Of course, the reconstructed PC value should match SEPC.
   *
   * If SEPC is ever larger than PC, then:
   * - Must have been using illegal instruction handler (not MMU)
   * - The actual instruction at PC is unfortunately a valid RISC-V
   *   instruction and it's been executed, randomly corrupting something
   *   (but at least not crashing).
   */

  Insn = *(UINT16 *)(Ra - 2);
  if (((Insn & 0x3) == 2) &&         // op == C2
      ((Insn & 0xf000) == 0x9000) && // funct4 == c.jalr
      ((Insn & 0xf80) != 0) &&       // rs1
      ((Insn & 0x7c) == 0))          // rs2
  {
    UINTN  Rs = (Insn >> 7) & 0x1F;
    *ProgramCounter = (&RiscV64Context->X0)[Rs];

    return EFI_SUCCESS;
  }

  if (Ra % 4) {
    /*
     * It was definitely an (unknown) compressed instruction.
     */
    DEBUG ((
      DEBUG_ERROR,
      "Unknown PC: unknown compressed instruction 0x%x at RA - 2 = 0x%lx\n",
      Insn,
      Ra - 2
      ));
    return EFI_NOT_FOUND;
  }

  Insn = *(UINT32 *)(Ra - 4);
  if (((Insn & 0x7f) == 0x67) &&  // opcode == jalr
      ((Insn & 0xf80) == 0x80) && // rd == x1
      ((Insn & 0x3000) == 0))     // func3
  {
    struct {
      signed int    x : 12;
    } imm12;
    UINTN  Rs = (Insn >> 15) & 0x1F;

    imm12.x         = Insn >> 20;
    *ProgramCounter = (&RiscV64Context->X0)[Rs] + imm12.x;

    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_ERROR,
    "Unknown PC: unknown instruction 0x%x @ RA - 4 = 0x%lx\n",
    Insn,
    Ra - 4
    ));

  return EFI_NOT_FOUND;
}

VOID
EFIAPI
X86InterpreterSyncExceptionCallback (
  IN     EFI_EXCEPTION_TYPE   ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  EFI_SYSTEM_CONTEXT_RISCV64  *RiscV64Context;
  X86_IMAGE_RECORD            *Record;
  EFI_STATUS                   Status;

  RiscV64Context = SystemContext.SystemContextRiscV64;

  // instruction permission faults or PC misalignment faults take us to the emu
  Record = FindImageRecord (RiscV64Context->SEPC);
  if (Record != NULL) {
    /*
     * SEPC always has bits 0 and 1 cleared as per
     * IALIGN. So it's good enough to validate the
     * PC came from the emulated image, but not good
     * enough for transfer control. We have to recover
     * the address by decoding the instruction that
     * made the call.
     */
    Status = RecoverPcFromCall (
                RiscV64Context,
                &RiscV64Context->X28 /* ra->t3 register */
                );
    if (!EFI_ERROR (Status)) {
      RiscV64Context->X29 = (UINT64)Record; /* t4 register */
      RiscV64Context->SEPC = (UINT64)X86EmulatorThunk; /* ra */
      return;
    }
  }

  // check whether the exception occurred in the JITed code
  if (RiscV64Context->X1 >= (UINTN)static_code_gen_buffer &&
      RiscV64Context->X1 < (UINTN)static_code_gen_buffer +
                            EFI_PAGES_TO_SIZE (CODE_GEN_BUFFER_PAGES)) {
    //
    // We can't handle this exception. Try to produce some meaningful
    // diagnostics regarding the X86 code this maps onto.
    //
    DEBUG ((DEBUG_ERROR, "Exception occurred during emulation:\n"));
    dump_x86_state();
  }
  DumpCpuContext (ExceptionType, SystemContext);
  CpuBreakpoint ();
}
