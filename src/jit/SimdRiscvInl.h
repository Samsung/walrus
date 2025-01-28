/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Only included by Backend.cpp */

#define OPCODE(x) (((uint32_t)x) << 26)

namespace SimdOp {

enum InstructionType : uint32_t {
    vm = 1 << 25,
    simd = 0x57 | vm,
    opfvf = (0x5 << 12) | simd,
    opfvv = (0x1 << 12) | simd,
    opivi = (0x3 << 12) | simd,
    opivv = (0x0 << 12) | simd,
    opivx = (0x4 << 12) | simd,
    opmvv = (0x2 << 12) | simd,
    opmvx = (0x6 << 12) | simd
};

enum TypeOpcode : uint32_t {
    vaaddu_vv = InstructionType::opmvv | OPCODE(0x8),
    vadd_vi = InstructionType::opivi | OPCODE(0x0),
    vadd_vv = InstructionType::opivv | OPCODE(0x0),
    vand_vv = InstructionType::opivv | OPCODE(0x9),
    vcompress_vm = InstructionType::opmvv | OPCODE(0x17),
#if defined(__riscv_zvbb)
    vcpop_v = InstructionType::opmvv | OPCODE(0x12) | (0xe << 15),
#endif
    vfadd_vf = InstructionType::opfvf | OPCODE(0x0),
    vfadd_vv = InstructionType::opfvv | OPCODE(0x0),
    vfcvt_f_x_v = InstructionType::opfvv | OPCODE(0x12) | (0x3 << 15),
    vfcvt_f_xu_v = InstructionType::opfvv | OPCODE(0x12) | (0x2 << 15),
    vfcvt_x_f_v = InstructionType::opfvv | OPCODE(0x12) | (0x1 << 15),
    vfcvt_rtz_x_f_v = InstructionType::opfvv | OPCODE(0x12) | (0x7 << 15),
    vfcvt_rtz_xu_f_v = InstructionType::opfvv | OPCODE(0x12) | (0x6 << 15),
    vfncvt_rtz_x_f_w = InstructionType::opfvv | OPCODE(0x12) | (0x17 << 15),
    vfncvt_rtz_xu_f_w = InstructionType::opfvv | OPCODE(0x12) | (0x16 << 15),
    vfdiv_vv = InstructionType::opfvv | OPCODE(0x20),
    vfirst_m = InstructionType::opmvv | OPCODE(0x10) | (0x11 << 15),
    vfncvt_f_f_w = InstructionType::opfvv | OPCODE(0x12) | (0x14 << 15),
    vfmax_vv = InstructionType::opfvv | OPCODE(0x6),
    vfmin_vv = InstructionType::opfvv | OPCODE(0x4),
    vfmul_vv = InstructionType::opfvv | OPCODE(0x24),
    vfsgnj_vv = InstructionType::opfvv | OPCODE(0x8),
    vfsgnjn_vv = InstructionType::opfvv | OPCODE(0x9),
    vfsgnjx_vv = InstructionType::opfvv | OPCODE(0xa),
    vfsqrt_v = InstructionType::opfvv | OPCODE(0x13),
    vfsub_vv = InstructionType::opfvv | OPCODE(0x2),
    vfwcvt_f_f_v = InstructionType::opfvv | OPCODE(0x12) | (0xc << 15),
    vfwcvt_f_x_v = InstructionType::opfvv | OPCODE(0x12) | (0xb << 15),
    vfwcvt_f_xu_v = InstructionType::opfvv | OPCODE(0x12) | (0xa << 15),
    vmax_vv = InstructionType::opivv | OPCODE(0x7),
    vmax_vx = InstructionType::opivx | OPCODE(0x7),
    vmaxu_vv = InstructionType::opivv | OPCODE(0x6),
    vmerge_vi = (InstructionType::opivi ^ InstructionType::vm) | OPCODE(0x17),
    vmerge_vv = (InstructionType::opivv ^ InstructionType::vm) | OPCODE(0x17),
    vmfeq_vv = InstructionType::opfvv | OPCODE(0x18),
    vmfle_vv = InstructionType::opfvv | OPCODE(0x19),
    vmflt_vv = InstructionType::opfvv | OPCODE(0x1b),
    vmfne_vv = InstructionType::opfvv | OPCODE(0x1c),
    vmin_vv = InstructionType::opivv | OPCODE(0x5),
    vminu_vv = InstructionType::opivv | OPCODE(0x4),
    vmseq_vv = InstructionType::opivv | OPCODE(0x18),
    vmsle_vv = InstructionType::opivv | OPCODE(0x1d),
    vmsleu_vv = InstructionType::opivv | OPCODE(0x1c),
    vmslt_vv = InstructionType::opivv | OPCODE(0x1b),
    vmslt_vx = InstructionType::opivx | OPCODE(0x1b),
    vmsltu_vv = InstructionType::opivv | OPCODE(0x1a),
    vmsne_vi = InstructionType::opivi | OPCODE(0x19),
    vmsne_vv = InstructionType::opivv | OPCODE(0x19),
    vmul_vv = InstructionType::opmvv | OPCODE(0x25),
    vmv_sx = InstructionType::opmvx | OPCODE(0x10),
    vmv_vi = InstructionType::opivi | OPCODE(0x17),
    vmv_vv = InstructionType::opivv | OPCODE(0x17),
    vmv_vx = InstructionType::opivx | OPCODE(0x17),
    vmv_xs = InstructionType::opmvv | OPCODE(0x10),
    vnclip_wi = InstructionType::opivi | OPCODE(0x2f),
    vnclipu_wi = InstructionType::opivi | OPCODE(0x2e),
    vor_vv = InstructionType::opivv | OPCODE(0xa),
    vredmaxu_vs = InstructionType::opmvv | OPCODE(0x6),
    vredminu_vs = InstructionType::opmvv | OPCODE(0x4),
    vredsum_vs = InstructionType::opmvv | OPCODE(0x0),
    vrgather_vv = InstructionType::opivv | OPCODE(0xc),
    vrsub_vi = InstructionType::opivi | OPCODE(0x3),
    vsadd_vv = InstructionType::opivv | OPCODE(0x21),
    vsaddu_vv = InstructionType::opivv | OPCODE(0x20),
    vsext_vf2 = InstructionType::opmvv | OPCODE(0x12) | (0x7 << 15),
    vslidedown_vi = InstructionType::opivi | OPCODE(0xf),
    vslideup_vi = InstructionType::opivi | OPCODE(0xe),
    vsll_vi = InstructionType::opivi | OPCODE(0x25),
    vsll_vx = InstructionType::opivx | OPCODE(0x25),
    vsra_vi = InstructionType::opivi | OPCODE(0x29),
    vsra_vx = InstructionType::opivx | OPCODE(0x29),
    vsrl_vi = InstructionType::opivi | OPCODE(0x28),
    vsrl_vx = InstructionType::opivx | OPCODE(0x28),
    vssub_vv = InstructionType::opivv | OPCODE(0x23),
    vssubu_vv = InstructionType::opivv | OPCODE(0x22),
    vsub_vv = InstructionType::opivv | OPCODE(0x2),
    vwmul_vv = InstructionType::opmvv | OPCODE(0x3b),
    vwmulu_vv = InstructionType::opmvv | OPCODE(0x38),
    vxor_vi = InstructionType::opivi | OPCODE(0xb),
    vxor_vv = InstructionType::opivv | OPCODE(0xb),
    vzext_vf2 = InstructionType::opmvv | OPCODE(0x12) | (0x6 << 15),
};

enum OperandTypes : uint32_t {
    rnIsImm = 1 << 1,
    rmIsImm = 1 << 2,
    rnIsGpr = 1 << 3,
    rmIsGpr = 1 << 4,
    rdIsGpr = 1 << 5
};

enum VectorLengthMultiplyTypes : uint32_t {
    vlMul1 = 0,
    vlMul2 = 1,
    vlMul4 = 2,
    vlMul8 = 3,
    vlMulF2 = 7,
    vlMulF4 = 6,
    vlMulF8 = 5,
};

}; // namespace SimdOp

