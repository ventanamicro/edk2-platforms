/*
 * Tiny Code Generator for QEMU
 *
 * Copyright (c) 2018 SiFive, Inc
 * Copyright (c) 2008-2009 Arnaud Patard <arnaud.patard@rtp-net.org>
 * Copyright (c) 2009 Aurelien Jarno <aurelien@aurel32.net>
 * Copyright (c) 2008 Fabrice Bellard
 *
 * Based on i386/tcg-target.c and mips/tcg-target.c
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#if CONFIG_DEBUG_TCG
static const char * const tcg_target_reg_names[TCG_TARGET_NB_REGS] = {
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "s0",
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6"
};
#endif

static const int tcg_target_reg_alloc_order[] = {
    /* Call saved registers */
    /* TCG_REG_S0 reservered for TCG_AREG0 */
    TCG_REG_S1,
    TCG_REG_S2,
    TCG_REG_S3,
    TCG_REG_S4,
    TCG_REG_S5,
    TCG_REG_S6,
    TCG_REG_S7,
    TCG_REG_S8,
    TCG_REG_S9,
    TCG_REG_S10,
    TCG_REG_S11,

    /* Call clobbered registers */
    TCG_REG_T0,
    TCG_REG_T1,
    TCG_REG_T2,
    TCG_REG_T3,
    TCG_REG_T4,
    TCG_REG_T5,
    TCG_REG_T6,

    /* Argument registers */
    TCG_REG_A0,
    TCG_REG_A1,
    TCG_REG_A2,
    TCG_REG_A3,
    TCG_REG_A4,
    TCG_REG_A5,
    TCG_REG_A6,
    TCG_REG_A7,
};

static const int tcg_target_call_iarg_regs[] = {
    TCG_REG_A0,
    TCG_REG_A1,
    TCG_REG_A2,
    TCG_REG_A3,
    TCG_REG_A4,
    TCG_REG_A5,
    TCG_REG_A6,
    TCG_REG_A7,
};

static const int tcg_target_call_oarg_regs[] = {
    TCG_REG_A0,
    TCG_REG_A1,
};

static int tcg_target_get_call_iarg_regs_count(int flags)
{
    return sizeof(tcg_target_call_iarg_regs) / sizeof(tcg_target_call_iarg_regs[0]);
}

#define MAKE_64BIT_MASK(shift, length) \
    (((~0ULL) >> (64 - (length))) << (shift))

#define ALL_GENERAL_REGS      MAKE_64BIT_MASK(0, 32)
/*
 * For softmmu, we need to avoid conflicts with the first 5
 * argument registers to call the helper.  Some of these are
 * also used for the tlb lookup.
 */
#ifdef CONFIG_SOFTMMU
#define SOFTMMU_RESERVE_REGS  MAKE_64BIT_MASK(TCG_REG_A0, 5)
#else
#define SOFTMMU_RESERVE_REGS  0
#endif

#define tcg_splitwx_to_rx(x) (x)

static inline ptrdiff_t tcg_ptr_byte_diff(const void *a, const void *b)
{
    return a - b;
}

static inline uint64_t sextract32u(uint32_t value, int start, int length)
{
    assert(start >= 0 && length > 0 && length <= 32 - start);
    /* Note that this implementation relies on right shift of signed
     * integers being an arithmetic shift.
     */
    return ((uint32_t)(value << (32 - length - start))) >> (32 - length);
}

static inline int64_t sextract64(uint64_t value, int start, int length)
{
    assert(start >= 0 && length > 0 && length <= 64 - start);
    /* Note that this implementation relies on right shift of signed
     * integers being an arithmetic shift.
     */
    return ((int64_t)(value << (64 - length - start))) >> (64 - length);
}

static inline tcg_target_long sextreg(tcg_target_long val, int pos, int len)
{
    return sextract64(val, pos, len);
}

/* parse target specific constraints */
static int target_parse_constraint(TCGArgConstraint *ct,
                                   const char **pct_str)
{
    const char *ct_str = *pct_str;

    switch (ct_str[0]) {
    case 'r':
        ct->ct |= TCG_CT_REG;
        tcg_regset_set32(ct->u.regs, 0, (1ULL << TCG_TARGET_NB_REGS) - 1);
        break;
    case 'l':
        ct->ct |= TCG_CT_REG;
        tcg_regset_set32(ct->u.regs, 0, (1ULL << TCG_TARGET_NB_REGS) - 1);
#ifdef CONFIG_SOFTMMU
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_A0);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_A1);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_A2);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_A3);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_A4);
#endif
        break;
    default:
        return -1;
    }

    ct_str++;
    *pct_str = ct_str;
    return 0;
}

/* test if a constant matches the constraint */
static bool tcg_target_const_match(tcg_target_long val,
                                         const TCGArgConstraint *arg_ct)
{
    int ct = arg_ct->ct;

    if (ct & TCG_CT_CONST) {
        return 1;
    }
    return 0;
}

/**
 * tcg_pcrel_diff
 * @s: the tcg context
 * @target: address of the target
 *
 * Produce a pc-relative difference, from the current code_ptr
 * to the destination address.
 */

static inline ptrdiff_t tcg_pcrel_diff(TCGContext *s, const void *target)
{
    return tcg_ptr_byte_diff(target, tcg_splitwx_to_rx(s->code_ptr));
}

/*
 * RISC-V Base ISA opcodes (IM)
 */

typedef enum {
    OPC_ADD = 0x33,
    OPC_ADDI = 0x13,
    OPC_AND = 0x7033,
    OPC_ANDI = 0x7013,
    OPC_AUIPC = 0x17,
    OPC_BEQ = 0x63,
    OPC_BGE = 0x5063,
    OPC_BGEU = 0x7063,
    OPC_BLT = 0x4063,
    OPC_BLTU = 0x6063,
    OPC_BNE = 0x1063,
    OPC_DIV = 0x2004033,
    OPC_DIVU = 0x2005033,
    OPC_JAL = 0x6f,
    OPC_JALR = 0x67,
    OPC_LB = 0x3,
    OPC_LBU = 0x4003,
    OPC_LD = 0x3003,
    OPC_LH = 0x1003,
    OPC_LHU = 0x5003,
    OPC_LUI = 0x37,
    OPC_LW = 0x2003,
    OPC_LWU = 0x6003,
    OPC_MUL = 0x2000033,
    OPC_MULH = 0x2001033,
    OPC_MULHSU = 0x2002033,
    OPC_MULHU = 0x2003033,
    OPC_OR = 0x6033,
    OPC_ORI = 0x6013,
    OPC_REM = 0x2006033,
    OPC_REMU = 0x2007033,
    OPC_SB = 0x23,
    OPC_SD = 0x3023,
    OPC_SH = 0x1023,
    OPC_SLL = 0x1033,
    OPC_SLLI = 0x1013,
    OPC_SLT = 0x2033,
    OPC_SLTI = 0x2013,
    OPC_SLTIU = 0x3013,
    OPC_SLTU = 0x3033,
    OPC_SRA = 0x40005033,
    OPC_SRAI = 0x40005013,
    OPC_SRL = 0x5033,
    OPC_SRLI = 0x5013,
    OPC_SUB = 0x40000033,
    OPC_SW = 0x2023,
    OPC_XOR = 0x4033,
    OPC_XORI = 0x4013,

    OPC_ADDIW = 0x1b,
    OPC_ADDW = 0x3b,
    OPC_DIVUW = 0x200503b,
    OPC_DIVW = 0x200403b,
    OPC_MULW = 0x200003b,
    OPC_REMUW = 0x200703b,
    OPC_REMW = 0x200603b,
    OPC_SLLIW = 0x101b,
    OPC_SLLW = 0x103b,
    OPC_SRAIW = 0x4000501b,
    OPC_SRAW = 0x4000503b,
    OPC_SRLIW = 0x501b,
    OPC_SRLW = 0x503b,
    OPC_SUBW = 0x4000003b,

    OPC_FENCE = 0x0000000f,
} RISCVInsn;

