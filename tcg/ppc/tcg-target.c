/*
 * Tiny Code Generator for QEMU
 *
 * Copyright (c) 2008 Fabrice Bellard
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

static uint8_t *tb_ret_addr;

#define FAST_PATH
#if TARGET_PHYS_ADDR_BITS <= 32
#define ADDEND_OFFSET 0
#else
#define ADDEND_OFFSET 4
#endif

static const char * const tcg_target_reg_names[TCG_TARGET_NB_REGS] = {
    "r0",
    "r1",
    "rp",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",
    "r16",
    "r17",
    "r18",
    "r19",
    "r20",
    "r21",
    "r22",
    "r23",
    "r24",
    "r25",
    "r26",
    "r27",
    "r28",
    "r29",
    "r30",
    "r31"
};

static const int tcg_target_reg_alloc_order[] = {
    TCG_REG_R0,
    TCG_REG_R1,
    TCG_REG_R2,
    TCG_REG_R3,
    TCG_REG_R4,
    TCG_REG_R5,
    TCG_REG_R6,
    TCG_REG_R7,
    TCG_REG_R8,
    TCG_REG_R9,
    TCG_REG_R10,
    TCG_REG_R11,
    TCG_REG_R12,
    TCG_REG_R13,
    TCG_REG_R14,
    TCG_REG_R15,
    TCG_REG_R16,
    TCG_REG_R17,
    TCG_REG_R18,
    TCG_REG_R19,
    TCG_REG_R20,
    TCG_REG_R21,
    TCG_REG_R22,
    TCG_REG_R23,
    TCG_REG_R24,
    TCG_REG_R25,
    TCG_REG_R26,
    TCG_REG_R27,
    TCG_REG_R28,
    TCG_REG_R29,
    TCG_REG_R30,
    TCG_REG_R31
};

static const int tcg_target_call_iarg_regs[] = {
    TCG_REG_R3,
    TCG_REG_R4,
    TCG_REG_R5,
    TCG_REG_R6,
    TCG_REG_R7,
    TCG_REG_R8,
    TCG_REG_R9,
    TCG_REG_R10
};

static const int tcg_target_call_oarg_regs[2] = {
    TCG_REG_R3,
    TCG_REG_R4
};

static const int tcg_target_callee_save_regs[] = {
    TCG_REG_R13,                /* sould r13 be saved? */
    TCG_REG_R14,
    TCG_REG_R15,
    TCG_REG_R16,
    TCG_REG_R17,
    TCG_REG_R18,
    TCG_REG_R19,
    TCG_REG_R20,
    TCG_REG_R21,
    TCG_REG_R22,
    TCG_REG_R23,
    TCG_REG_R28,
    TCG_REG_R29,
    TCG_REG_R30,
    TCG_REG_R31
};

static uint32_t reloc_pc24_val (void *pc, tcg_target_long target)
{
    tcg_target_long disp;

    disp = target - (tcg_target_long) pc;
    if ((disp << 6) >> 6 != disp)
        tcg_abort ();

    return disp & 0x3fffffc;
}

static void reloc_pc24 (void *pc, tcg_target_long target)
{
    *(uint32_t *) pc = (*(uint32_t *) pc & ~0x3fffffc)
        | reloc_pc24_val (pc, target);
}

static uint16_t reloc_pc14_val (void *pc, tcg_target_long target)
{
    tcg_target_long disp;

    disp = target - (tcg_target_long) pc;
    if (disp != (int16_t) disp)
        tcg_abort ();

    return disp & 0xfffc;
}

static void reloc_pc14 (void *pc, tcg_target_long target)
{
    *(uint32_t *) pc = (*(uint32_t *) pc & ~0xfffc)
        | reloc_pc14_val (pc, target);
}

static void patch_reloc(uint8_t *code_ptr, int type,
                        tcg_target_long value, tcg_target_long addend)
{
    value += addend;
    switch (type) {
    case R_PPC_REL14:
        reloc_pc14 (code_ptr, value);
        break;
    case R_PPC_REL24:
        reloc_pc24 (code_ptr, value);
        break;
    default:
        tcg_abort();
    }
}

/* maximum number of register used for input function arguments */
static int tcg_target_get_call_iarg_regs_count(int flags)
{
    return sizeof (tcg_target_call_iarg_regs) / sizeof (tcg_target_call_iarg_regs[0]);
}

/* parse target specific constraints */
static int target_parse_constraint(TCGArgConstraint *ct, const char **pct_str)
{
    const char *ct_str;

    ct_str = *pct_str;
    switch (ct_str[0]) {
    case 'A': case 'B': case 'C': case 'D':
        ct->ct |= TCG_CT_REG;
        tcg_regset_set_reg(ct->u.regs, 3 + ct_str[0] - 'A');
        break;
    case 'r':
        ct->ct |= TCG_CT_REG;
        tcg_regset_set32(ct->u.regs, 0, 0xffffffff);
        break;
    case 'L':                   /* qemu_ld constraint */
        ct->ct |= TCG_CT_REG;
        tcg_regset_set32(ct->u.regs, 0, 0xffffffff);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R3);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R4);
        break;
    case 'K':                   /* qemu_st[8..32] constraint */
        ct->ct |= TCG_CT_REG;
        tcg_regset_set32(ct->u.regs, 0, 0xffffffff);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R3);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R4);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R5);
#if TARGET_LONG_BITS == 64
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R6);
#endif
        break;
    case 'M':                   /* qemu_st64 constraint */
        ct->ct |= TCG_CT_REG;
        tcg_regset_set32(ct->u.regs, 0, 0xffffffff);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R3);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R4);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R5);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R6);
        tcg_regset_reset_reg(ct->u.regs, TCG_REG_R7);
        break;
    default:
        return -1;
    }
    ct_str++;
    *pct_str = ct_str;
    return 0;
}

/* test if a constant matches the constraint */
static int tcg_target_const_match(tcg_target_long val,
                                  const TCGArgConstraint *arg_ct)
{
    int ct;

    ct = arg_ct->ct;
    if (ct & TCG_CT_CONST)
        return 1;
    return 0;
}

#define OPCD(opc) ((opc)<<26)
#define XO31(opc) (OPCD(31)|((opc)<<1))
#define XO19(opc) (OPCD(19)|((opc)<<1))