static void simdEmitVsetivli(struct sljit_compiler* compiler, sljit_s32 type, uint32_t vlmul)
{
    uint32_t elem_size = (uint32_t)(((type) >> 18) & 0x3f);
    uint32_t avl = (uint32_t)1 << (4 - elem_size);

    uint32_t opcode = VSETIVLI | (6 << 7) | (elem_size << 23) | (vlmul << 20) | (avl << 15);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitOp(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, uint32_t optype = 0)
{
    rd = sljit_get_register_index((optype & SimdOp::rdIsGpr) ? SLJIT_GP_REGISTER : SLJIT_SIMD_REG_128, rd);

    if (!(optype & SimdOp::rnIsImm) && !(optype & SimdOp::rnIsGpr)) {
        ASSERT(rn >= SLJIT_VR0);
        rn = sljit_get_register_index(SLJIT_SIMD_REG_128, rn);
    } else if (optype & SimdOp::rnIsGpr) {
        ASSERT(rn >= SLJIT_R0);
        rn = sljit_get_register_index(SLJIT_GP_REGISTER, rn);
    }

    if (!(optype & SimdOp::rmIsImm) && !(optype & SimdOp::rmIsGpr)) {
        ASSERT(rm >= SLJIT_VR0);
        rm = sljit_get_register_index(SLJIT_SIMD_REG_128, rm);
    } else if (optype & SimdOp::rmIsGpr) {
        ASSERT(rm >= SLJIT_R0);
        rm = sljit_get_register_index(SLJIT_GP_REGISTER, rm);
    }

    /* Mapping: rd -> vd, rm -> vs1, rn -> vs2. */
    opcode |= ((uint32_t)rd << 7) | ((uint32_t)rm << 15) | ((uint32_t)rn << 20);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitTypedOp(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, uint32_t optype = 0, uint32_t vlmul = 0)
{
    simdEmitVsetivli(compiler, type, vlmul);
    simdEmitOp(compiler, opcode, rd, rn, rm, optype);
}

static void simdEmitCSRRWI(sljit_compiler* compiler, sljit_s32 rd, uint32_t csr, uint32_t uimm)
{
    rd = sljit_get_register_index(SLJIT_GP_REGISTER, rd);
    uint32_t opcode = 0x73 | ((uint32_t)rd << 7) | (0x5 << 12) | (csr << 20) | (uimm << 15);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitCSRRW(sljit_compiler* compiler, sljit_s32 rd, uint32_t csr, sljit_s32 rs1)
{
    rd = sljit_get_register_index(SLJIT_GP_REGISTER, rd);
    rs1 = sljit_get_register_index(SLJIT_GP_REGISTER, rs1);
    uint32_t opcode = 0x73 | ((uint32_t)rd << 7) | ((uint32_t)rs1 << 15) | (0x1 << 12) | (csr << 20);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitAbs(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 mask = SLJIT_VR0;
    sljit_s32 tmp = rd == rn ? SLJIT_TMP_DEST_VREG : rd;

    simdEmitTypedOp(compiler, type, SimdOp::vmv_vv, tmp, 0, rn, SimdOp::rnIsImm);
    simdEmitOp(compiler, SimdOp::vmslt_vx, mask, rn, 0, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vrsub_vi ^ SimdOp::vm, tmp, rn, 0, SimdOp::rmIsImm);

    if (rd == rn) {
        simdEmitOp(compiler, SimdOp::vmv_vv, rd, 0, tmp, SimdOp::rnIsImm);
    }
}

static void simdEmitPairwiseAdd(sljit_compiler* compiler, sljit_s32 type, bool isSigned, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    sljit_s32 shift = (type == SLJIT_SIMD_ELEM_16) ? 8 : 16;

    simdEmitTypedOp(compiler, type, isSigned ? SimdOp::vsra_vi : SimdOp::vsrl_vi, tmp, rn, shift, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vsll_vi, rd, rn, shift, SimdOp::rmIsImm);
    simdEmitOp(compiler, isSigned ? SimdOp::vsra_vi : SimdOp::vsrl_vi, rd, rd, shift, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vadd_vv, rd, rd, tmp);
}

static void simdEmitAllTrue(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    simdEmitTypedOp(compiler, type, SimdOp::vmv_vi, tmp, 0, (0x1F), SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vredminu_vs, tmp, rn, tmp);
    simdEmitOp(compiler, SimdOp::vmv_xs, rd, 0, tmp, SimdOp::rnIsImm | SimdOp::rdIsGpr);
    struct sljit_jump* notAllTrue = sljit_emit_cmp(compiler, SLJIT_EQUAL, rd, 0, SLJIT_IMM, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, rd, 0, SLJIT_IMM, 1);
    sljit_set_label(notAllTrue, sljit_emit_label(compiler));
}

static void simdEmitAnyTrue(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    simdEmitTypedOp(compiler, type, SimdOp::vmv_sx, tmp, 0, TMP_ZERO, SimdOp::rmIsGpr);
    simdEmitOp(compiler, SimdOp::vredmaxu_vs, tmp, rn, tmp);
    simdEmitOp(compiler, SimdOp::vmv_xs, rd, 0, tmp, SimdOp::rnIsImm | SimdOp::rdIsGpr);
    struct sljit_jump* notAnyTrue = sljit_emit_cmp(compiler, SLJIT_EQUAL, rd, 0, SLJIT_IMM, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, rd, 0, SLJIT_IMM, 1);
    sljit_set_label(notAnyTrue, sljit_emit_label(compiler));
}

static void simdEmitAvgr(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 gptmp = SLJIT_TMP_DEST_REG;
    simdEmitCSRRWI(compiler, gptmp, 0x00A, 0);
    simdEmitTypedOp(compiler, type, SimdOp::vaaddu_vv, rd, rn, rm);
    simdEmitCSRRW(compiler, gptmp, 0x00A, gptmp);
}

static void simdEmitCompare(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, bool reverseMask = false)
{
    sljit_s32 mask = SLJIT_VR0;
    simdEmitTypedOp(compiler, type, opcode, mask, rn, rm);
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    simdEmitOp(compiler, SimdOp::vmv_vi, tmp, 0, reverseMask ? (0x1F) : 0, SimdOp::rmIsImm | SimdOp::rnIsImm);
    simdEmitOp(compiler, SimdOp::vmerge_vi, rd, tmp, reverseMask ? 0 : (0x1F), SimdOp::rmIsImm);
}

static void simdEmitExtendLow(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;

    simdEmitTypedOp(compiler, type, opcode, rd == rn ? tmp : rd, rn, 0, SimdOp::rmIsImm);

    if (rd == rn) {
        simdEmitTypedOp(compiler, type, SimdOp::vmv_vv, rd, 0, tmp, SimdOp::rnIsImm);
    }
}

static void simdEmitExtendHigh(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;

    simdEmitTypedOp(compiler, SLJIT_SIMD_ELEM_8, SimdOp::vslidedown_vi, tmp, rn, 8, SimdOp::rmIsImm);
    simdEmitTypedOp(compiler, type, opcode, rd, tmp, 0, SimdOp::rmIsImm);
}

static void simdEmitExtendLowF2(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;

    simdEmitTypedOp(compiler, type, opcode, rd == rn ? tmp : rd, rn, 0, SimdOp::rmIsImm, SimdOp::vlMulF2);

    if (rd == rn) {
        simdEmitTypedOp(compiler, type, SimdOp::vmv_vv, rd, 0, tmp, SimdOp::rnIsImm);
    }
}

static void simdEmitNarrowZero(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;

    simdEmitVsetivli(compiler, type, 0);

    if (rd == rn) {
        simdEmitOp(compiler, SimdOp::vmv_vv, tmp, 0, rd, SimdOp::rnIsImm);
        rn = tmp;
    }

    simdEmitOp(compiler, SimdOp::vmv_vi, rd, 0, 0, SimdOp::rmIsImm | SimdOp::rnIsImm);
    simdEmitTypedOp(compiler, type, opcode, rd, rn, 0, SimdOp::rmIsImm, SimdOp::vlMulF2);
}

static void simdEmitI32x4DotI16x8(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    simdEmitTypedOp(compiler, SLJIT_SIMD_ELEM_32, SimdOp::vmul_vv, tmp, rn, rm);
    simdEmitTypedOp(compiler, SLJIT_SIMD_ELEM_16, SimdOp::vredsum_vs, rd, tmp, rn);
}

static void simdEmitFCeil(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn)
{
    const int floatMantissaBits = type == (SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64) ? 52 : 23;
    const int floatExponentBits = type == (SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64) ? 11 : 8;
    const int floatExponentBias = type == (SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64) ? 1023 : 127;
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    sljit_s32 gptmp = SLJIT_TMP_DEST_REG;
    sljit_s32 ftmp = SLJIT_TMP_DEST_FREG;
    sljit_s32 mask = SLJIT_VR0;
    simdEmitTypedOp(compiler, type, SimdOp::vmv_vi, tmp, 0, 0, SimdOp::rmIsImm | SimdOp::rnIsImm);
    sljit_emit_op1(compiler, SLJIT_MOV, gptmp, 0, SLJIT_IMM, 64 - floatMantissaBits - floatExponentBits);
    simdEmitOp(compiler, SimdOp::vsll_vx, tmp, rn, gptmp, SimdOp::rmIsGpr);
    sljit_emit_op1(compiler, SLJIT_MOV, gptmp, 0, SLJIT_IMM, 64 - floatExponentBias);
    simdEmitOp(compiler, SimdOp::vsrl_vx, tmp, tmp, gptmp, SimdOp::rmIsGpr);
    sljit_emit_op1(compiler, SLJIT_MOV, gptmp, 0, SLJIT_IMM, floatExponentBias + floatMantissaBits);
    simdEmitOp(compiler, SimdOp::vmslt_vx, mask, tmp, gptmp, SimdOp::rmIsGpr);
    simdEmitCSRRWI(compiler, gptmp, 0x00A, 0x3);
    simdEmitOp(compiler, SimdOp::vmv_vv, rd, 0, rn);
    if (rd == rn) {
        simdEmitOp(compiler, SimdOp::vmv_vv, tmp, 0, rn);
    }
    simdEmitOp(compiler, SimdOp::vfcvt_x_f_v ^ SimdOp::vm, rd, rn, 0, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vfcvt_f_x_v ^ SimdOp::vm, rd, rd, 0, SimdOp::rmIsImm);
    if (rd == rn) {
        simdEmitOp(compiler, SimdOp::vfsgnj_vv, rd, rd, tmp);
    } else {
        simdEmitOp(compiler, SimdOp::vfsgnj_vv, rd, rd, rn);
    }
    simdEmitOp(compiler, SimdOp::vmfeq_vv, mask, rn, rn);
    simdEmitOp(compiler, SimdOp::vxor_vi, mask, mask, (0x1F), SimdOp::rmIsImm);
    uint32_t opC = (sljit_get_register_index(SLJIT_FLOAT_REGISTER, ftmp) << 7) | (TMP_ZERO << 15);
    if (type == (SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32)) {
        opC |= 0x53 | (0x70 << 25);
    } else {
#if (defined SLJIT_CONFIG_RISCV64 && SLJIT_CONFIG_RISCV64)
        opC |= 0x53 | (0x79 << 25);
#else
        opC |= 0x53 | (0x69 << 25);
#endif
    }
    sljit_emit_op_custom(compiler, &opC, sizeof(uint32_t));
    simdEmitOp(compiler, SimdOp::vfadd_vf ^ SimdOp::vm, rd, rn, ftmp);
}

static void simdEmitFMinMax(sljit_compiler* compiler, sljit_s32 type, sljit_s32 opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 mask = SLJIT_VR0;
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    sljit_s32 gptmp = SLJIT_TMP_DEST_REG;

    simdEmitTypedOp(compiler, type, SimdOp::vmfeq_vv, mask, rn, rn);
    simdEmitOp(compiler, SimdOp::vmfeq_vv, tmp, rm, rm);
    simdEmitOp(compiler, SimdOp::vand_vv, mask, mask, tmp);
    sljit_emit_op1(compiler, SLJIT_MOV, gptmp, 0, SLJIT_IMM, type == (SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64) ? 0x7FF8000000000000 : 0x7FC00000U);
    simdEmitOp(compiler, SimdOp::vmv_vx, tmp, 0, gptmp, SimdOp::rmIsGpr | SimdOp::rnIsImm);
    simdEmitOp(compiler, opcode ^ SimdOp::vm, tmp, rn, rm);
    simdEmitOp(compiler, SimdOp::vmv_vv, rd, 0, tmp, SimdOp::rnIsImm);
}

static void simdEmitTruncSat(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;

    simdEmitTypedOp(compiler, type, SimdOp::vmfne_vv, SLJIT_VR0, rn, rn);
    simdEmitOp(compiler, SimdOp::vmerge_vi, tmp, rn, 0, SimdOp::rmIsImm);

    if (opcode == SimdOp::vfncvt_rtz_x_f_w || opcode == SimdOp::vfncvt_rtz_xu_f_w) {
        simdEmitOp(compiler, SimdOp::vmv_vi, rd, 0, 0, SimdOp::rmIsImm | SimdOp::rnIsImm);

        /* 8 byte result. */
        uint32_t opcode = VSETIVLI | (6 << 7) | (2 << 23) | (2 << 15);
        sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
    }

    simdEmitOp(compiler, opcode, rd, tmp, 0, SimdOp::rmIsImm);
}

static void SimdEmitFTrunc(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;

    simdEmitTypedOp(compiler, type, SimdOp::vfcvt_x_f_v, tmp, rn, 0, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vfcvt_f_x_v, rd, tmp, 0, SimdOp::rmIsImm);
}

static void simdEmitPMinMax(sljit_compiler* compiler, sljit_s32 type, bool min, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 mask = SLJIT_VR0;

    simdEmitTypedOp(compiler, type, SimdOp::vmflt_vv, mask, min ? rm : rn, min ? rn : rm);
    simdEmitOp(compiler, SimdOp::vmerge_vv, rd, rn, rm);
}

static void simdEmitPopcnt(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn, sljit_s32 rt)
{
#if defined(__riscv_zvbb)
    simdEmitTypedOp(compiler, type, SimdOp::vcpop_v, rd, rn, 0, SimdOp::rmIsImm);
#else
    sljit_s32 mask = SLJIT_VR0;
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    sljit_s32 tmpgp = SLJIT_TMP_DEST_REG;
    simdEmitTypedOp(compiler, type, SimdOp::vmv_vv, tmp, 0, rn);
    simdEmitOp(compiler, SimdOp::vmv_vi, rd, 0, 0, SimdOp::rmIsImm);
    struct sljit_label* label = sljit_emit_label(compiler);
    simdEmitOp(compiler, SimdOp::vmsne_vi, mask, tmp, 0);
    simdEmitOp(compiler, SimdOp::vadd_vi ^ SimdOp::vm, rd, rd, 1, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vadd_vi ^ SimdOp::vm, rt, tmp, (0x1F), SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vand_vv, tmp, tmp, rt);
    uint32_t opcode = SimdOp::vfirst_m | sljit_get_register_index(SLJIT_GP_REGISTER, tmpgp) << 7 | sljit_get_register_index(SLJIT_SIMD_REG_128, tmp) << 20;
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
    struct sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_SIG_LESS, tmpgp, 0, SLJIT_IMM, 0);
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), label);
    sljit_set_label(jump, sljit_emit_label(compiler));
#endif
}

static void simdEmitSwizzle(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    if (rd == rn) {
        simdEmitTypedOp(compiler, type, SimdOp::vrgather_vv, tmp, rn, rm);
        simdEmitOp(compiler, SimdOp::vmv_vv, rd, 0, tmp);
    } else {
        simdEmitTypedOp(compiler, type, SimdOp::vrgather_vv, rd, rn, rm);
    }
}

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::I8X16NegOpcode:
    case ByteCode::I8X16AbsOpcode:
    case ByteCode::I8X16PopcntOpcode:
    case ByteCode::V128NotOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8NegOpcode:
    case ByteCode::I16X8AbsOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4NegOpcode:
    case ByteCode::I32X4AbsOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4TruncSatF32X4SOpcode:
    case ByteCode::I32X4TruncSatF32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        srcType = SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2NegOpcode:
    case ByteCode::I64X2AbsOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ExtendLowI32X4SOpcode:
    case ByteCode::I64X2ExtendHighI32X4SOpcode:
    case ByteCode::I64X2ExtendLowI32X4UOpcode:
    case ByteCode::I64X2ExtendHighI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4AbsOpcode:
    case ByteCode::F32X4NegOpcode:
    case ByteCode::F32X4SqrtOpcode:
    case ByteCode::F32X4CeilOpcode:
    case ByteCode::F32X4FloorOpcode:
    case ByteCode::F32X4TruncOpcode:
    case ByteCode::F32X4NearestOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F32X4ConvertI32X4SOpcode:
    case ByteCode::F32X4ConvertI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F64X2AbsOpcode:
    case ByteCode::F64X2NegOpcode:
    case ByteCode::F64X2SqrtOpcode:
    case ByteCode::F64X2CeilOpcode:
    case ByteCode::F64X2FloorOpcode:
    case ByteCode::F64X2TruncOpcode:
    case ByteCode::F64X2NearestOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F64X2PromoteLowF32X4Opcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));

    args[1].set(operands + 1);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, instr->requiredReg(0));

    switch (instr->opcode()) {
    case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        simdEmitNarrowZero(compiler, srcType, SimdOp::vfncvt_f_f_w, dst, args[0].arg);
        break;
    case ByteCode::F32X4ConvertI32X4SOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfcvt_f_x_v, dst, args[0].arg, 0, SimdOp::rmIsImm);
        break;
    case ByteCode::F32X4ConvertI32X4UOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfcvt_f_xu_v, dst, args[0].arg, 0, SimdOp::rmIsImm);
        break;
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
        simdEmitExtendLowF2(compiler, srcType, SimdOp::vfwcvt_f_x_v, dst, args[0].arg);
        break;
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        simdEmitExtendLowF2(compiler, srcType, SimdOp::vfwcvt_f_xu_v, dst, args[0].arg);
        break;
    case ByteCode::V128NotOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vxor_vi, dst, args[0].arg, (0x1F), SimdOp::rmIsImm);
        break;
    case ByteCode::I8X16NegOpcode:
    case ByteCode::I16X8NegOpcode:
    case ByteCode::I32X4NegOpcode:
    case ByteCode::I64X2NegOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vrsub_vi, dst, args[0].arg, 0, SimdOp::rmIsImm);
        break;
    case ByteCode::I8X16AbsOpcode:
    case ByteCode::I16X8AbsOpcode:
    case ByteCode::I32X4AbsOpcode:
    case ByteCode::I64X2AbsOpcode:
        simdEmitAbs(compiler, srcType, dst, args[0].arg);
        break;
    case ByteCode::I8X16PopcntOpcode:
        simdEmitPopcnt(compiler, srcType, dst, args[0].arg, instr->requiredReg(1));
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
        simdEmitPairwiseAdd(compiler, dstType, true, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        simdEmitPairwiseAdd(compiler, dstType, false, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
    case ByteCode::I64X2ExtendLowI32X4SOpcode:
        simdEmitExtendLow(compiler, dstType, SimdOp::vsext_vf2, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
    case ByteCode::I64X2ExtendHighI32X4SOpcode:
        simdEmitExtendHigh(compiler, dstType, SimdOp::vsext_vf2, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
    case ByteCode::I64X2ExtendLowI32X4UOpcode:
        simdEmitExtendLow(compiler, dstType, SimdOp::vzext_vf2, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
    case ByteCode::I64X2ExtendHighI32X4UOpcode:
        simdEmitExtendHigh(compiler, dstType, SimdOp::vzext_vf2, dst, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF32X4SOpcode:
        simdEmitTruncSat(compiler, srcType, SimdOp::vfcvt_rtz_x_f_v, dst, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF32X4UOpcode:
        simdEmitTruncSat(compiler, srcType, SimdOp::vfcvt_rtz_xu_f_v, dst, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
        simdEmitTruncSat(compiler, srcType, SimdOp::vfncvt_rtz_x_f_w, dst, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        simdEmitTruncSat(compiler, srcType, SimdOp::vfncvt_rtz_xu_f_w, dst, args[0].arg);
        break;
    case ByteCode::F32X4AbsOpcode:
    case ByteCode::F64X2AbsOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfsgnjx_vv, dst, args[0].arg, args[0].arg);
        break;
    case ByteCode::F32X4CeilOpcode:
    case ByteCode::F64X2CeilOpcode:
        simdEmitFCeil(compiler, srcType, dst, args[0].arg);
        break;
    case ByteCode::F32X4FloorOpcode:
    case ByteCode::F64X2FloorOpcode:
        break;
    case ByteCode::F32X4NearestOpcode:
    case ByteCode::F64X2NearestOpcode:
        break;
    case ByteCode::F32X4NegOpcode:
    case ByteCode::F64X2NegOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfsgnjn_vv, dst, args[0].arg, args[0].arg);
        break;
    case ByteCode::F32X4SqrtOpcode:
    case ByteCode::F64X2SqrtOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfsqrt_v, dst, args[0].arg, 0, SimdOp::rmIsImm);
        break;
    case ByteCode::F32X4TruncOpcode:
    case ByteCode::F64X2TruncOpcode:
        SimdEmitFTrunc(compiler, srcType, dst, args[0].arg);
        break;
    case ByteCode::F64X2PromoteLowF32X4Opcode:
        simdEmitExtendLowF2(compiler, srcType, SimdOp::vfwcvt_f_f_v, dst, args[0].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[1].arg, args[1].argw);
    }
}

static bool emitUnaryCondSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 type = SLJIT_NOT_EQUAL;

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        type = SLJIT_ORDERED_EQUAL;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::V128AnyTrueOpcode);
        srcType = SLJIT_SIMD_ELEM_8;
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));

    sljit_s32 dst = SLJIT_TMP_DEST_REG;

    if (!(instr->info() & Instruction::kIsMergeCompare)) {
        args[1].set(operands + 1);
        dst = GET_TARGET_REG(args[1].arg, SLJIT_TMP_DEST_REG);
    }

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
    case ByteCode::I16X8AllTrueOpcode:
    case ByteCode::I32X4AllTrueOpcode:
    case ByteCode::I64X2AllTrueOpcode:
        simdEmitAllTrue(compiler, srcType, dst, args[0].arg);
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::V128AnyTrueOpcode);
        simdEmitAnyTrue(compiler, srcType, dst, args[0].arg);
        break;
    }

    ASSERT(instr->next() != nullptr);

    if (instr->info() & Instruction::kIsMergeCompare) {
        return true;
    }

    return false;
}