/*
 * RISC-V immediate and instruction encoders (excludes 16-bit RVC)
 */

/* Type-R */

static int32_t encode_r(RISCVInsn opc, TCGReg rd, TCGReg rs1, TCGReg rs2)
{
    return opc | (rd & 0x1f) << 7 | (rs1 & 0x1f) << 15 | (rs2 & 0x1f) << 20;
}

/* Type-I */

static int32_t encode_imm12(uint32_t imm)
{
    return (imm & 0xfff) << 20;
}

static int32_t encode_i(RISCVInsn opc, TCGReg rd, TCGReg rs1, uint32_t imm)
{
    return opc | (rd & 0x1f) << 7 | (rs1 & 0x1f) << 15 | encode_imm12(imm);
}

/* Type-S */

static int32_t encode_simm12(uint32_t imm)
{
    int32_t ret = 0;

    ret |= (imm & 0xFE0) << 20;
    ret |= (imm & 0x1F) << 7;

    return ret;
}

static int32_t encode_s(RISCVInsn opc, TCGReg rs1, TCGReg rs2, uint32_t imm)
{
    return opc | (rs1 & 0x1f) << 15 | (rs2 & 0x1f) << 20 | encode_simm12(imm);
}

/* Type-SB */

static int32_t encode_sbimm12(uint32_t imm)
{
    int32_t ret = 0;

    ret |= (imm & 0x1000) << 19;
    ret |= (imm & 0x7e0) << 20;
    ret |= (imm & 0x1e) << 7;
    ret |= (imm & 0x800) >> 4;

    return ret;
}

static int32_t encode_sb(RISCVInsn opc, TCGReg rs1, TCGReg rs2, uint32_t imm)
{
    return opc | (rs1 & 0x1f) << 15 | (rs2 & 0x1f) << 20 | encode_sbimm12(imm);
}

/* Type-U */

static int32_t encode_uimm20(uint32_t imm)
{
    return imm & 0xfffff000;
}

static int32_t encode_u(RISCVInsn opc, TCGReg rd, uint32_t imm)
{
    return opc | (rd & 0x1f) << 7 | encode_uimm20(imm);
}

/* Type-UJ */

static int32_t encode_ujimm20(uint32_t imm)
{
    int32_t ret = 0;

    ret |= (imm & 0x0007fe) << (21 - 1);
    ret |= (imm & 0x000800) << (20 - 11);
    ret |= (imm & 0x0ff000) << (12 - 12);
    ret |= (imm & 0x100000) << (31 - 20);

    return ret;
}

static int32_t encode_uj(RISCVInsn opc, TCGReg rd, uint32_t imm)
{
    return opc | (rd & 0x1f) << 7 | encode_ujimm20(imm);
}

/*
 * RISC-V instruction emitters
 */

static void tcg_out_opc_imm(TCGContext *s, RISCVInsn opc,
                            TCGReg rd, TCGReg rs1, TCGArg imm)
{
    tcg_out32(s, encode_i(opc, rd, rs1, imm));
}

static void tcg_out_opc_reg(TCGContext *s, RISCVInsn opc,
                            TCGReg rd, TCGReg rs1, TCGReg rs2)
{
    tcg_out32(s, encode_r(opc, rd, rs1, rs2));
}

static void tcg_out_opc_store(TCGContext *s, RISCVInsn opc,
                              TCGReg rs1, TCGReg rs2, uint32_t imm)
{
    tcg_out32(s, encode_s(opc, rs1, rs2, imm));
}

static void tcg_out_opc_branch(TCGContext *s, RISCVInsn opc,
                               TCGReg rs1, TCGReg rs2, uint32_t imm)
{
    tcg_out32(s, encode_sb(opc, rs1, rs2, imm));
}

static void tcg_out_opc_upper(TCGContext *s, RISCVInsn opc,
                              TCGReg rd, uint32_t imm)
{
    tcg_out32(s, encode_u(opc, rd, imm));
}

static void tcg_out_opc_jump(TCGContext *s, RISCVInsn opc,
                             TCGReg rd, uint32_t imm)
{
    tcg_out32(s, encode_uj(opc, rd, imm));
}

static void tcg_out_nop_fill(tcg_insn_unit *p, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        p[i] = encode_i(OPC_ADDI, TCG_REG_ZERO, TCG_REG_ZERO, 0);
    }
}

/*
 * Relocations
 */

static bool reloc_sbimm12(tcg_insn_unit *src_rw, const tcg_insn_unit *target)
{
    const tcg_insn_unit *src_rx = tcg_splitwx_to_rx(src_rw);
    intptr_t offset = (intptr_t)target - (intptr_t)src_rx;

    tcg_debug_assert((offset & 1) == 0);
    if (offset == sextreg(offset, 0, 12)) {
        *src_rw |= encode_sbimm12(offset);
        return true;
    }

    assert(false);
    return false;
}

static bool reloc_jimm20(tcg_insn_unit *src_rw, const tcg_insn_unit *target)
{
    const tcg_insn_unit *src_rx = tcg_splitwx_to_rx(src_rw);
    intptr_t offset = (intptr_t)target - (intptr_t)src_rx;

    tcg_debug_assert((offset & 1) == 0);
    if (offset == sextreg(offset, 0, 20)) {
        *src_rw |= encode_ujimm20(offset);
        return true;
    }

    assert(false);
    return false;
}

static bool reloc_call(tcg_insn_unit *src_rw, const tcg_insn_unit *target)
{
    const tcg_insn_unit *src_rx = tcg_splitwx_to_rx(src_rw);
    intptr_t offset = (intptr_t)target - (intptr_t)src_rx;
    int32_t lo = sextreg(offset, 0, 12);
    int32_t hi = offset - lo;

    if (offset == hi + lo) {
        src_rw[0] |= encode_uimm20(hi);
        src_rw[1] |= encode_imm12(lo);
        return true;
    }

    assert(false);
    return false;
}

#define R_RISCV_BRANCH        16
#define R_RISCV_JAL           17
#define R_RISCV_CALL          18

static void patch_reloc(uint8_t *code_ptr, int type, 
                        tcg_target_long value, tcg_target_long addend)

{
    tcg_debug_assert(addend == 0);
    switch (type) {
    case R_RISCV_BRANCH:
        reloc_sbimm12((tcg_insn_unit *)code_ptr, (tcg_insn_unit *)value);
        break;
    case R_RISCV_JAL:
        reloc_jimm20((tcg_insn_unit *)code_ptr, (tcg_insn_unit *)value);
        break;
    case R_RISCV_CALL:
        reloc_call((tcg_insn_unit *)code_ptr, (tcg_insn_unit *)value);
        break;
    default:
        assert(false);
    }
}

/*
 * TCG intrinsics
 */

static bool tcg_out_mov(TCGContext *s, TCGType type, TCGReg ret, TCGReg arg)
{
    if (ret == arg) {
        return true;
    }
    switch (type) {
    case TCG_TYPE_I32:
    case TCG_TYPE_I64:
        tcg_out_opc_imm(s, OPC_ADDI, ret, arg, 0);
        break;
    default:
        assert(false);
    }
    return true;
}