#define B      OPCD(18)
#define BC     OPCD(16)
#define LBZ    OPCD(34)
#define LHZ    OPCD(40)
#define LHA    OPCD(42)
#define LWZ    OPCD(32)
#define STB    OPCD(38)
#define STH    OPCD(44)
#define STW    OPCD(36)

#define ADDI   OPCD(14)
#define ADDIS  OPCD(15)
#define ORI    OPCD(24)
#define ORIS   OPCD(25)
#define XORI   OPCD(26)
#define XORIS  OPCD(27)
#define ANDI   OPCD(28)
#define ANDIS  OPCD(29)
#define MULLI  OPCD( 7)
#define CMPLI  OPCD(10)
#define CMPI   OPCD(11)

#define LWZU   OPCD(33)
#define STWU   OPCD(37)

#define RLWINM OPCD(21)

#define BCLR   XO19(16)
#define BCCTR  XO19(528)
#define CRAND  XO19(257)

#define EXTSB  XO31(954)
#define EXTSH  XO31(922)
#define ADD    XO31(266)
#define ADDE   XO31(138)
#define ADDC   XO31( 10)
#define AND    XO31( 28)
#define SUBF   XO31( 40)
#define SUBFC  XO31(  8)
#define SUBFE  XO31(136)
#define OR     XO31(444)
#define XOR    XO31(316)
#define MULLW  XO31(235)
#define MULHWU XO31( 11)
#define DIVW   XO31(491)
#define DIVWU  XO31(459)
#define CMP    XO31(  0)
#define CMPL   XO31( 32)
#define LHBRX  XO31(790)
#define LWBRX  XO31(534)
#define STHBRX XO31(918)
#define STWBRX XO31(662)
#define MFSPR  XO31(339)
#define MTSPR  XO31(467)
#define SRAWI  XO31(824)
#define NEG    XO31(104)

#define LBZX   XO31( 87)
#define LHZX   XO31(276)
#define LHAX   XO31(343)
#define LWZX   XO31( 23)
#define STBX   XO31(215)
#define STHX   XO31(407)
#define STWX   XO31(151)

#define SPR(a,b) ((((a)<<5)|(b))<<11)
#define LR     SPR(8, 0)
#define CTR    SPR(9, 0)

#define SLW    XO31( 24)
#define SRW    XO31(536)
#define SRAW   XO31(792)

#define LMW    OPCD(46)
#define STMW   OPCD(47)

#define TW     XO31(4)
#define TRAP   (TW | TO (31))

#define RT(r) ((r)<<21)
#define RS(r) ((r)<<21)
#define RA(r) ((r)<<16)
#define RB(r) ((r)<<11)
#define TO(t) ((t)<<21)
#define SH(s) ((s)<<11)
#define MB(b) ((b)<<6)
#define ME(e) ((e)<<1)
#define BO(o) ((o)<<21)

#define LK    1

#define TAB(t,a,b) (RT(t) | RA(a) | RB(b))
#define SAB(s,a,b) (RS(s) | RA(a) | RB(b))

#define BF(n)    ((n)<<23)
#define BI(n, c) (((c)+((n)*4))<<16)
#define BT(n, c) (((c)+((n)*4))<<21)
#define BA(n, c) (((c)+((n)*4))<<16)
#define BB(n, c) (((c)+((n)*4))<<11)

#define BO_COND_TRUE  BO (12)
#define BO_COND_FALSE BO (4)
#define BO_ALWAYS     BO (20)

enum {
    CR_LT,
    CR_GT,
    CR_EQ,
    CR_SO
};

static const uint32_t tcg_to_bc[10] = {
    [TCG_COND_EQ]  = BC | BI (7, CR_EQ) | BO_COND_TRUE,
    [TCG_COND_NE]  = BC | BI (7, CR_EQ) | BO_COND_FALSE,
    [TCG_COND_LT]  = BC | BI (7, CR_LT) | BO_COND_TRUE,
    [TCG_COND_GE]  = BC | BI (7, CR_LT) | BO_COND_FALSE,
    [TCG_COND_LE]  = BC | BI (7, CR_GT) | BO_COND_FALSE,
    [TCG_COND_GT]  = BC | BI (7, CR_GT) | BO_COND_TRUE,
    [TCG_COND_LTU] = BC | BI (7, CR_LT) | BO_COND_TRUE,
    [TCG_COND_GEU] = BC | BI (7, CR_LT) | BO_COND_FALSE,
    [TCG_COND_LEU] = BC | BI (7, CR_GT) | BO_COND_FALSE,
    [TCG_COND_GTU] = BC | BI (7, CR_GT) | BO_COND_TRUE,
};

static void tcg_out_mov(TCGContext *s, int ret, int arg)
{
    tcg_out32 (s, OR | SAB (arg, ret, arg));
}

static void tcg_out_movi(TCGContext *s, TCGType type,
                         int ret, tcg_target_long arg)
{
    if (arg == (int16_t) arg)
        tcg_out32 (s, ADDI | RT (ret) | RA (0) | (arg & 0xffff));
    else {
        tcg_out32 (s, ADDIS | RT (ret) | RA (0) | ((arg >> 16) & 0xffff));
        if (arg & 0xffff)
            tcg_out32 (s, ORI | RS (ret) | RA (ret) | (arg & 0xffff));
    }
}

static void tcg_out_ldst (TCGContext *s, int ret, int addr,
                          int offset, int op1, int op2)
{
    if (offset == (int16_t) offset)
        tcg_out32 (s, op1 | RT (ret) | RA (addr) | (offset & 0xffff));
    else {
        tcg_out_movi (s, TCG_TYPE_I32, 0, offset);
        tcg_out32 (s, op2 | RT (ret) | RA (addr) | RB (0));
    }
}

static void tcg_out_b (TCGContext *s, int mask, tcg_target_long target)
{
    tcg_target_long disp;

    disp = target - (tcg_target_long) s->code_ptr;
    if ((disp << 6) >> 6 == disp)
        tcg_out32 (s, B | disp | mask);
    else {
        tcg_out_movi (s, TCG_TYPE_I32, 0, (tcg_target_long) target);
        tcg_out32 (s, MTSPR | RS (0) | CTR);
        tcg_out32 (s, BCCTR | BO_ALWAYS | mask);
    }
}

#if defined(CONFIG_SOFTMMU)
extern void __ldb_mmu(void);
extern void __ldw_mmu(void);
extern void __ldl_mmu(void);
extern void __ldq_mmu(void);

extern void __stb_mmu(void);
extern void __stw_mmu(void);
extern void __stl_mmu(void);
extern void __stq_mmu(void);