static void simdEmitExtmul(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_VREG;
    bool useTmp = (rd == rn || rd == rm);

    simdEmitTypedOp(compiler, type, opcode, useTmp ? tmp : rd, rn, rm, 0, SimdOp::vlMulF2);

    if (useTmp) {
        simdEmitTypedOp(compiler, type, SimdOp::vmv_vv, rd, 0, tmp, SimdOp::rnIsImm);
    }
}

static void simdEmitExtmulHigh(sljit_compiler* compiler, sljit_s32 type, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp1 = SLJIT_TMP_DEST_VREG;
    sljit_s32 tmp2 = SLJIT_VR0;

    simdEmitTypedOp(compiler, SLJIT_SIMD_ELEM_8, SimdOp::vslidedown_vi, tmp1, rn, 8, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vslidedown_vi, tmp2, rm, 8, SimdOp::rmIsImm);

    simdEmitTypedOp(compiler, type, opcode, rd, tmp1, tmp2, 0, SimdOp::vlMulF2);
}

static void simdEmitNarrowSigned(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp1 = SLJIT_TMP_DEST_VREG;
    sljit_s32 tmp2 = SLJIT_VR0;

    ASSERT(type == SLJIT_SIMD_ELEM_8 || type == SLJIT_SIMD_ELEM_16);

    simdEmitTypedOp(compiler, type, SimdOp::vnclip_wi, tmp2, rm, 0, SimdOp::rmIsImm, SimdOp::vlMulF2);
    simdEmitOp(compiler, SimdOp::vnclip_wi, rd == rn ? tmp1 : rd, rn, 0, SimdOp::rmIsImm);

    simdEmitVsetivli(compiler, SLJIT_SIMD_ELEM_8, 0);

    if (rd == rn) {
        simdEmitOp(compiler, SimdOp::vmv_vv, rd, 0, tmp1, SimdOp::rnIsImm);
    }

    simdEmitOp(compiler, SimdOp::vslideup_vi, rd, tmp2, 8, SimdOp::rmIsImm);
}