static void tcg_out_movi_aux(TCGContext *s, TCGReg rd,
                         uint32_t val)
{
    uint32_t lo, hi;

    lo = sextract32u(val, 0, 12);
    hi = val - lo;

    tcg_out_opc_upper(s, OPC_LUI, rd, hi);
    tcg_out_opc_imm(s, OPC_SLLI, rd, rd, 32);
    tcg_out_opc_imm(s, OPC_SRLI, rd, rd, 32);
    if (lo) {
        lo <<= 12;
        tcg_out_opc_upper(s, OPC_LUI, TCG_REG_TMP4, lo);
        tcg_out_opc_imm(s, OPC_SLLI, TCG_REG_TMP4, TCG_REG_TMP4, 32);
        tcg_out_opc_imm(s, OPC_SRLI, TCG_REG_TMP4, TCG_REG_TMP4, 32);
        tcg_out_opc_imm(s, OPC_SRLI, TCG_REG_TMP4, TCG_REG_TMP4, 12);
        tcg_out_opc_reg(s, OPC_ADD, rd, rd, TCG_REG_TMP4);
    }
}

static void tcg_out_movi(TCGContext *s, TCGType type, TCGReg rd,
                          tcg_target_long val)
{
    TCGReg TmpReg = TCG_REG_TMP3;

    tcg_out_movi_aux(s, rd, val & 0xffffffff);
    if (val >> 32) {
        tcg_out_movi_aux(s, TmpReg, val >> 32);
        tcg_out_opc_imm(s, OPC_SLLI, TmpReg, TmpReg, 32);
        tcg_out_opc_reg(s, OPC_ADD, rd, rd, TmpReg);
    }
}

static void tcg_out_ext8u(TCGContext *s, TCGReg ret, TCGReg arg)
{
    tcg_out_opc_imm(s, OPC_ANDI, ret, arg, 0xff);
}

static void tcg_out_ext16u(TCGContext *s, TCGReg ret, TCGReg arg)
{
    tcg_out_opc_imm(s, OPC_SLLIW, ret, arg, 16);
    tcg_out_opc_imm(s, OPC_SRLIW, ret, ret, 16);
}

static void tcg_out_ext32u(TCGContext *s, TCGReg ret, TCGReg arg)
{
    tcg_out_opc_imm(s, OPC_SLLI, ret, arg, 32);
    tcg_out_opc_imm(s, OPC_SRLI, ret, ret, 32);
}

static void tcg_out_ext8s(TCGContext *s, TCGReg ret, TCGReg arg)
{
    tcg_out_opc_imm(s, OPC_SLLIW, ret, arg, 24);
    tcg_out_opc_imm(s, OPC_SRAIW, ret, ret, 24);
}

static void tcg_out_ext16s(TCGContext *s, TCGReg ret, TCGReg arg)
{
    tcg_out_opc_imm(s, OPC_SLLIW, ret, arg, 16);
    tcg_out_opc_imm(s, OPC_SRAIW, ret, ret, 16);
}

static void tcg_out_ext32s(TCGContext *s, TCGReg ret, TCGReg arg)
{
    tcg_out_opc_imm(s, OPC_ADDIW, ret, arg, 0);
}

static void tcg_out_ldst(TCGContext *s, RISCVInsn opc, TCGReg data,
                         TCGReg addr, intptr_t offset)
{
    intptr_t imm12 = sextreg(offset, 0, 12);
    if (offset != imm12) {
        intptr_t diff = offset - (uintptr_t)s->code_ptr;

        if (addr == TCG_REG_ZERO && diff == (int32_t)diff) {
            imm12 = sextreg(diff, 0, 12);
            tcg_out_opc_upper(s, OPC_AUIPC, TCG_REG_TMP2, diff - imm12);
        } else {
            tcg_out_movi(s, TCG_TYPE_PTR, TCG_REG_TMP2, offset - imm12);
            if (addr != TCG_REG_ZERO) {
                tcg_out_opc_reg(s, OPC_ADD, TCG_REG_TMP2, TCG_REG_TMP2, addr);
            }
        }
        addr = TCG_REG_TMP2;
    }

    switch (opc) {
    case OPC_SB:
    case OPC_SH:
    case OPC_SW:
    case OPC_SD:
        tcg_out_opc_store(s, opc, addr, data, imm12);
        break;
    case OPC_LB:
    case OPC_LBU:
    case OPC_LH:
    case OPC_LHU:
    case OPC_LW:
    case OPC_LWU:
    case OPC_LD:
        tcg_out_opc_imm(s, opc, data, addr, imm12);
        break;
    default:
        tcg_debug_assert(false);
    }
}

static void tcg_out_ld(TCGContext *s, TCGType type, TCGReg arg,
                       TCGReg arg1, intptr_t arg2)
{
    bool is32bit = (type == TCG_TYPE_I32);
    tcg_out_ldst(s, is32bit ? OPC_LW : OPC_LD, arg, arg1, arg2);
}

static void tcg_out_st(TCGContext *s, TCGType type, TCGReg arg,
                       TCGReg arg1, intptr_t arg2)
{
    bool is32bit = (type == TCG_TYPE_I32);
    tcg_out_ldst(s, is32bit ? OPC_SW : OPC_SD, arg, arg1, arg2);
}

static bool tcg_out_sti(TCGContext *s, TCGType type, TCGArg val,
                        TCGReg base, intptr_t ofs)
{
    if (val == 0) {
        tcg_out_st(s, type, TCG_REG_ZERO, base, ofs);
        return true;
    }
    return false;
}

static const struct {
    RISCVInsn op;
    bool swap;
} tcg_brcond_to_riscv[] = {
    [TCG_COND_EQ] =  { OPC_BEQ,  false },
    [TCG_COND_NE] =  { OPC_BNE,  false },
    [TCG_COND_LT] =  { OPC_BLT,  false },
    [TCG_COND_GE] =  { OPC_BGE,  false },
    [TCG_COND_LE] =  { OPC_BGE,  true  },
    [TCG_COND_GT] =  { OPC_BLT,  true  },
    [TCG_COND_LTU] = { OPC_BLTU, false },
    [TCG_COND_GEU] = { OPC_BGEU, false },
    [TCG_COND_LEU] = { OPC_BGEU, true  },
    [TCG_COND_GTU] = { OPC_BLTU, true  }
};

static void tcg_out_brcond(TCGContext *s, TCGCond cond, TCGReg arg1,
                           TCGReg arg2, int l)
{
    RISCVInsn op = tcg_brcond_to_riscv[cond].op;

    tcg_debug_assert(op != 0);

    if (tcg_brcond_to_riscv[cond].swap) {
        TCGReg t = arg1;
        arg1 = arg2;
        arg2 = t;
    }

    tcg_out_opc_branch(s, op, arg1, arg2, 0);
    tcg_out_reloc(s, (uint8_t *)((tcg_insn_unit *)(s->code_ptr) - 1), R_RISCV_BRANCH, l, 0);
}