static void *qemu_ld_helpers[4] = {
    __ldb_mmu,
    __ldw_mmu,
    __ldl_mmu,
    __ldq_mmu,
};

static void *qemu_st_helpers[4] = {
    __stb_mmu,
    __stw_mmu,
    __stl_mmu,
    __stq_mmu,
};
#endif

static void tcg_out_qemu_ld (TCGContext *s, const TCGArg *args, int opc)
{
    int addr_reg, data_reg, data_reg2, r0, mem_index, s_bits, bswap;
#ifdef CONFIG_SOFTMMU
    int r1, r2;
    void *label1_ptr, *label2_ptr;
#endif
#if TARGET_LONG_BITS == 64
    int addr_reg2;
#endif

    data_reg = *args++;
    if (opc == 3)
        data_reg2 = *args++;
    else
        data_reg2 = 0;
    addr_reg = *args++;
#if TARGET_LONG_BITS == 64
    addr_reg2 = *args++;
#endif
    mem_index = *args;
    s_bits = opc & 3;

#ifdef CONFIG_SOFTMMU
    r0 = 3;
    r1 = 4;
    r2 = 0;

    tcg_out32 (s, (RLWINM
                   | RA (r0)
                   | RS (addr_reg)
                   | SH (32 - (TARGET_PAGE_BITS - CPU_TLB_ENTRY_BITS))
                   | MB (32 - (CPU_TLB_BITS + CPU_TLB_ENTRY_BITS))
                   | ME (31 - CPU_TLB_ENTRY_BITS)
                   )
        );
    tcg_out32 (s, ADD | RT (r0) | RA (r0) | RB (TCG_AREG0));
    tcg_out32 (s, (LWZU
                   | RT (r1)
                   | RA (r0)
                   | offsetof (CPUState, tlb_table[mem_index][0].addr_read)
                   )
        );
    tcg_out32 (s, (RLWINM
                   | RA (r2)
                   | RS (addr_reg)
                   | SH (0)
                   | MB ((32 - s_bits) & 31)
                   | ME (31 - TARGET_PAGE_BITS)
                   )
        );

    tcg_out32 (s, CMP | BF (7) | RA (r2) | RB (r1));
#if TARGET_LONG_BITS == 64
    tcg_out32 (s, LWZ | RT (r1) | RA (r0) | 4);
    tcg_out32 (s, CMP | BF (6) | RA (addr_reg2) | RB (r1));
    tcg_out32 (s, CRAND | BT (7, CR_EQ) | BA (6, CR_EQ) | BB (7, CR_EQ));
#endif

    label1_ptr = s->code_ptr;
#ifdef FAST_PATH
    tcg_out32 (s, BC | BI (7, CR_EQ) | BO_COND_TRUE);
#endif

    /* slow path */
#if TARGET_LONG_BITS == 32
    tcg_out_mov (s, 3, addr_reg);
    tcg_out_movi (s, TCG_TYPE_I32, 4, mem_index);
#else
    tcg_out_mov (s, 3, addr_reg2);
    tcg_out_mov (s, 4, addr_reg);
    tcg_out_movi (s, TCG_TYPE_I32, 5, mem_index);
#endif

    tcg_out_b (s, LK, (tcg_target_long) qemu_ld_helpers[s_bits]);
    switch (opc) {
    case 0|4:
        tcg_out32 (s, EXTSB | RA (data_reg) | RS (3));
        break;
    case 1|4:
        tcg_out32 (s, EXTSH | RA (data_reg) | RS (3));
        break;
    case 0:
    case 1:
    case 2:
        if (data_reg != 3)
            tcg_out_mov (s, data_reg, 3);
        break;
    case 3:
        if (data_reg == 3) {
            if (data_reg2 == 4) {
                tcg_out_mov (s, 0, 4);
                tcg_out_mov (s, 4, 3);
                tcg_out_mov (s, 3, 0);
            }
            else {
                tcg_out_mov (s, data_reg2, 3);
                tcg_out_mov (s, 3, 4);
            }
        }
        else {
            if (data_reg != 4) tcg_out_mov (s, data_reg, 4);
            if (data_reg2 != 3) tcg_out_mov (s, data_reg2, 3);
        }
        break;
    }
    label2_ptr = s->code_ptr;
    tcg_out32 (s, B);

    /* label1: fast path */
#ifdef FAST_PATH
    reloc_pc14 (label1_ptr, (tcg_target_long) s->code_ptr);
#endif

    /* r0 now contains &env->tlb_table[mem_index][index].addr_read */
    tcg_out32 (s, (LWZ
                   | RT (r0)
                   | RA (r0)
                   | (ADDEND_OFFSET + offsetof (CPUTLBEntry, addend)
                      - offsetof (CPUTLBEntry, addr_read))
                   ));
    /* r0 = env->tlb_table[mem_index][index].addend */
    tcg_out32 (s, ADD | RT (r0) | RA (r0) | RB (addr_reg));
    /* r0 = env->tlb_table[mem_index][index].addend + addr */

#else  /* !CONFIG_SOFTMMU */
    r0 = addr_reg;
#endif

#ifdef TARGET_WORDS_BIGENDIAN
    bswap = 0;
#else
    bswap = 1;
#endif
    switch (opc) {
    default:
    case 0:
        tcg_out32 (s, LBZ | RT (data_reg) | RA (r0));
        break;
    case 0|4:
        tcg_out32 (s, LBZ | RT (data_reg) | RA (r0));
        tcg_out32 (s, EXTSB | RA (data_reg) | RS (data_reg));
        break;
    case 1:
        if (bswap) tcg_out32 (s, LHBRX | RT (data_reg) | RB (r0));
        else tcg_out32 (s, LHZ | RT (data_reg) | RA (r0));
        break;
    case 1|4:
        if (bswap) {
            tcg_out32 (s, LHBRX | RT (data_reg) | RB (r0));
            tcg_out32 (s, EXTSH | RA (data_reg) | RS (data_reg));
        }
        else tcg_out32 (s, LHA | RT (data_reg) | RA (r0));
        break;
    case 2:
        if (bswap) tcg_out32 (s, LWBRX | RT (data_reg) | RB (r0));
        else tcg_out32 (s, LWZ | RT (data_reg)| RA (r0));
        break;
    case 3:
        if (bswap) {
            if (r0 == data_reg) {
                tcg_out32 (s, LWBRX | RT (0) | RB (r0));
                tcg_out32 (s, ADDI | RT (r0) | RA (r0) |  4);
                tcg_out32 (s, LWBRX | RT (data_reg2) | RB (r0));
                tcg_out_mov (s, data_reg, 0);
            }
            else {
                tcg_out32 (s, LWBRX | RT (data_reg) | RB (r0));
                tcg_out32 (s, ADDI | RT (r0) | RA (r0) |  4);
                tcg_out32 (s, LWBRX | RT (data_reg2) | RB (r0));
            }
        }
        else {
            if (r0 == data_reg2) {
                tcg_out32 (s, LWZ | RT (0) | RA (r0));
                tcg_out32 (s, LWZ | RT (data_reg) | RA (r0) | 4);
                tcg_out_mov (s, data_reg2, 0);
            }
            else {
                tcg_out32 (s, LWZ | RT (data_reg2) | RA (r0));
                tcg_out32 (s, LWZ | RT (data_reg) | RA (r0) | 4);
            }
        }
        break;
    }

#ifdef CONFIG_SOFTMMU
    reloc_pc24 (label2_ptr, (tcg_target_long) s->code_ptr);
#endif
}