static void simdEmitNarrowUnsigned(sljit_compiler* compiler, sljit_s32 type, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp1 = SLJIT_TMP_DEST_VREG;
    sljit_s32 tmp2 = SLJIT_VR0;
    sljit_s32 srcType = (type == SLJIT_SIMD_ELEM_8) ? SLJIT_SIMD_ELEM_16 : SLJIT_SIMD_ELEM_32;

    ASSERT(type == SLJIT_SIMD_ELEM_8 || type == SLJIT_SIMD_ELEM_16);
    simdEmitTypedOp(compiler, srcType, SimdOp::vmax_vx, tmp1, rn, 0, SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vmax_vx, rd, rm, 0, SimdOp::rmIsImm);

    simdEmitTypedOp(compiler, type, SimdOp::vnclipu_wi, tmp2, rd, 0, SimdOp::rmIsImm, SimdOp::vlMulF2);
    simdEmitOp(compiler, SimdOp::vnclipu_wi, rd, tmp1, 0, SimdOp::rmIsImm);

    simdEmitTypedOp(compiler, SLJIT_SIMD_ELEM_8, SimdOp::vslideup_vi, rd, tmp2, 8, SimdOp::rmIsImm);
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::I8X16AddOpcode:
    case ByteCode::I8X16SubOpcode:
    case ByteCode::I8X16AddSatSOpcode:
    case ByteCode::I8X16AddSatUOpcode:
    case ByteCode::I8X16SubSatSOpcode:
    case ByteCode::I8X16SubSatUOpcode:
    case ByteCode::I8X16EqOpcode:
    case ByteCode::I8X16NeOpcode:
    case ByteCode::I8X16LtSOpcode:
    case ByteCode::I8X16LtUOpcode:
    case ByteCode::I8X16LeSOpcode:
    case ByteCode::I8X16LeUOpcode:
    case ByteCode::I8X16GtSOpcode:
    case ByteCode::I8X16GtUOpcode:
    case ByteCode::I8X16GeSOpcode:
    case ByteCode::I8X16GeUOpcode:
    case ByteCode::I8X16MinSOpcode:
    case ByteCode::I8X16MinUOpcode:
    case ByteCode::I8X16MaxSOpcode:
    case ByteCode::I8X16MaxUOpcode:
    case ByteCode::I8X16AvgrUOpcode:
    case ByteCode::I8X16SwizzleOpcode:
    case ByteCode::V128AndOpcode:
    case ByteCode::V128OrOpcode:
    case ByteCode::V128XorOpcode:
    case ByteCode::V128AndnotOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I8X16NarrowI16X8SOpcode:
    case ByteCode::I8X16NarrowI16X8UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8AddOpcode:
    case ByteCode::I16X8SubOpcode:
    case ByteCode::I16X8MulOpcode:
    case ByteCode::I16X8AddSatSOpcode:
    case ByteCode::I16X8AddSatUOpcode:
    case ByteCode::I16X8SubSatSOpcode:
    case ByteCode::I16X8SubSatUOpcode:
    case ByteCode::I16X8EqOpcode:
    case ByteCode::I16X8NeOpcode:
    case ByteCode::I16X8LtSOpcode:
    case ByteCode::I16X8LtUOpcode:
    case ByteCode::I16X8LeSOpcode:
    case ByteCode::I16X8LeUOpcode:
    case ByteCode::I16X8GtSOpcode:
    case ByteCode::I16X8GtUOpcode:
    case ByteCode::I16X8GeSOpcode:
    case ByteCode::I16X8GeUOpcode:
    case ByteCode::I16X8MinSOpcode:
    case ByteCode::I16X8MinUOpcode:
    case ByteCode::I16X8MaxSOpcode:
    case ByteCode::I16X8MaxUOpcode:
    case ByteCode::I16X8AvgrUOpcode:
    case ByteCode::I16X8Q15mulrSatSOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8ExtmulLowI8X16SOpcode:
    case ByteCode::I16X8ExtmulHighI8X16SOpcode:
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8NarrowI32X4SOpcode:
    case ByteCode::I16X8NarrowI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4AddOpcode:
    case ByteCode::I32X4SubOpcode:
    case ByteCode::I32X4MulOpcode:
    case ByteCode::I32X4EqOpcode:
    case ByteCode::I32X4NeOpcode:
    case ByteCode::I32X4LtSOpcode:
    case ByteCode::I32X4LtUOpcode:
    case ByteCode::I32X4LeSOpcode:
    case ByteCode::I32X4LeUOpcode:
    case ByteCode::I32X4GtSOpcode:
    case ByteCode::I32X4GtUOpcode:
    case ByteCode::I32X4GeSOpcode:
    case ByteCode::I32X4GeUOpcode:
    case ByteCode::I32X4MinSOpcode:
    case ByteCode::I32X4MinUOpcode:
    case ByteCode::I32X4MaxSOpcode:
    case ByteCode::I32X4MaxUOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
    case ByteCode::I32X4DotI16X8SOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2AddOpcode:
    case ByteCode::I64X2SubOpcode:
    case ByteCode::I64X2MulOpcode:
    case ByteCode::I64X2EqOpcode:
    case ByteCode::I64X2NeOpcode:
    case ByteCode::I64X2LtSOpcode:
    case ByteCode::I64X2LeSOpcode:
    case ByteCode::I64X2GtSOpcode:
    case ByteCode::I64X2GeSOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4AddOpcode:
    case ByteCode::F32X4SubOpcode:
    case ByteCode::F32X4MulOpcode:
    case ByteCode::F32X4DivOpcode:
    case ByteCode::F32X4EqOpcode:
    case ByteCode::F32X4NeOpcode:
    case ByteCode::F32X4LtOpcode:
    case ByteCode::F32X4LeOpcode:
    case ByteCode::F32X4GtOpcode:
    case ByteCode::F32X4GeOpcode:
    case ByteCode::F32X4PMinOpcode:
    case ByteCode::F32X4PMaxOpcode:
    case ByteCode::F32X4MaxOpcode:
    case ByteCode::F32X4MinOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2AddOpcode:
    case ByteCode::F64X2SubOpcode:
    case ByteCode::F64X2MulOpcode:
    case ByteCode::F64X2DivOpcode:
    case ByteCode::F64X2SqrtOpcode:
    case ByteCode::F64X2EqOpcode:
    case ByteCode::F64X2NeOpcode:
    case ByteCode::F64X2LtOpcode:
    case ByteCode::F64X2LeOpcode:
    case ByteCode::F64X2GtOpcode:
    case ByteCode::F64X2GeOpcode:
    case ByteCode::F64X2PMinOpcode:
    case ByteCode::F64X2PMaxOpcode:
    case ByteCode::F64X2MaxOpcode:
    case ByteCode::F64X2MinOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], srcType, instr->requiredReg(1));

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, instr->requiredReg(2));

    switch (instr->opcode()) {
    case ByteCode::I8X16AddOpcode:
    case ByteCode::I16X8AddOpcode:
    case ByteCode::I32X4AddOpcode:
    case ByteCode::I64X2AddOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vadd_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubOpcode:
    case ByteCode::I16X8SubOpcode:
    case ByteCode::I32X4SubOpcode:
    case ByteCode::I64X2SubOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vsub_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatSOpcode:
    case ByteCode::I16X8AddSatSOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vsadd_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatUOpcode:
    case ByteCode::I16X8AddSatUOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vsaddu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatSOpcode:
    case ByteCode::I16X8SubSatSOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vssub_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatUOpcode:
    case ByteCode::I16X8SubSatUOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vssubu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16EqOpcode:
    case ByteCode::I16X8EqOpcode:
    case ByteCode::I32X4EqOpcode:
    case ByteCode::I64X2EqOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmseq_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NeOpcode:
    case ByteCode::I16X8NeOpcode:
    case ByteCode::I32X4NeOpcode:
    case ByteCode::I64X2NeOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmsne_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16LtSOpcode:
    case ByteCode::I16X8LtSOpcode:
    case ByteCode::I32X4LtSOpcode:
    case ByteCode::I64X2LtSOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmslt_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16LtUOpcode:
    case ByteCode::I16X8LtUOpcode:
    case ByteCode::I32X4LtUOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmsltu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16LeSOpcode:
    case ByteCode::I16X8LeSOpcode:
    case ByteCode::I32X4LeSOpcode:
    case ByteCode::I64X2LeSOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmsle_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16LeUOpcode:
    case ByteCode::I16X8LeUOpcode:
    case ByteCode::I32X4LeUOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmsleu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GtSOpcode:
    case ByteCode::I16X8GtSOpcode:
    case ByteCode::I32X4GtSOpcode:
    case ByteCode::I64X2GtSOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmsle_vv, dst, args[0].arg, args[1].arg, true);
        break;
    case ByteCode::I8X16GtUOpcode:
    case ByteCode::I16X8GtUOpcode:
    case ByteCode::I32X4GtUOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmsleu_vv, dst, args[0].arg, args[1].arg, true);
        break;
    case ByteCode::I8X16GeSOpcode:
    case ByteCode::I16X8GeSOpcode:
    case ByteCode::I32X4GeSOpcode:
    case ByteCode::I64X2GeSOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmslt_vv, dst, args[0].arg, args[1].arg, true);
        break;
    case ByteCode::I8X16GeUOpcode:
    case ByteCode::I16X8GeUOpcode:
    case ByteCode::I32X4GeUOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmsltu_vv, dst, args[0].arg, args[1].arg, true);
        break;
    case ByteCode::I8X16MinSOpcode:
    case ByteCode::I16X8MinSOpcode:
    case ByteCode::I32X4MinSOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vmin_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16MinUOpcode:
    case ByteCode::I16X8MinUOpcode:
    case ByteCode::I32X4MinUOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vminu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16MaxSOpcode:
    case ByteCode::I16X8MaxSOpcode:
    case ByteCode::I32X4MaxSOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vmax_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16MaxUOpcode:
    case ByteCode::I16X8MaxUOpcode:
    case ByteCode::I32X4MaxUOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vmaxu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AvgrUOpcode:
    case ByteCode::I16X8AvgrUOpcode:
        simdEmitAvgr(compiler, srcType, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NarrowI16X8SOpcode:
    case ByteCode::I16X8NarrowI32X4SOpcode:
        simdEmitNarrowSigned(compiler, dstType, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NarrowI16X8UOpcode:
    case ByteCode::I16X8NarrowI32X4UOpcode:
        simdEmitNarrowUnsigned(compiler, dstType, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MulOpcode:
    case ByteCode::I32X4MulOpcode:
    case ByteCode::I64X2MulOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vmul_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulLowI8X16SOpcode:
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
        simdEmitExtmul(compiler, srcType, SimdOp::vwmul_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16SOpcode:
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
        simdEmitExtmulHigh(compiler, srcType, SimdOp::vwmul_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
        simdEmitExtmul(compiler, srcType, SimdOp::vwmulu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        simdEmitExtmulHigh(compiler, srcType, SimdOp::vwmulu_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8Q15mulrSatSOpcode:
        break;
    case ByteCode::I32X4DotI16X8SOpcode:
        simdEmitI32x4DotI16x8(compiler, srcType, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4AddOpcode:
    case ByteCode::F64X2AddOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfadd_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4SubOpcode:
    case ByteCode::F64X2SubOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfsub_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MulOpcode:
    case ByteCode::F64X2MulOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfmul_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4DivOpcode:
    case ByteCode::F64X2DivOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vfdiv_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MaxOpcode:
    case ByteCode::F64X2MaxOpcode:
        simdEmitFMinMax(compiler, srcType, SimdOp::vfmax_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MinOpcode:
    case ByteCode::F64X2MinOpcode:
        simdEmitFMinMax(compiler, srcType, SimdOp::vfmin_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4PMinOpcode:
    case ByteCode::F64X2PMinOpcode:
        simdEmitPMinMax(compiler, srcType, true, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4PMaxOpcode:
    case ByteCode::F64X2PMaxOpcode:
        simdEmitPMinMax(compiler, srcType, false, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4EqOpcode:
    case ByteCode::F64X2EqOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmfeq_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4NeOpcode:
    case ByteCode::F64X2NeOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmfne_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4LtOpcode:
    case ByteCode::F64X2LtOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmflt_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4LeOpcode:
    case ByteCode::F64X2LeOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmfle_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4GtOpcode:
    case ByteCode::F64X2GtOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmflt_vv, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::F32X4GeOpcode:
    case ByteCode::F64X2GeOpcode:
        simdEmitCompare(compiler, srcType, SimdOp::vmfle_vv, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::V128AndOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vand_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128OrOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vor_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128XorOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vxor_vv, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128AndnotOpcode:
        simdEmitTypedOp(compiler, srcType, SimdOp::vxor_vi, SLJIT_TMP_DEST_VREG, args[1].arg, (0x1F), SimdOp::rmIsImm);
        simdEmitOp(compiler, SimdOp::vand_vv, dst, args[0].arg, SLJIT_TMP_DEST_VREG);
        break;
    case ByteCode::I8X16SwizzleOpcode:
        simdEmitSwizzle(compiler, srcType, dst, args[0].arg, args[1].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[2].arg, args[2].argw);
    }
}

static void simdEmitBitSelect(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, sljit_s32 ro)
{
    sljit_s32 tmp = SLJIT_TMP_DEST_FREG;

    simdEmitVsetivli(compiler, SLJIT_SIMD_ELEM_8, 0);
    simdEmitOp(compiler, SimdOp::vxor_vi, SLJIT_TMP_DEST_VREG, ro, (0x1F), SimdOp::rmIsImm);
    simdEmitOp(compiler, SimdOp::vand_vv, SLJIT_TMP_DEST_VREG, SLJIT_TMP_DEST_VREG, rm);
    simdEmitOp(compiler, SimdOp::vand_vv, rd, rn, ro);
    simdEmitOp(compiler, SimdOp::vor_vv, rd, rd, SLJIT_TMP_DEST_VREG);
}

static void emitTernarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[4];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::V128BitSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_128;
        dstType = SLJIT_SIMD_ELEM_128;
        break;
    case ByteCode::I8X16RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I32X4DotI8X16I7X16AddSOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F32X4RelaxedMaddOpcode:
    case ByteCode::F32X4RelaxedNmaddOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2RelaxedMaddOpcode:
    case ByteCode::F64X2RelaxedNmaddOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], srcType, instr->requiredReg(1));
    simdOperandToArg(compiler, operands + 2, args[2], dstType, instr->requiredReg(2));

    args[3].set(operands + 3);
    sljit_s32 dst = GET_TARGET_REG(args[3].arg, instr->requiredReg(2));

    switch (instr->opcode()) {
    case ByteCode::V128BitSelectOpcode:
        simdEmitBitSelect(compiler, dst, args[0].arg, args[1].arg, args[2].arg);
        break;
    case ByteCode::I8X16RelaxedLaneSelectOpcode:
        break;
    case ByteCode::I16X8RelaxedLaneSelectOpcode:
        break;
    case ByteCode::I32X4RelaxedLaneSelectOpcode:
        break;
    case ByteCode::I64X2RelaxedLaneSelectOpcode:
        break;
    case ByteCode::I32X4DotI8X16I7X16AddSOpcode:
        break;
    case ByteCode::F32X4RelaxedMaddOpcode:
        break;
    case ByteCode::F32X4RelaxedNmaddOpcode:
        break;
    case ByteCode::F64X2RelaxedMaddOpcode:
        break;
    case ByteCode::F64X2RelaxedNmaddOpcode:
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[3].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[3].arg, args[3].argw);
    }
}

static void emitSelectSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, instr->requiredReg(1));
    simdOperandToArg(compiler, operands + 2, args[2], SLJIT_SIMD_ELEM_128, instr->requiredReg(2));

    args[2].set(operands + 3);
    if (SLJIT_IS_MEM(args[2].arg)) {
    }
}

static void emitShuffleSIMD(sljit_compiler* compiler, Instruction* instr)
{
}

static void emitShiftSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    uint32_t op = 0;
    int div = 8;
    int mask = 0x1F;

    args[1].set(operands + 1);

    const bool isImm = SLJIT_IS_IMM(args[1].arg);
    sljit_s32 type = SLJIT_SIMD_ELEM_8;

    switch (instr->opcode()) {
    case ByteCode::I8X16ShlOpcode:
        op = isImm ? SimdOp::vsll_vi : SimdOp::vsll_vx;
        break;
    case ByteCode::I8X16ShrSOpcode:
        op = isImm ? SimdOp::vsra_vi : SimdOp::vsra_vx;
        break;
    case ByteCode::I8X16ShrUOpcode:
        op = isImm ? SimdOp::vsrl_vi : SimdOp::vsrl_vx;
        break;
    case ByteCode::I16X8ShlOpcode:
        op = isImm ? SimdOp::vsll_vi : SimdOp::vsll_vx;
        div = 16;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8ShrSOpcode:
        op = isImm ? SimdOp::vsra_vi : SimdOp::vsra_vx;
        div = 16;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8ShrUOpcode:
        op = isImm ? SimdOp::vsrl_vi : SimdOp::vsrl_vx;
        div = 16;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4ShlOpcode:
        op = isImm ? SimdOp::vsll_vi : SimdOp::vsll_vx;
        div = 32;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ShrSOpcode:
        op = isImm ? SimdOp::vsra_vi : SimdOp::vsra_vx;
        div = 32;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ShrUOpcode:
        op = isImm ? SimdOp::vsrl_vi : SimdOp::vsrl_vx;
        div = 32;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2ShlOpcode:
        op = isImm ? SimdOp::vsll_vi : SimdOp::vsll_vx;
        div = 64;
        type = SLJIT_SIMD_ELEM_64;
        mask = 0x3F;
        break;
    case ByteCode::I64X2ShrSOpcode:
        op = isImm ? SimdOp::vsra_vi : SimdOp::vsra_vx;
        div = 64;
        type = SLJIT_SIMD_ELEM_64;
        mask = 0x3F;
        break;
    case ByteCode::I64X2ShrUOpcode:
        op = isImm ? SimdOp::vsrl_vi : SimdOp::vsrl_vx;
        div = 64;
        type = SLJIT_SIMD_ELEM_64;
        mask = 0x3F;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    simdOperandToArg(compiler, operands, args[0], type, instr->requiredReg(0));

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, instr->requiredReg(0));

    if (isImm) {
        args[1].argw &= mask;

        if (args[1].argw == 0) {
            if (args[2].arg != args[0].arg) {
                sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, args[0].arg, args[2].arg, args[2].argw);
            }
            return;
        }
        args[1].argw %= div;
    }

    simdEmitTypedOp(compiler, type, op, dst, args[0].arg, isImm ? args[1].argw : args[1].arg, isImm ? SimdOp::rmIsImm : SimdOp::rmIsGpr);

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}