static void tcg_out_setcond(TCGContext *s, TCGCond cond, TCGReg ret,
                            TCGReg arg1, TCGReg arg2)
{
    switch (cond) {
    case TCG_COND_EQ:
        tcg_out_opc_reg(s, OPC_SUB, ret, arg1, arg2);
        tcg_out_opc_imm(s, OPC_SLTIU, ret, ret, 1);
        break;
    case TCG_COND_NE:
        tcg_out_opc_reg(s, OPC_SUB, ret, arg1, arg2);
        tcg_out_opc_reg(s, OPC_SLTU, ret, TCG_REG_ZERO, ret);
        break;
    case TCG_COND_LT:
        tcg_out_opc_reg(s, OPC_SLT, ret, arg1, arg2);
        break;
    case TCG_COND_GE:
        tcg_out_opc_reg(s, OPC_SLT, ret, arg1, arg2);
        tcg_out_opc_imm(s, OPC_XORI, ret, ret, 1);
        break;
    case TCG_COND_LE:
        tcg_out_opc_reg(s, OPC_SLT, ret, arg2, arg1);
        tcg_out_opc_imm(s, OPC_XORI, ret, ret, 1);
        break;
    case TCG_COND_GT:
        tcg_out_opc_reg(s, OPC_SLT, ret, arg2, arg1);
        break;
    case TCG_COND_LTU:
        tcg_out_opc_reg(s, OPC_SLTU, ret, arg1, arg2);
        break;
    case TCG_COND_GEU:
        tcg_out_opc_reg(s, OPC_SLTU, ret, arg1, arg2);
        tcg_out_opc_imm(s, OPC_XORI, ret, ret, 1);
        break;
    case TCG_COND_LEU:
        tcg_out_opc_reg(s, OPC_SLTU, ret, arg2, arg1);
        tcg_out_opc_imm(s, OPC_XORI, ret, ret, 1);
        break;
    case TCG_COND_GTU:
        tcg_out_opc_reg(s, OPC_SLTU, ret, arg2, arg1);
        break;
    default:
         tcg_debug_assert(false);
         break;
     }
}

static void tcg_out_brcond2(TCGContext *s, TCGCond cond, TCGReg al, TCGReg ah,
                            TCGReg bl, TCGReg bh, int l)
{
    /* todo */
    tcg_debug_assert(false);
}

static void tcg_out_setcond2(TCGContext *s, TCGCond cond, TCGReg ret,
                             TCGReg al, TCGReg ah, TCGReg bl, TCGReg bh)
{
    /* todo */
    tcg_debug_assert(false);
}

static void tcg_out_callr(TCGContext *s, TCGReg arg)
{
    tcg_out_opc_reg(s, OPC_JALR, TCG_REG_RA, arg, 0);
}

static void tcg_out_call_int(TCGContext *s, const tcg_insn_unit *arg, bool tail)
{
    TCGReg link = tail ? TCG_REG_ZERO : TCG_REG_RA;
    ptrdiff_t offset = tcg_pcrel_diff(s, arg);

    tcg_debug_assert((offset & 1) == 0);
    if (offset == sextreg(offset, 0, 20)) {
        /* short jump: -2097150 to 2097152 */
        tcg_out_opc_jump(s, OPC_JAL, link, offset);
    } else if (offset == (int32_t)offset) {
        /* long jump: -2147483646 to 2147483648 */
        tcg_out_opc_upper(s, OPC_AUIPC, TCG_REG_TMP0, 0);
        tcg_out_opc_imm(s, OPC_JALR, link, TCG_REG_TMP0, 0);
        reloc_call((tcg_insn_unit *)(s->code_ptr) - 2, arg);
    } else {
        /* far jump: 64-bit */
        tcg_target_long imm = sextreg((tcg_target_long)arg, 0, 12);
        tcg_target_long base = (tcg_target_long)arg - imm;
        tcg_out_movi(s, TCG_TYPE_PTR, TCG_REG_TMP0, base);
        tcg_out_opc_imm(s, OPC_JALR, link, TCG_REG_TMP0, imm);
    }
}

static void tcg_out_call(TCGContext *s, const tcg_insn_unit *arg)
{
    tcg_out_call_int(s, arg, false);
}

/*
 * Load/store and TLB
 */

#if defined(CONFIG_SOFTMMU)
/* helper signature: helper_ret_ld_mmu(CPUState *env, target_ulong addr,
 *                                     MemOpIdx oi, uintptr_t ra)
 */
static void * const qemu_ld_helpers[MO_SSIZE + 1] = {
    [MO_UB] = helper_ret_ldub_mmu,
    [MO_SB] = helper_ret_ldsb_mmu,
#if HOST_BIG_ENDIAN
    [MO_UW] = helper_be_lduw_mmu,
    [MO_SW] = helper_be_ldsw_mmu,
    [MO_UL] = helper_be_ldul_mmu,
    [MO_SL] = helper_be_ldsl_mmu,
    [MO_UQ] = helper_be_ldq_mmu,
#else
    [MO_UW] = helper_le_lduw_mmu,
    [MO_SW] = helper_le_ldsw_mmu,
    [MO_UL] = helper_le_ldul_mmu,
    [MO_SL] = helper_le_ldsl_mmu,
    [MO_UQ] = helper_le_ldq_mmu,
#endif
};

/* helper signature: helper_ret_st_mmu(CPUState *env, target_ulong addr,
 *                                     uintxx_t val, MemOpIdx oi,
 *                                     uintptr_t ra)
 */
static void * const qemu_st_helpers[MO_SIZE + 1] = {
    [MO_8]   = helper_ret_stb_mmu,
#if HOST_BIG_ENDIAN
    [MO_16] = helper_be_stw_mmu,
    [MO_32] = helper_be_stl_mmu,
    [MO_64] = helper_be_stq_mmu,
#else
    [MO_16] = helper_le_stw_mmu,
    [MO_32] = helper_le_stl_mmu,
    [MO_64] = helper_le_stq_mmu,
#endif
};

/* We don't support oversize guests */
QEMU_BUILD_BUG_ON(TCG_TARGET_REG_BITS < TARGET_LONG_BITS);

/* We expect to use a 12-bit negative offset from ENV.  */
QEMU_BUILD_BUG_ON(TLB_MASK_TABLE_OFS(0) > 0);
QEMU_BUILD_BUG_ON(TLB_MASK_TABLE_OFS(0) < -(1 << 11));

static void tcg_out_goto(TCGContext *s, const tcg_insn_unit *target)
{
    tcg_out_opc_jump(s, OPC_JAL, TCG_REG_ZERO, 0);
    bool ok = reloc_jimm20(s->code_ptr - 1, target);
    tcg_debug_assert(ok);
}