static void tcg_out_qemu_st (TCGContext *s, const TCGArg *args, int opc)
{
    int addr_reg, r0, r1, data_reg, data_reg2, mem_index, bswap;
#ifdef CONFIG_SOFTMMU
    int r2, ir;
    void *label1_ptr, *label2_ptr;
#endif
#if TARGET_LONG_BITS == 64
    int addr_reg2;
#endif

    data_reg = *args++;
    if (opc == 3)
        data_reg2 = *args++;
    else
        data_reg2 = 0;
    addr_reg = *args++;
#if TARGET_LONG_BITS == 64
    addr_reg2 = *args++;
#endif
    mem_index = *args;

#ifdef CONFIG_SOFTMMU
    r0 = 3;
    r1 = 4;
    r2 = 0;

    tcg_out32 (s, (RLWINM
                   | RA (r0)
                   | RS (addr_reg)
                   | SH (32 - (TARGET_PAGE_BITS - CPU_TLB_ENTRY_BITS))
                   | MB (32 - (CPU_TLB_ENTRY_BITS + CPU_TLB_BITS))
                   | ME (31 - CPU_TLB_ENTRY_BITS)
                   )
        );
    tcg_out32 (s, ADD | RT (r0) | RA (r0) | RB (TCG_AREG0));
    tcg_out32 (s, (LWZU
                   | RT (r1)
                   | RA (r0)
                   | offsetof (CPUState, tlb_table[mem_index][0].addr_write)
                   )
        );
    tcg_out32 (s, (RLWINM
                   | RA (r2)
                   | RS (addr_reg)
                   | SH (0)
                   | MB ((32 - opc) & 31)
                   | ME (31 - TARGET_PAGE_BITS)
                   )
        );

    tcg_out32 (s, CMP | (7 << 23) | RA (r2) | RB (r1));
#if TARGET_LONG_BITS == 64
    tcg_out32 (s, LWZ | RT (r1) | RA (r0) | 4);
    tcg_out32 (s, CMP | BF (6) | RA (addr_reg2) | RB (r1));
    tcg_out32 (s, CRAND | BT (7, CR_EQ) | BA (6, CR_EQ) | BB (7, CR_EQ));
#endif

    label1_ptr = s->code_ptr;
#ifdef FAST_PATH
    tcg_out32 (s, BC | BI (7, CR_EQ) | BO_COND_TRUE);
#endif

    /* slow path */
#if TARGET_LONG_BITS == 32
    tcg_out_mov (s, 3, addr_reg);
    ir = 4;
#else
    tcg_out_mov (s, 3, addr_reg2);
    tcg_out_mov (s, 4, addr_reg);
    ir = 5;
#endif

    switch (opc) {
    case 0:
        tcg_out32 (s, (RLWINM
                       | RA (ir)
                       | RS (data_reg)
                       | SH (0)
                       | MB (24)
                       | ME (31)));
        break;
    case 1:
        tcg_out32 (s, (RLWINM
                       | RA (ir)
                       | RS (data_reg)
                       | SH (0)
                       | MB (16)
                       | ME (31)));
        break;
    case 2:
        tcg_out_mov (s, ir, data_reg);
        break;
    case 3:
        tcg_out_mov (s, 5, data_reg2);
        tcg_out_mov (s, 6, data_reg);
        ir = 6;
        break;
    }
    ir++;

    tcg_out_movi (s, TCG_TYPE_I32, ir, mem_index);
    tcg_out_b (s, LK, (tcg_target_long) qemu_st_helpers[opc]);
    label2_ptr = s->code_ptr;
    tcg_out32 (s, B);

    /* label1: fast path */
#ifdef FAST_PATH
    reloc_pc14 (label1_ptr, (tcg_target_long) s->code_ptr);
#endif

    tcg_out32 (s, (LWZ
                   | RT (r0)
                   | RA (r0)
                   | (ADDEND_OFFSET + offsetof (CPUTLBEntry, addend)
                      - offsetof (CPUTLBEntry, addr_write))
                   ));
    /* r0 = env->tlb_table[mem_index][index].addend */
    tcg_out32 (s, ADD | RT (r0) | RA (r0) | RB (addr_reg));
    /* r0 = env->tlb_table[mem_index][index].addend + addr */

#else  /* !CONFIG_SOFTMMU */
    r1 = 4;
    r0 = addr_reg;
#endif

#ifdef TARGET_WORDS_BIGENDIAN
    bswap = 0;
#else
    bswap = 1;
#endif
    switch (opc) {
    case 0:
        tcg_out32 (s, STB | RS (data_reg) | RA (r0));
        break;
    case 1:
        if (bswap) tcg_out32 (s, STHBRX | RS (data_reg) | RA (0) | RB (r0));
        else tcg_out32 (s, STH | RS (data_reg) | RA (r0));
        break;
    case 2:
        if (bswap) tcg_out32 (s, STWBRX | RS (data_reg) | RA (0) | RB (r0));
        else tcg_out32 (s, STW | RS (data_reg) | RA (r0));
        break;
    case 3:
        if (bswap) {
            tcg_out32 (s, ADDI | RT (r1) | RA (r0) | 4);
            tcg_out32 (s, STWBRX | RS (data_reg) | RA (0) | RB (r0));
            tcg_out32 (s, STWBRX | RS (data_reg2) | RA (0) | RB (r1));
        }
        else {
            tcg_out32 (s, STW | RS (data_reg2) | RA (r0));
            tcg_out32 (s, STW | RS (data_reg) | RA (r0) | 4);
        }
        break;
    }

#ifdef CONFIG_SOFTMMU
    reloc_pc24 (label2_ptr, (tcg_target_long) s->code_ptr);
#endif
}

