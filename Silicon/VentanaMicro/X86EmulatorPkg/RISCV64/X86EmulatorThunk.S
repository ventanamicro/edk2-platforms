//
// Copyright (c) 2017, Linaro, Ltd. <ard.biesheuvel@linaro.org>
// Copyright (c) 2022, Ventana Micro Systems Inc. All Rights Reserved.<BR>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//

	.text
	.global		X86EmulatorThunk
X86EmulatorThunk:
	addi sp, sp, -176 /* Align 16-byte */
	sd	ra, 0(sp)
	sd	gp, 8(sp)
	sd	tp, 16(sp)
	sd	s1, 24(sp)
	sd	s2, 32(sp)
	sd	s3, 40(sp)
	sd	s4, 48(sp)
	sd	s5, 56(sp)
	sd	s6, 64(sp)
	sd	s7, 72(sp)
	sd	s8, 80(sp)
	sd	s9, 88(sp)
	sd	s10, 96(sp)
	sd	s11, 104(sp)
	sd	t0, 112(sp)
	sd	t1, 120(sp)
	sd	t2, 128(sp)
	sd	t3, 136(sp)
	sd	t4, 144(sp)
	sd	t5, 152(sp)
	sd	t6, 160(sp)
	sd	s0, 168(sp)
	addi	sp, sp, -64

	sd	a0, 0(sp)
	sd	a1, 8(sp)
	sd	a2, 16(sp)
	sd	a3, 24(sp)
	sd	a4, 32(sp)
	sd	a5, 40(sp)
	sd	a6, 48(sp)
	sd	a7, 56(sp)

	mv	a0, x28
	mv	a1, sp
	mv	a2, x29
	call	X86EmulatorVmEntry

	addi	sp, sp, 64
	ld	ra, 0(sp)
	ld	gp, 8(sp)
	ld	tp, 16(sp)
	ld	s1, 24(sp)
	ld	s2, 32(sp)
	ld	s3, 40(sp)
	ld	s4, 48(sp)
	ld	s5, 56(sp)
	ld	s6, 64(sp)
	ld	s7, 72(sp)
	ld	s8, 80(sp)
	ld	s9, 88(sp)
	ld	s10, 96(sp)
	ld	s11, 104(sp)
	ld	t0, 112(sp)
	ld	t1, 120(sp)
	ld	t2, 128(sp)
	ld	t3, 136(sp)
	ld	t4, 144(sp)
	ld	t5, 152(sp)
	ld	t6, 160(sp)
	ld 	s0, 168(sp)
	addi	sp, sp, 176

	ret