static void tcg_out_tlb_load(TCGContext *s, TCGReg addrl,
                             TCGReg addrh, MemOpIdx oi,
                             tcg_insn_unit **label_ptr, bool is_load)
{
    MemOp opc = get_memop(oi);
    unsigned s_bits = opc & MO_SIZE;
    unsigned a_bits = get_alignment_bits(opc);
    tcg_target_long compare_mask;
    int mem_index = get_mmuidx(oi);
    int fast_ofs = TLB_MASK_TABLE_OFS(mem_index);
    int mask_ofs = fast_ofs + offsetof(CPUTLBDescFast, mask);
    int table_ofs = fast_ofs + offsetof(CPUTLBDescFast, table);
    TCGReg mask_base = TCG_AREG0, table_base = TCG_AREG0;

    tcg_out_ld(s, TCG_TYPE_PTR, TCG_REG_TMP0, mask_base, mask_ofs);
    tcg_out_ld(s, TCG_TYPE_PTR, TCG_REG_TMP1, table_base, table_ofs);

    tcg_out_opc_imm(s, OPC_SRLI, TCG_REG_TMP2, addrl,
                    TARGET_PAGE_BITS - CPU_TLB_ENTRY_BITS);
    tcg_out_opc_reg(s, OPC_AND, TCG_REG_TMP2, TCG_REG_TMP2, TCG_REG_TMP0);
    tcg_out_opc_reg(s, OPC_ADD, TCG_REG_TMP2, TCG_REG_TMP2, TCG_REG_TMP1);

    /* Load the tlb comparator and the addend.  */
    tcg_out_ld(s, TCG_TYPE_TL, TCG_REG_TMP0, TCG_REG_TMP2,
               is_load ? offsetof(CPUTLBEntry, addr_read)
               : offsetof(CPUTLBEntry, addr_write));
    tcg_out_ld(s, TCG_TYPE_PTR, TCG_REG_TMP2, TCG_REG_TMP2,
               offsetof(CPUTLBEntry, addend));

    /* We don't support unaligned accesses. */
    if (a_bits < s_bits) {
        a_bits = s_bits;
    }
    /* Clear the non-page, non-alignment bits from the address.  */
    compare_mask = (tcg_target_long)TARGET_PAGE_MASK | ((1 << a_bits) - 1);
    if (compare_mask == sextreg(compare_mask, 0, 12)) {
        tcg_out_opc_imm(s, OPC_ANDI, TCG_REG_TMP1, addrl, compare_mask);
    } else {
        tcg_out_movi(s, TCG_TYPE_TL, TCG_REG_TMP1, compare_mask);
        tcg_out_opc_reg(s, OPC_AND, TCG_REG_TMP1, TCG_REG_TMP1, addrl);
    }

    /* Compare masked address with the TLB entry. */
    label_ptr[0] = s->code_ptr;
    tcg_out_opc_branch(s, OPC_BNE, TCG_REG_TMP0, TCG_REG_TMP1, 0);

    /* TLB Hit - translate address using addend.  */
    if (TCG_TARGET_REG_BITS > TARGET_LONG_BITS) {
        tcg_out_ext32u(s, TCG_REG_TMP0, addrl);
        addrl = TCG_REG_TMP0;
    }
    tcg_out_opc_reg(s, OPC_ADD, TCG_REG_TMP0, TCG_REG_TMP2, addrl);
}

static void add_qemu_ldst_label(TCGContext *s, int is_ld, MemOpIdx oi,
                                TCGType ext,
                                TCGReg datalo, TCGReg datahi,
                                TCGReg addrlo, TCGReg addrhi,
                                void *raddr, tcg_insn_unit **label_ptr)
{
    TCGLabelQemuLdst *label = new_ldst_label(s);

    label->is_ld = is_ld;
    label->oi = oi;
    label->type = ext;
    label->datalo_reg = datalo;
    label->datahi_reg = datahi;
    label->addrlo_reg = addrlo;
    label->addrhi_reg = addrhi;
    label->raddr = tcg_splitwx_to_rx(raddr);
    label->label_ptr[0] = label_ptr[0];
}

static bool tcg_out_qemu_ld_slow_path(TCGContext *s, TCGLabelQemuLdst *l)
{
    MemOpIdx oi = l->oi;
    MemOp opc = get_memop(oi);
    TCGReg a0 = tcg_target_call_iarg_regs[0];
    TCGReg a1 = tcg_target_call_iarg_regs[1];
    TCGReg a2 = tcg_target_call_iarg_regs[2];
    TCGReg a3 = tcg_target_call_iarg_regs[3];

    /* We don't support oversize guests */
    if (TCG_TARGET_REG_BITS < TARGET_LONG_BITS) {
        g_assert_not_reached();
    }

    /* resolve label address */
    if (!reloc_sbimm12(l->label_ptr[0], tcg_splitwx_to_rx(s->code_ptr))) {
        return false;
    }

    /* call load helper */
    tcg_out_mov(s, TCG_TYPE_PTR, a0, TCG_AREG0);
    tcg_out_mov(s, TCG_TYPE_PTR, a1, l->addrlo_reg);
    tcg_out_movi(s, TCG_TYPE_PTR, a2, oi);
    tcg_out_movi(s, TCG_TYPE_PTR, a3, (tcg_target_long)l->raddr);

    tcg_out_call(s, qemu_ld_helpers[opc & MO_SSIZE]);
    tcg_out_mov(s, (opc & MO_SIZE) == MO_64, l->datalo_reg, a0);

    tcg_out_goto(s, l->raddr);
    return true;
}

static bool tcg_out_qemu_st_slow_path(TCGContext *s, TCGLabelQemuLdst *l)
{
    MemOpIdx oi = l->oi;
    MemOp opc = get_memop(oi);
    MemOp s_bits = opc & MO_SIZE;
    TCGReg a0 = tcg_target_call_iarg_regs[0];
    TCGReg a1 = tcg_target_call_iarg_regs[1];
    TCGReg a2 = tcg_target_call_iarg_regs[2];
    TCGReg a3 = tcg_target_call_iarg_regs[3];
    TCGReg a4 = tcg_target_call_iarg_regs[4];

    /* We don't support oversize guests */
    if (TCG_TARGET_REG_BITS < TARGET_LONG_BITS) {
        g_assert_not_reached();
    }

    /* resolve label address */
    if (!reloc_sbimm12(l->label_ptr[0], tcg_splitwx_to_rx(s->code_ptr))) {
        return false;
    }

    /* call store helper */
    tcg_out_mov(s, TCG_TYPE_PTR, a0, TCG_AREG0);
    tcg_out_mov(s, TCG_TYPE_PTR, a1, l->addrlo_reg);
    tcg_out_mov(s, TCG_TYPE_PTR, a2, l->datalo_reg);
    switch (s_bits) {
    case MO_8:
        tcg_out_ext8u(s, a2, a2);
        break;
    case MO_16:
        tcg_out_ext16u(s, a2, a2);
        break;
    default:
        break;
    }
    tcg_out_movi(s, TCG_TYPE_PTR, a3, oi);
    tcg_out_movi(s, TCG_TYPE_PTR, a4, (tcg_target_long)l->raddr);

    tcg_out_call(s, qemu_st_helpers[opc & MO_SIZE]);

    tcg_out_goto(s, l->raddr);
    return true;
}
#endif /* CONFIG_SOFTMMU */

static void tcg_out_qemu_ld_direct(TCGContext *s, TCGReg lo, TCGReg hi,
                                   TCGReg base, int opc)
{
    switch (opc) {
    case (0 | 0):
        tcg_out_opc_imm(s, OPC_LBU, lo, base, 0);
        break;
    case (4 | 0):
        tcg_out_opc_imm(s, OPC_LB, lo, base, 0);
        break;
    case (0 | 1):
        tcg_out_opc_imm(s, OPC_LHU, lo, base, 0);
        break;
    case (4 | 1):
        tcg_out_opc_imm(s, OPC_LH, lo, base, 0);
        break;
    case (0 | 2):
        tcg_out_opc_imm(s, OPC_LWU, lo, base, 0);
        break;
    case (4 | 2):
        tcg_out_opc_imm(s, OPC_LW, lo, base, 0);
        break;
    case (0 | 3):
        /* Prefer to load from offset 0 first, but allow for overlap.  */
        tcg_out_opc_imm(s, OPC_LD, lo, base, 0);
        break;
    default:
        tcg_debug_assert(false);
    }
}