void tcg_target_qemu_prologue (TCGContext *s)
{
    int i, frame_size;

    frame_size = 0
        + 4                     /* back chain */
        + 4                     /* LR */
        + TCG_STATIC_CALL_ARGS_SIZE
        + ARRAY_SIZE (tcg_target_callee_save_regs) * 4
        ;
    frame_size = (frame_size + 15) & ~15;

    tcg_out32 (s, MFSPR | RT (0) | LR);
    tcg_out32 (s, STWU | RS (1) | RA (1) | (-frame_size & 0xffff));
    for (i = 0; i < ARRAY_SIZE (tcg_target_callee_save_regs); ++i)
        tcg_out32 (s, (STW
                       | RS (tcg_target_callee_save_regs[i])
                       | RA (1)
                       | (i * 4 + 8 + TCG_STATIC_CALL_ARGS_SIZE)
                       )
            );
    tcg_out32 (s, STW | RS (0) | RA (1) | (frame_size - 4));

    tcg_out32 (s, MTSPR | RS (3) | CTR);
    tcg_out32 (s, BCCTR | BO_ALWAYS);
    tb_ret_addr = s->code_ptr;

    for (i = 0; i < ARRAY_SIZE (tcg_target_callee_save_regs); ++i)
        tcg_out32 (s, (LWZ
                       | RT (tcg_target_callee_save_regs[i])
                       | RA (1)
                       | (i * 4 + 8 + TCG_STATIC_CALL_ARGS_SIZE)
                       )
            );
    tcg_out32 (s, LWZ | RT (0) | RA (1) | (frame_size - 4));
    tcg_out32 (s, MTSPR | RS (0) | LR);
    tcg_out32 (s, ADDI | RT (1) | RA (1) | frame_size);
    tcg_out32 (s, BCLR | BO_ALWAYS);
}

static void tcg_out_ld (TCGContext *s, TCGType type, int ret, int arg1,
                        tcg_target_long arg2)
{
    tcg_out_ldst (s, ret, arg1, arg2, LWZ, LWZX);
}

static void tcg_out_st (TCGContext *s, TCGType type, int arg, int arg1,
                        tcg_target_long arg2)
{
    tcg_out_ldst (s, arg, arg1, arg2, STW, STWX);
}

static void ppc_addi (TCGContext *s, int rt, int ra, tcg_target_long si)
{
    if (!si && rt == ra)
        return;

    if (si == (int16_t) si)
        tcg_out32 (s, ADDI | RT (rt) | RA (ra) | (si & 0xffff));
    else {
        uint16_t h = ((si >> 16) & 0xffff) + ((uint16_t) si >> 15);
        tcg_out32 (s, ADDIS | RT (rt) | RA (ra) | h);
        tcg_out32 (s, ADDI | RT (rt) | RA (rt) | (si & 0xffff));
    }
}

static void tcg_out_addi(TCGContext *s, int reg, tcg_target_long val)
{
    ppc_addi (s, reg, reg, val);
}

static void tcg_out_brcond(TCGContext *s, int cond,
                           TCGArg arg1, TCGArg arg2, int const_arg2,
                           int label_index)
{
    TCGLabel *l = &s->labels[label_index];
    int imm;
    uint32_t op;

    switch (cond) {
    case TCG_COND_EQ:
    case TCG_COND_NE:
        if (const_arg2) {
            if ((int16_t) arg2 == arg2) {
                op = CMPI;
                imm = 1;
                break;
            }
            else if ((uint16_t) arg2 == arg2) {
                op = CMPLI;
                imm = 1;
                break;
            }
        }
        op = CMPL;
        imm = 0;
        break;

    case TCG_COND_LT:
    case TCG_COND_GE:
    case TCG_COND_LE:
    case TCG_COND_GT:
        if (const_arg2) {
            if ((int16_t) arg2 == arg2) {
                op = CMPI;
                imm = 1;
                break;
            }
        }
        op = CMP;
        imm = 0;
        break;

    case TCG_COND_LTU:
    case TCG_COND_GEU:
    case TCG_COND_LEU:
    case TCG_COND_GTU:
        if (const_arg2) {
            if ((uint16_t) arg2 == arg2) {
                op = CMPLI;
                imm = 1;
                break;
            }
        }
        op = CMPL;
        imm = 0;
        break;

    default:
        tcg_abort ();
    }
    op |= BF (7);

    if (imm)
        tcg_out32 (s, op | RA (arg1) | (arg2 & 0xffff));
    else {
        if (const_arg2) {
            tcg_out_movi (s, TCG_TYPE_I32, 0, arg2);
            tcg_out32 (s, op | RA (arg1) | RB (0));
        }
        else
            tcg_out32 (s, op | RA (arg1) | RB (arg2));
    }

    if (l->has_value)
        tcg_out32 (s, tcg_to_bc[cond] | reloc_pc14_val (s->code_ptr,
                                                        l->u.value));
    else {
        uint16_t val = *(uint16_t *) &s->code_ptr[2];

        /* Thanks to Andrzej Zaborowski */
        tcg_out32 (s, tcg_to_bc[cond] | (val & 0xfffc));
        tcg_out_reloc (s, s->code_ptr - 4, R_PPC_REL14, label_index, 0);
    }
}

/* brcond2 is taken verbatim from i386 tcg-target */
/* XXX: we implement it at the target level to avoid having to
   handle cross basic blocks temporaries */
