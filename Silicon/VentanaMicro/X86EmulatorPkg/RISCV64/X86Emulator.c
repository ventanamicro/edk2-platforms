//
// Copyright (c) 2017, Linaro, Ltd. <ard.biesheuvel@linaro.org>
// Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//

#include "X86Emulator.h"

extern CONST UINT64 X86EmulatorThunk[];

VOID
EFIAPI
X86InterpreterSyncExceptionCallback (
  IN     EFI_EXCEPTION_TYPE   ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  EFI_SYSTEM_CONTEXT_RISCV64  *RiscV64Context;
  X86_IMAGE_RECORD            *Record;

  RiscV64Context = SystemContext.SystemContextRiscV64;

  // instruction permission faults or PC misalignment faults take us to the emu
  Record = FindImageRecord (RiscV64Context->SEPC);
  if (Record != NULL) {
    RiscV64Context->X28 = RiscV64Context->SEPC; /* ra->t3 register*/
    RiscV64Context->X29 = (UINT64)Record; /* t4 register */
    RiscV64Context->SEPC = (UINT64)X86EmulatorThunk; /* ra */
    return;
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
}