static void tcg_out_qemu_ld(TCGContext *s, const TCGArg *args, int opc)
{
    TCGReg addr_regl, addr_regh __attribute__((unused));
    TCGReg data_regl, data_regh;
#if defined(CONFIG_SOFTMMU)
    tcg_insn_unit *label_ptr[1];
#endif
    TCGReg base = TCG_REG_TMP0;

    data_regl = *args++;
    data_regh = 0;
    addr_regl = *args++;

#if defined(CONFIG_SOFTMMU)
    tcg_out_tlb_load(s, addr_regl, addr_regh, oi, label_ptr, 1);
    tcg_out_qemu_ld_direct(s, data_regl, data_regh, base, opc, is_64);
    add_qemu_ldst_label(s, 1, oi,
                        (is_64 ? TCG_TYPE_I64 : TCG_TYPE_I32),
                        data_regl, data_regh, addr_regl, addr_regh,
                        s->code_ptr, label_ptr);
#else
    if (TCG_TARGET_REG_BITS > TARGET_LONG_BITS) {
        tcg_out_ext32u(s, base, addr_regl);
        addr_regl = base;
    }
    tcg_out_qemu_ld_direct(s, data_regl, data_regh, addr_regl, opc);
#endif
}

static void tcg_out_qemu_st_direct(TCGContext *s, TCGReg lo, TCGReg hi,
                                   TCGReg base, int opc)
{
    switch (opc) {
    case 0:
        tcg_out_opc_store(s, OPC_SB, base, lo, 0);
        break;
    case 1:
        tcg_out_opc_store(s, OPC_SH, base, lo, 0);
        break;
    case 2:
        tcg_out_opc_store(s, OPC_SW, base, lo, 0);
        break;
    case 3:
        tcg_out_opc_store(s, OPC_SD, base, lo, 0);
        break;
    default:
        tcg_debug_assert(false);
    }
}

static void tcg_out_qemu_st(TCGContext *s, const TCGArg *args, int opc)
{
    TCGReg addr_regl, addr_regh __attribute__((unused));
    TCGReg data_regl, data_regh;
#if defined(CONFIG_SOFTMMU)
    tcg_insn_unit *label_ptr[1];
#endif
    TCGReg base = TCG_REG_TMP0;

    data_regl = *args++;
    data_regh = 0;
    addr_regl = *args++;

#if defined(CONFIG_SOFTMMU)
    tcg_out_tlb_load(s, addr_regl, addr_regh, oi, label_ptr, 0);
    tcg_out_qemu_st_direct(s, data_regl, data_regh, base, opc);
    add_qemu_ldst_label(s, 0, oi,
                        (is_64 ? TCG_TYPE_I64 : TCG_TYPE_I32),
                        data_regl, data_regh, addr_regl, addr_regh,
                        s->code_ptr, label_ptr);
#else
    if (TCG_TARGET_REG_BITS > TARGET_LONG_BITS) {
        tcg_out_ext32u(s, base, addr_regl);
        addr_regl = base;
    }
    tcg_out_qemu_st_direct(s, data_regl, data_regh, addr_regl, opc);
#endif
}

static const tcg_insn_unit *tb_ret_addr;