static void tcg_out_brcond2(TCGContext *s,
                            const TCGArg *args, const int *const_args)
{
    int label_next;
    label_next = gen_new_label();
    switch(args[4]) {
    case TCG_COND_EQ:
        tcg_out_brcond(s, TCG_COND_NE, args[0], args[2], const_args[2], label_next);
        tcg_out_brcond(s, TCG_COND_EQ, args[1], args[3], const_args[3], args[5]);
        break;
    case TCG_COND_NE:
        tcg_out_brcond(s, TCG_COND_NE, args[0], args[2], const_args[2], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], args[5]);
        break;
    case TCG_COND_LT:
        tcg_out_brcond(s, TCG_COND_LT, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_LT, args[0], args[2], const_args[2], args[5]);
        break;
    case TCG_COND_LE:
        tcg_out_brcond(s, TCG_COND_LT, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_LE, args[0], args[2], const_args[2], args[5]);
        break;
    case TCG_COND_GT:
        tcg_out_brcond(s, TCG_COND_GT, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_GT, args[0], args[2], const_args[2], args[5]);
        break;
    case TCG_COND_GE:
        tcg_out_brcond(s, TCG_COND_GT, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_GE, args[0], args[2], const_args[2], args[5]);
        break;
    case TCG_COND_LTU:
        tcg_out_brcond(s, TCG_COND_LTU, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_LTU, args[0], args[2], const_args[2], args[5]);
        break;
    case TCG_COND_LEU:
        tcg_out_brcond(s, TCG_COND_LTU, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_LEU, args[0], args[2], const_args[2], args[5]);
        break;
    case TCG_COND_GTU:
        tcg_out_brcond(s, TCG_COND_GTU, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_GTU, args[0], args[2], const_args[2], args[5]);
        break;
    case TCG_COND_GEU:
        tcg_out_brcond(s, TCG_COND_GTU, args[1], args[3], const_args[3], args[5]);
        tcg_out_brcond(s, TCG_COND_NE, args[1], args[3], const_args[3], label_next);
        tcg_out_brcond(s, TCG_COND_GEU, args[0], args[2], const_args[2], args[5]);
        break;
    default:
        tcg_abort();
    }
    tcg_out_label(s, label_next, (tcg_target_long)s->code_ptr);
}

static uint64_t __attribute ((used)) ppc_udiv_helper (uint64_t a, uint32_t b)
{
    uint64_t rem, quo;
    quo = a / b;
    rem = a % b;
    return (rem << 32) | (uint32_t) quo;
}

static uint64_t __attribute ((used)) ppc_div_helper (int64_t a, int32_t b)
{
    int64_t rem, quo;
    quo = a / b;
    rem = a % b;
    return (rem << 32) | (uint32_t) quo;
}

#define MAKE_TRAMPOLINE(name)                   \
extern void name##_trampoline (void);           \
asm (#name "_trampoline:\n"                     \
     " mflr 0\n"                                \
     " addi 1,1,-112\n"                         \
     " mr   4,6\n"                              \
     " stmw 7,0(1)\n"                           \
     " stw  0,108(0)\n"                         \
     " bl   ppc_" #name "_helper\n"             \
     " lmw  7,0(1)\n"                           \
     " lwz  0,108(0)\n"                         \
     " addi 1,1,112\n"                          \
     " mtlr 0\n"                                \
     " blr\n"                                   \
    )

MAKE_TRAMPOLINE (div);
MAKE_TRAMPOLINE (udiv);

static void tcg_out_div2 (TCGContext *s, int uns)
{
    void *label1_ptr, *label2_ptr;

    tcg_out32 (s, CMPLI | BF (7) | RA (3));
    label1_ptr = s->code_ptr;
    tcg_out32 (s, BC | BI (7, CR_EQ) | BO_COND_TRUE);

    tcg_out_b (s, LK, (tcg_target_long) (uns ? udiv_trampoline : div_trampoline));

    label2_ptr = s->code_ptr;
    tcg_out32 (s, B);

    reloc_pc14 (label1_ptr, (tcg_target_long) s->code_ptr);

    tcg_out32 (s, (uns ? DIVWU : DIVW) | TAB (6, 4, 5));
    tcg_out32 (s, MULLW | TAB (0, 6, 5));
    tcg_out32 (s, SUBF | TAB (3, 0, 4));

    reloc_pc24 (label2_ptr, (tcg_target_long) s->code_ptr);
}