static void tcg_out_op(TCGContext *s, TCGOpcode opc,
                       const TCGArg *args,
                       const int *const_args)
{
    TCGArg a0 = args[0];
    TCGArg a1 = args[1];
    TCGArg a2 = args[2];
    int c2 = const_args[2];

    switch (opc) {
    case INDEX_op_exit_tb:
        tcg_out_movi(s, TCG_TYPE_PTR, TCG_REG_A0, a0);
        tcg_out_call_int(s, tb_ret_addr, true);
        break;

    case INDEX_op_goto_tb:
        assert(s->tb_jmp_offset == 0);
        /* indirect jump method */
        tcg_out_ld(s, TCG_TYPE_PTR, TCG_REG_TMP0, TCG_REG_ZERO,
                   (uintptr_t)(s->tb_next + a0));
        tcg_out_opc_imm(s, OPC_JALR, TCG_REG_ZERO, TCG_REG_TMP0, 0);
        s->tb_next_offset[args[0]] = s->code_ptr - s->code_buf;
        break;

    case INDEX_op_br:
        tcg_out_opc_jump(s, OPC_JAL, TCG_REG_ZERO, 0);
        tcg_out_reloc(s, (uint8_t *)((tcg_insn_unit *)(s->code_ptr) - 1), R_RISCV_JAL, a0, 0);
        break;

    case INDEX_op_ld8u_i32:
    case INDEX_op_ld8u_i64:
        tcg_out_ldst(s, OPC_LBU, a0, a1, a2);
        break;
    case INDEX_op_ld8s_i32:
    case INDEX_op_ld8s_i64:
        tcg_out_ldst(s, OPC_LB, a0, a1, a2);
        break;
    case INDEX_op_ld16u_i32:
    case INDEX_op_ld16u_i64:
        tcg_out_ldst(s, OPC_LHU, a0, a1, a2);
        break;
    case INDEX_op_ld16s_i32:
    case INDEX_op_ld16s_i64:
        tcg_out_ldst(s, OPC_LH, a0, a1, a2);
        break;
    case INDEX_op_ld_i32:
    case INDEX_op_ld32u_i64:
        tcg_out_ldst(s, OPC_LWU, a0, a1, a2);
        break;
    case INDEX_op_ld32s_i64:
        tcg_out_ldst(s, OPC_LW, a0, a1, a2);
        break;
    case INDEX_op_ld_i64:
        tcg_out_ldst(s, OPC_LD, a0, a1, a2);
        break;

    case INDEX_op_st8_i32:
    case INDEX_op_st8_i64:
        tcg_out_ldst(s, OPC_SB, a0, a1, a2);
        break;
    case INDEX_op_st16_i32:
    case INDEX_op_st16_i64:
        tcg_out_ldst(s, OPC_SH, a0, a1, a2);
        break;
    case INDEX_op_st_i32:
    case INDEX_op_st32_i64:
        tcg_out_ldst(s, OPC_SW, a0, a1, a2);
        break;
    case INDEX_op_st_i64:
        tcg_out_ldst(s, OPC_SD, a0, a1, a2);
        break;

    case INDEX_op_add_i32:
        if (c2) {
            tcg_out_opc_imm(s, OPC_ADDIW, a0, a1, a2);
        } else {
            tcg_out_opc_reg(s, OPC_ADDW, a0, a1, a2);
        }
        break;
    case INDEX_op_add_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_ADDI, a0, a1, a2);
        } else {
            tcg_out_opc_reg(s, OPC_ADD, a0, a1, a2);
        }
        break;

    case INDEX_op_sub_i32:
        if (c2) {
            tcg_out_opc_imm(s, OPC_ADDIW, a0, a1, -a2);
        } else {
            tcg_out_opc_reg(s, OPC_SUBW, a0, a1, a2);
        }
        break;
    case INDEX_op_sub_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_ADDI, a0, a1, -a2);
        } else {
            tcg_out_opc_reg(s, OPC_SUB, a0, a1, a2);
        }
        break;

    case INDEX_op_and_i32:
    case INDEX_op_and_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_ANDI, a0, a1, a2);
        } else {
            tcg_out_opc_reg(s, OPC_AND, a0, a1, a2);
        }
        break;

    case INDEX_op_or_i32:
    case INDEX_op_or_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_ORI, a0, a1, a2);
        } else {
            tcg_out_opc_reg(s, OPC_OR, a0, a1, a2);
        }
        break;

    case INDEX_op_xor_i32:
    case INDEX_op_xor_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_XORI, a0, a1, a2);
        } else {
            tcg_out_opc_reg(s, OPC_XOR, a0, a1, a2);
        }
        break;

    case INDEX_op_not_i32:
    case INDEX_op_not_i64:
        tcg_out_opc_imm(s, OPC_XORI, a0, a1, -1);
        break;

    case INDEX_op_neg_i32:
        tcg_out_opc_reg(s, OPC_SUBW, a0, TCG_REG_ZERO, a1);
        break;
    case INDEX_op_neg_i64:
        tcg_out_opc_reg(s, OPC_SUB, a0, TCG_REG_ZERO, a1);
        break;

    case INDEX_op_mul_i32:
        tcg_out_opc_reg(s, OPC_MULW, a0, a1, a2);
        break;
    case INDEX_op_mul_i64:
        tcg_out_opc_reg(s, OPC_MUL, a0, a1, a2);
        break;

    case INDEX_op_div_i32:
        tcg_out_opc_reg(s, OPC_DIVW, a0, a1, a2);
        break;
    case INDEX_op_div_i64:
        tcg_out_opc_reg(s, OPC_DIV, a0, a1, a2);
        break;

    case INDEX_op_divu_i32:
        tcg_out_opc_reg(s, OPC_DIVUW, a0, a1, a2);
        break;
    case INDEX_op_divu_i64:
        tcg_out_opc_reg(s, OPC_DIVU, a0, a1, a2);
        break;

    case INDEX_op_rem_i32:
        tcg_out_opc_reg(s, OPC_REMW, a0, a1, a2);
        break;
    case INDEX_op_rem_i64:
        tcg_out_opc_reg(s, OPC_REM, a0, a1, a2);
        break;

    case INDEX_op_remu_i32:
        tcg_out_opc_reg(s, OPC_REMUW, a0, a1, a2);
        break;
    case INDEX_op_remu_i64:
        tcg_out_opc_reg(s, OPC_REMU, a0, a1, a2);
        break;

    case INDEX_op_shl_i32:
        if (c2) {
            tcg_out_opc_imm(s, OPC_SLLIW, a0, a1, a2 & 0x1f);
        } else {
            tcg_out_opc_reg(s, OPC_SLLW, a0, a1, a2);
        }
        break;
    case INDEX_op_shl_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_SLLI, a0, a1, a2 & 0x3f);
        } else {
            tcg_out_opc_reg(s, OPC_SLL, a0, a1, a2);
        }
        break;

    case INDEX_op_shr_i32:
        if (c2) {
            tcg_out_opc_imm(s, OPC_SRLIW, a0, a1, a2 & 0x1f);
        } else {
            tcg_out_opc_reg(s, OPC_SRLW, a0, a1, a2);
        }
        break;
    case INDEX_op_shr_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_SRLI, a0, a1, a2 & 0x3f);
        } else {
            tcg_out_opc_reg(s, OPC_SRL, a0, a1, a2);
        }
        break;

    case INDEX_op_sar_i32:
        if (c2) {
            tcg_out_opc_imm(s, OPC_SRAIW, a0, a1, a2 & 0x1f);
        } else {
            tcg_out_opc_reg(s, OPC_SRAW, a0, a1, a2);
        }
        break;
    case INDEX_op_sar_i64:
        if (c2) {
            tcg_out_opc_imm(s, OPC_SRAI, a0, a1, a2 & 0x3f);
        } else {
            tcg_out_opc_reg(s, OPC_SRA, a0, a1, a2);
        }
        break;

    case INDEX_op_brcond_i32:
    case INDEX_op_brcond_i64:
        tcg_out_brcond(s, a2, a0, a1, args[3]);
        break;

    case INDEX_op_setcond_i64:
        tcg_out_setcond(s, args[3], a0, a1, a2);
        break;

    case INDEX_op_qemu_ld8u:
        tcg_out_qemu_ld(s, args, 0 | 0);
        break;
    case INDEX_op_qemu_ld8s:
        tcg_out_qemu_ld(s, args, 4 | 0);
        break;
    case INDEX_op_qemu_ld16u:
        tcg_out_qemu_ld(s, args, 0 | 1);
        break;
    case INDEX_op_qemu_ld16s:
        tcg_out_qemu_ld(s, args, 4 | 1);
        break;
    case INDEX_op_qemu_ld32:
    case INDEX_op_qemu_ld32u:
        tcg_out_qemu_ld(s, args, 0 | 2);
        break;
    case INDEX_op_qemu_ld32s:
        tcg_out_qemu_ld(s, args, 4 | 2);
        break;
    case INDEX_op_qemu_ld64:
        tcg_out_qemu_ld(s, args, 0 | 3);
        break;
    case INDEX_op_qemu_st8:
        tcg_out_qemu_st(s, args, 0);
        break;
    case INDEX_op_qemu_st16:
        tcg_out_qemu_st(s, args, 1);
        break;
    case INDEX_op_qemu_st32:
        tcg_out_qemu_st(s, args, 2);
        break;
    case INDEX_op_qemu_st64:
        tcg_out_qemu_st(s, args, 3);
        break;

    case INDEX_op_ext8u_i32:
    case INDEX_op_ext8u_i64:
        tcg_out_ext8u(s, a0, a1);
        break;

    case INDEX_op_ext16u_i32:
    case INDEX_op_ext16u_i64:
        tcg_out_ext16u(s, a0, a1);
        break;

    case INDEX_op_ext32u_i64:
        tcg_out_ext32u(s, a0, a1);
        break;

    case INDEX_op_ext8s_i32:
    case INDEX_op_ext8s_i64:
        tcg_out_ext8s(s, a0, a1);
        break;

    case INDEX_op_ext16s_i32:
    case INDEX_op_ext16s_i64:
        tcg_out_ext16s(s, a0, a1);
        break;

    case INDEX_op_ext32s_i64:
        tcg_out_ext32s(s, a0, a1);
        break;

    case INDEX_op_mov_i32:
    case INDEX_op_mov_i64:
        tcg_out_mov(s, TCG_TYPE_REG, a0, a1);
        break;
    case INDEX_op_call:
        if (const_args[0]) {
            tcg_out_call_int(s, (tcg_insn_unit *)a0, false);
        } else {
            tcg_out_callr(s, a0);
        }
        break;
    default:
        assert(false);
    }
}

static const int tcg_target_callee_save_regs[] = {
    TCG_REG_S0,       /* used for the global env (TCG_AREG0) */
    TCG_REG_S1,
    TCG_REG_S2,
    TCG_REG_S3,
    TCG_REG_S4,
    TCG_REG_S5,
    TCG_REG_S6,
    TCG_REG_S7,
    TCG_REG_S8,
    TCG_REG_S9,
    TCG_REG_S10,
    TCG_REG_S11,
    TCG_REG_RA,       /* should be last for ABI compliance */
};

/* Stack frame parameters.  */
#define REG_SIZE   (TCG_TARGET_REG_BITS / 8)
#define SAVE_SIZE  ((int)ARRAY_SIZE(tcg_target_callee_save_regs) * REG_SIZE)
#define TEMP_SIZE  (CPU_TEMP_BUF_NLONGS * (int)sizeof(long))
#define FRAME_SIZE ((TCG_STATIC_CALL_ARGS_SIZE + TEMP_SIZE + SAVE_SIZE \
                     + TCG_TARGET_STACK_ALIGN - 1) \
                    & -TCG_TARGET_STACK_ALIGN)
#define SAVE_OFS   (TCG_STATIC_CALL_ARGS_SIZE + TEMP_SIZE)

/* We're expecting to be able to use an immediate for frame allocation.  */
QEMU_BUILD_BUG_ON(FRAME_SIZE > 0x7ff);