static void tcg_out_op(TCGContext *s, int opc, const TCGArg *args,
                       const int *const_args)
{
    switch (opc) {
    case INDEX_op_exit_tb:
        tcg_out_movi (s, TCG_TYPE_I32, TCG_REG_R3, args[0]);
        tcg_out_b (s, 0, (tcg_target_long) tb_ret_addr);
        break;
    case INDEX_op_goto_tb:
        if (s->tb_jmp_offset) {
            /* direct jump method */

            s->tb_jmp_offset[args[0]] = s->code_ptr - s->code_buf;
            s->code_ptr += 16;
        }
        else {
            tcg_abort ();
        }
        s->tb_next_offset[args[0]] = s->code_ptr - s->code_buf;
        break;
    case INDEX_op_br:
        {
            TCGLabel *l = &s->labels[args[0]];

            if (l->has_value) {
                tcg_out_b (s, 0, l->u.value);
            }
            else {
                uint32_t val = *(uint32_t *) s->code_ptr;

                /* Thanks to Andrzej Zaborowski */
                tcg_out32 (s, B | (val & 0x3fffffc));
                tcg_out_reloc (s, s->code_ptr - 4, R_PPC_REL24, args[0], 0);
            }
        }
        break;
    case INDEX_op_call:
        if (const_args[0]) {
            tcg_out_b (s, LK, args[0]);
        }
        else {
            tcg_out32 (s, MTSPR | RS (args[0]) | LR);
            tcg_out32 (s, BCLR | BO_ALWAYS | LK);
        }
        break;
    case INDEX_op_jmp:
        if (const_args[0]) {
            tcg_out_b (s, 0, args[0]);
        }
        else {
            tcg_out32 (s, MTSPR | RS (args[0]) | CTR);
            tcg_out32 (s, BCCTR | BO_ALWAYS);
        }
        break;
    case INDEX_op_movi_i32:
        tcg_out_movi(s, TCG_TYPE_I32, args[0], args[1]);
        break;
    case INDEX_op_ld8u_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], LBZ, LBZX);
        break;
    case INDEX_op_ld8s_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], LBZ, LBZX);
        tcg_out32 (s, EXTSB | RS (args[0]) | RA (args[0]));
        break;
    case INDEX_op_ld16u_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], LHZ, LHZX);
        break;
    case INDEX_op_ld16s_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], LHA, LHAX);
        break;
    case INDEX_op_ld_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], LWZ, LWZX);
        break;
    case INDEX_op_st8_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], STB, STBX);
        break;
    case INDEX_op_st16_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], STH, STHX);
        break;
    case INDEX_op_st_i32:
        tcg_out_ldst (s, args[0], args[1], args[2], STW, STWX);
        break;

    case INDEX_op_add_i32:
        if (const_args[2])
            ppc_addi (s, args[0], args[1], args[2]);
        else
            tcg_out32 (s, ADD | TAB (args[0], args[1], args[2]));
        break;
    case INDEX_op_sub_i32:
        if (const_args[2])
            ppc_addi (s, args[0], args[1], -args[2]);
        else
            tcg_out32 (s, SUBF | TAB (args[0], args[2], args[1]));
        break;

    case INDEX_op_and_i32:
        if (const_args[2]) {
            if (!args[2])
                tcg_out_movi (s, TCG_TYPE_I32, args[0], 0);
            else {
                if ((args[2] & 0xffff) == args[2])
                    tcg_out32 (s, ANDI | RS (args[1]) | RA (args[0]) | args[2]);
                else if ((args[2] & 0xffff0000) == args[2])
                    tcg_out32 (s, ANDIS | RS (args[1]) | RA (args[0])
                               | ((args[2] >> 16) & 0xffff));
                else if (args[2] == 0xffffffff) {
                    if (args[0] != args[1])
                        tcg_out_mov (s, args[0], args[1]);
                }
                else {
                    tcg_out_movi (s, TCG_TYPE_I32, 0, args[2]);
                    tcg_out32 (s, AND | SAB (args[1], args[0], 0));
                }
            }
        }
        else
            tcg_out32 (s, AND | SAB (args[1], args[0], args[2]));
        break;
    case INDEX_op_or_i32:
        if (const_args[2]) {
            if (args[2]) {
                if (args[2] & 0xffff) {
                    tcg_out32 (s, ORI | RS (args[1])  | RA (args[0])
                               | (args[2] & 0xffff));
                    if (args[2] >> 16)
                        tcg_out32 (s, ORIS | RS (args[0])  | RA (args[0])
                                   | ((args[2] >> 16) & 0xffff));
                }
                else {
                    tcg_out32 (s, ORIS | RS (args[1])  | RA (args[0])
                               | ((args[2] >> 16) & 0xffff));
                }
            }
            else {
                if (args[0] != args[1])
                    tcg_out_mov (s, args[0], args[1]);
            }
        }
        else
            tcg_out32 (s, OR | SAB (args[1], args[0], args[2]));
        break;
    case INDEX_op_xor_i32:
        if (const_args[2]) {
            if (args[2]) {
                if ((args[2] & 0xffff) == args[2])
                    tcg_out32 (s, XORI | RS (args[1])  | RA (args[0])
                               | (args[2] & 0xffff));
                else if ((args[2] & 0xffff0000) == args[2])
                    tcg_out32 (s, XORIS | RS (args[1])  | RA (args[0])
                               | ((args[2] >> 16) & 0xffff));
                else {
                    tcg_out_movi (s, TCG_TYPE_I32, 0, args[2]);
                    tcg_out32 (s, XOR | SAB (args[1], args[0], 0));
                }
            }
            else {
                if (args[0] != args[1])
                    tcg_out_mov (s, args[0], args[1]);
            }
        }
        else
            tcg_out32 (s, XOR | SAB (args[1], args[0], args[2]));
        break;

    case INDEX_op_mul_i32:
        if (const_args[2]) {
            if (args[2] == (int16_t) args[2])
                tcg_out32 (s, MULLI | RT (args[0]) | RA (args[1])
                           | (args[2] & 0xffff));
            else {
                tcg_out_movi (s, TCG_TYPE_I32, 0, args[2]);
                tcg_out32 (s, MULLW | TAB (args[0], args[1], 0));
            }
        }
        else
            tcg_out32 (s, MULLW | TAB (args[0], args[1], args[2]));
        break;
    case INDEX_op_mulu2_i32:
        if (args[0] == args[2] || args[0] == args[3]) {
            tcg_out32 (s, MULLW | TAB (0, args[2], args[3]));
            tcg_out32 (s, MULHWU | TAB (args[1], args[2], args[3]));
            tcg_out_mov (s, args[0], 0);
        }
        else {
            tcg_out32 (s, MULLW | TAB (args[0], args[2], args[3]));
            tcg_out32 (s, MULHWU | TAB (args[1], args[2], args[3]));
        }
        break;
    case INDEX_op_div2_i32:
        tcg_out_div2 (s, 0);
        break;
    case INDEX_op_divu2_i32:
        tcg_out_div2 (s, 1);
        break;

    case INDEX_op_shl_i32:
        if (const_args[2]) {
            if (args[2])
                tcg_out32 (s, (RLWINM
                               | RA (args[0])
                               | RS (args[1])
                               | SH (args[2])
                               | MB (0)
                               | ME (31 - args[2])
                               )
                    );
            else
                tcg_out_mov (s, args[0], args[1]);
        }
        else
            tcg_out32 (s, SLW | SAB (args[1], args[0], args[2]));
        break;
    case INDEX_op_shr_i32:
        if (const_args[2]) {
            if (args[2])
                tcg_out32 (s, (RLWINM
                               | RA (args[0])
                               | RS (args[1])
                               | SH (32 - args[2])
                               | MB (args[2])
                               | ME (31)
                               )
                    );
            else
                tcg_out_mov (s, args[0], args[1]);
        }
        else
            tcg_out32 (s, SRW | SAB (args[1], args[0], args[2]));
        break;
    case INDEX_op_sar_i32:
        if (const_args[2])
            tcg_out32 (s, SRAWI | RS (args[1]) | RA (args[0]) | SH (args[2]));
        else
            tcg_out32 (s, SRAW | SAB (args[1], args[0], args[2]));
        break;

    case INDEX_op_add2_i32:
        if (args[0] == args[3] || args[0] == args[5]) {
            tcg_out32 (s, ADDC | TAB (0, args[2], args[4]));
            tcg_out32 (s, ADDE | TAB (args[1], args[3], args[5]));
            tcg_out_mov (s, args[0], 0);
        }
        else {
            tcg_out32 (s, ADDC | TAB (args[0], args[2], args[4]));
            tcg_out32 (s, ADDE | TAB (args[1], args[3], args[5]));
        }
        break;
    case INDEX_op_sub2_i32:
        if (args[0] == args[3] || args[0] == args[5]) {
            tcg_out32 (s, SUBFC | TAB (0, args[4], args[2]));
            tcg_out32 (s, SUBFE | TAB (args[1], args[5], args[3]));
            tcg_out_mov (s, args[0], 0);
        }
        else {
            tcg_out32 (s, SUBFC | TAB (args[0], args[4], args[2]));
            tcg_out32 (s, SUBFE | TAB (args[1], args[5], args[3]));
        }
        break;

    case INDEX_op_brcond_i32:
        /*
          args[0] = r0
          args[1] = r1
          args[2] = cond
          args[3] = r1 is const
          args[4] = label_index
        */
        tcg_out_brcond (s, args[2], args[0], args[1], const_args[1], args[3]);
        break;
    case INDEX_op_brcond2_i32:
        tcg_out_brcond2(s, args, const_args);
        break;

    case INDEX_op_neg_i32:
        tcg_out32 (s, NEG | RT (args[0]) | RA (args[1]));
        break;

    case INDEX_op_qemu_ld8u:
        tcg_out_qemu_ld(s, args, 0);
        break;
    case INDEX_op_qemu_ld8s:
        tcg_out_qemu_ld(s, args, 0 | 4);
        break;
    case INDEX_op_qemu_ld16u:
        tcg_out_qemu_ld(s, args, 1);
        break;
    case INDEX_op_qemu_ld16s:
        tcg_out_qemu_ld(s, args, 1 | 4);
        break;
    case INDEX_op_qemu_ld32u:
        tcg_out_qemu_ld(s, args, 2);
        break;
    case INDEX_op_qemu_ld64:
        tcg_out_qemu_ld(s, args, 3);
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

    default:
        tcg_dump_ops (s, stderr);
        tcg_abort ();
    }
}

static const TCGTargetOpDef ppc_op_defs[] = {
    { INDEX_op_exit_tb, { } },
    { INDEX_op_goto_tb, { } },
    { INDEX_op_call, { "ri" } },
    { INDEX_op_jmp, { "ri" } },
    { INDEX_op_br, { } },

    { INDEX_op_mov_i32, { "r", "r" } },
    { INDEX_op_movi_i32, { "r" } },
    { INDEX_op_ld8u_i32, { "r", "r" } },
    { INDEX_op_ld8s_i32, { "r", "r" } },
    { INDEX_op_ld16u_i32, { "r", "r" } },
    { INDEX_op_ld16s_i32, { "r", "r" } },
    { INDEX_op_ld_i32, { "r", "r" } },
    { INDEX_op_st8_i32, { "r", "r" } },
    { INDEX_op_st16_i32, { "r", "r" } },
    { INDEX_op_st_i32, { "r", "r" } },

    { INDEX_op_add_i32, { "r", "r", "ri" } },
    { INDEX_op_mul_i32, { "r", "r", "ri" } },
    { INDEX_op_mulu2_i32, { "r", "r", "r", "r" } },
    { INDEX_op_div2_i32, { "D", "A", "B", "1", "C" } },
    { INDEX_op_divu2_i32, { "D", "A", "B", "1", "C" } },
    { INDEX_op_sub_i32, { "r", "r", "ri" } },
    { INDEX_op_and_i32, { "r", "r", "ri" } },
    { INDEX_op_or_i32, { "r", "r", "ri" } },
    { INDEX_op_xor_i32, { "r", "r", "ri" } },

    { INDEX_op_shl_i32, { "r", "r", "ri" } },
    { INDEX_op_shr_i32, { "r", "r", "ri" } },
    { INDEX_op_sar_i32, { "r", "r", "ri" } },

    { INDEX_op_brcond_i32, { "r", "ri" } },

    { INDEX_op_add2_i32, { "r", "r", "r", "r", "r", "r" } },
    { INDEX_op_sub2_i32, { "r", "r", "r", "r", "r", "r" } },
    { INDEX_op_brcond2_i32, { "r", "r", "r", "r" } },

    { INDEX_op_neg_i32, { "r", "r" } },

#if TARGET_LONG_BITS == 32
    { INDEX_op_qemu_ld8u, { "r", "L" } },
    { INDEX_op_qemu_ld8s, { "r", "L" } },
    { INDEX_op_qemu_ld16u, { "r", "L" } },
    { INDEX_op_qemu_ld16s, { "r", "L" } },
    { INDEX_op_qemu_ld32u, { "r", "L" } },
    { INDEX_op_qemu_ld32s, { "r", "L" } },
    { INDEX_op_qemu_ld64, { "r", "r", "L" } },

    { INDEX_op_qemu_st8, { "K", "K" } },
    { INDEX_op_qemu_st16, { "K", "K" } },
    { INDEX_op_qemu_st32, { "K", "K" } },
    { INDEX_op_qemu_st64, { "M", "M", "M" } },
#else
    { INDEX_op_qemu_ld8u, { "r", "L", "L" } },
    { INDEX_op_qemu_ld8s, { "r", "L", "L" } },
    { INDEX_op_qemu_ld16u, { "r", "L", "L" } },
    { INDEX_op_qemu_ld16s, { "r", "L", "L" } },
    { INDEX_op_qemu_ld32u, { "r", "L", "L" } },
    { INDEX_op_qemu_ld32s, { "r", "L", "L" } },
    { INDEX_op_qemu_ld64, { "r", "L", "L", "L" } },

    { INDEX_op_qemu_st8, { "K", "K", "K" } },
    { INDEX_op_qemu_st16, { "K", "K", "K" } },
    { INDEX_op_qemu_st32, { "K", "K", "K" } },
    { INDEX_op_qemu_st64, { "M", "M", "M", "M" } },
#endif

    { -1 },
};

void tcg_target_init(TCGContext *s)
{
    tcg_regset_set32(tcg_target_available_regs[TCG_TYPE_I32], 0, 0xffffffff);
    tcg_regset_set32(tcg_target_call_clobber_regs, 0,
                     (1 << TCG_REG_R0) |
                     (1 << TCG_REG_R3) |
                     (1 << TCG_REG_R4) |
                     (1 << TCG_REG_R5) |
                     (1 << TCG_REG_R6) |
                     (1 << TCG_REG_R7) |
                     (1 << TCG_REG_R8) |
                     (1 << TCG_REG_R9) |
                     (1 << TCG_REG_R10) |
                     (1 << TCG_REG_R11) |
                     (1 << TCG_REG_R12)
        );

    tcg_regset_clear(s->reserved_regs);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_R0);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_R1);
    tcg_regset_set_reg(s->reserved_regs, TCG_REG_R2);

    tcg_add_target_add_op_defs(ppc_op_defs);
}