/* Generate global QEMU prologue and epilogue code */
static void tcg_target_qemu_prologue(TCGContext *s)
{
    int i;

    tcg_set_frame(s, TCG_REG_SP, TCG_STATIC_CALL_ARGS_SIZE, TEMP_SIZE);

    /* TB prologue */
    tcg_out_opc_imm(s, OPC_ADDI, TCG_REG_SP, TCG_REG_SP, -FRAME_SIZE);
    for (i = 0; i < ARRAY_SIZE(tcg_target_callee_save_regs); i++) {
        tcg_out_st(s, TCG_TYPE_REG, tcg_target_callee_save_regs[i],
                   TCG_REG_SP, SAVE_OFS + i * REG_SIZE);
    }

    /* Call generated code */
    tcg_out_mov(s, TCG_TYPE_PTR, TCG_AREG0, tcg_target_call_iarg_regs[0]);
    tcg_out_opc_imm(s, OPC_JALR, TCG_REG_ZERO, tcg_target_call_iarg_regs[1], 0);

    /* TB epilogue */
    tb_ret_addr = (tcg_insn_unit *) tcg_splitwx_to_rx(s->code_ptr);
    for (i = 0; i < ARRAY_SIZE(tcg_target_callee_save_regs); i++) {
        tcg_out_ld(s, TCG_TYPE_REG, tcg_target_callee_save_regs[i],
                   TCG_REG_SP, SAVE_OFS + i * REG_SIZE);
    }

    tcg_out_opc_imm(s, OPC_ADDI, TCG_REG_SP, TCG_REG_SP, FRAME_SIZE);
    tcg_out_opc_imm(s, OPC_JALR, TCG_REG_ZERO, TCG_REG_RA, 0);
}

static const TCGTargetOpDef riscv64_op_defs[] = {
    { INDEX_op_exit_tb, { } },
    { INDEX_op_goto_tb, { } },
    { INDEX_op_call, { "ri" } },
    { INDEX_op_br, { } },

    { INDEX_op_mov_i32, { "r", "r" } },
    { INDEX_op_mov_i64, { "r", "r" } },

    { INDEX_op_movi_i32, { "r" } },
    { INDEX_op_movi_i64, { "r" } },

    { INDEX_op_ld8u_i32, { "r", "r" } },
    { INDEX_op_ld8s_i32, { "r", "r" } },
    { INDEX_op_ld16u_i32, { "r", "r" } },
    { INDEX_op_ld16s_i32, { "r", "r" } },
    { INDEX_op_ld_i32, { "r", "r" } },
    { INDEX_op_ld8u_i64, { "r", "r" } },
    { INDEX_op_ld8s_i64, { "r", "r" } },
    { INDEX_op_ld16u_i64, { "r", "r" } },
    { INDEX_op_ld16s_i64, { "r", "r" } },
    { INDEX_op_ld32u_i64, { "r", "r" } },
    { INDEX_op_ld32s_i64, { "r", "r" } },
    { INDEX_op_ld_i64, { "r", "r" } },

    { INDEX_op_st8_i32, { "r", "r" } },
    { INDEX_op_st16_i32, { "r", "r" } },
    { INDEX_op_st_i32, { "r", "r" } },
    { INDEX_op_st8_i64, { "r", "r" } },
    { INDEX_op_st16_i64, { "r", "r" } },
    { INDEX_op_st32_i64, { "r", "r" } },
    { INDEX_op_st_i64, { "r", "r" } },

    { INDEX_op_add_i32, { "r", "r", "r" } },
    { INDEX_op_add_i64, { "r", "r", "r" } },
    { INDEX_op_sub_i32, { "r", "r", "r" } },
    { INDEX_op_sub_i64, { "r", "r", "r" } },
    { INDEX_op_mul_i32, { "r", "r", "r" } },
    { INDEX_op_mul_i64, { "r", "r", "r" } },
    { INDEX_op_and_i32, { "r", "r", "r" } },
    { INDEX_op_and_i64, { "r", "r", "r" } },
    { INDEX_op_or_i32, { "r", "r", "r" } },
    { INDEX_op_or_i64, { "r", "r", "r" } },
    { INDEX_op_xor_i32, { "r", "r", "r" } },
    { INDEX_op_xor_i64, { "r", "r", "r" } },

    { INDEX_op_shl_i32, { "r", "r", "ri" } },
    { INDEX_op_shr_i32, { "r", "r", "ri" } },
    { INDEX_op_sar_i32, { "r", "r", "ri" } },
    { INDEX_op_shl_i64, { "r", "r", "ri" } },
    { INDEX_op_shr_i64, { "r", "r", "ri" } },
    { INDEX_op_sar_i64, { "r", "r", "ri" } },

    { INDEX_op_brcond_i32, { "r", "r" } },
    { INDEX_op_setcond_i32, { "r", "r", "r" } },
    { INDEX_op_brcond_i64, { "r", "r" } },
    { INDEX_op_setcond_i64, { "r", "r", "r" } },

    { INDEX_op_qemu_ld8u, { "r", "l" } },
    { INDEX_op_qemu_ld8s, { "r", "l" } },
    { INDEX_op_qemu_ld16u, { "r", "l" } },
    { INDEX_op_qemu_ld16s, { "r", "l" } },
    { INDEX_op_qemu_ld32u, { "r", "l" } },
    { INDEX_op_qemu_ld32s, { "r", "l" } },

    { INDEX_op_qemu_ld32, { "r", "l" } },
    { INDEX_op_qemu_ld64, { "r", "l" } },

    { INDEX_op_qemu_st8, { "l", "l" } },
    { INDEX_op_qemu_st16, { "l", "l" } },
    { INDEX_op_qemu_st32, { "l", "l" } },
    { INDEX_op_qemu_st64, { "l", "l" } },

    { INDEX_op_ext8s_i32, { "r", "r" } },
    { INDEX_op_ext16s_i32, { "r", "r" } },
    { INDEX_op_ext8u_i32, { "r", "r" } },
    { INDEX_op_ext16u_i32, { "r", "r" } },

    { INDEX_op_ext8s_i64, { "r", "r" } },
    { INDEX_op_ext16s_i64, { "r", "r" } },
    { INDEX_op_ext32s_i64, { "r", "r" } },
    { INDEX_op_ext8u_i64, { "r", "r" } },
    { INDEX_op_ext16u_i64, { "r", "r" } },
    { INDEX_op_ext32u_i64, { "r", "r" } },
    { INDEX_op_not_i32, { "r", "r" } },
    { INDEX_op_not_i64, { "r", "r" } },
    { INDEX_op_neg_i32, { "r", "r" } },
    { INDEX_op_neg_i64, { "r", "r" } },
    { INDEX_op_div_i32, { "r", "r", "r" } },
    { INDEX_op_div_i64, { "r", "r", "r" } },

    { -1 },
};

static void tcg_target_init(TCGContext *s)
{
    tcg_regset_set32(tcg_target_available_regs[TCG_TYPE_I32], 0, 0xffffffff);
    tcg_regset_set32(tcg_target_available_regs[TCG_TYPE_I64], 0, 0xffffffff);

    tcg_target_call_clobber_regs = -1u;
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S0);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S1);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S2);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S3);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S4);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S5);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S6);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S7);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S8);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S9);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S10);
    tcg_regset_reset_reg(tcg_target_call_clobber_regs, TCG_REG_S11);

    tcg_regset_clear(s->reserved_regs);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_ZERO);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_TMP0);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_TMP1);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_TMP2);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_SP);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_GP);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_TP);

    tcg_add_target_add_op_defs(riscv64_op_defs);
}
