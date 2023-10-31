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

/* Only included by jit-backend.cc */

namespace SimdOp {

enum OpcodeFlags : uint32_t {
    prefix66 = 1 << 16,
    prefixF2 = 2 << 16,
    prefixF3 = 3 << 16,
    prefixMask = 3 << 16,
    opcodeHasImm = 1 << 18,
    opcodeHasArg = 1 << 19,
    opcode38 = 1 << 20,
    opcode3A = 1 << 21,
    opcodeW0 = 1 << 22,
    opcodeW1 = 1 << 23,
    reg256 = 1 << 24
};

#define OPCODE_AND_IMM(opcode, arg) ((opcode) | ((arg) << 8) | SimdOp::opcodeHasImm)

enum Type : uint32_t {
    addpd = 0x58 | SimdOp::prefix66,
    addps = 0x58,
    andpd = 0x54 | SimdOp::prefix66,
    andps = 0x54,
    andnpd = 0x55 | SimdOp::prefix66,
    andnps = 0x55,
    blendps = 0x0c | SimdOp::opcode3A | SimdOp::prefix66,
    cmpeqpd = OPCODE_AND_IMM(0xc2, 0) | SimdOp::prefix66,
    cmpeqps = OPCODE_AND_IMM(0xc2, 0),
    cmplepd = OPCODE_AND_IMM(0xc2, 2) | SimdOp::prefix66,
    cmpleps = OPCODE_AND_IMM(0xc2, 2),
    cmpltpd = OPCODE_AND_IMM(0xc2, 1) | SimdOp::prefix66,
    cmpltps = OPCODE_AND_IMM(0xc2, 1),
    cmpunordpd = OPCODE_AND_IMM(0xc2, 3) | SimdOp::prefix66,
    cmpunordps = OPCODE_AND_IMM(0xc2, 3),
    cmpneqpd = OPCODE_AND_IMM(0xc2, 4) | SimdOp::prefix66,
    cmpneqps = OPCODE_AND_IMM(0xc2, 4),
    cmpnltpd = OPCODE_AND_IMM(0xc2, 5) | SimdOp::prefix66,
    cmpnltps = OPCODE_AND_IMM(0xc2, 5),
    cvtdq2pd = 0xe6 | SimdOp::prefixF3,
    cvtdq2ps = 0x5b,
    cvtpd2ps = 0x5a | SimdOp::prefix66,
    cvtps2pd = 0x5a,
    cvttpd2dq = 0xe6 | SimdOp::prefix66,
    cvttps2dq = 0x5b | SimdOp::prefixF3,
    divpd = 0x5e | SimdOp::prefix66,
    divps = 0x5e,
    maxpd = 0x5f | SimdOp::prefix66,
    maxps = 0x5f,
    minpd = 0x5d | SimdOp::prefix66,
    minps = 0x5d,
    mulpd = 0x59 | SimdOp::prefix66,
    mulps = 0x59,
    orpd = 0x56 | SimdOp::prefix66,
    orps = 0x56,
    packssdw = 0x6b | SimdOp::prefix66,
    packsswb = 0x63 | SimdOp::prefix66,
    packusdw = 0x2b | SimdOp::opcode38 | SimdOp::prefix66,
    packuswb = 0x67 | SimdOp::prefix66,
    paddb = 0xfc | SimdOp::prefix66,
    paddd = 0xfe | SimdOp::prefix66,
    paddq = 0xd4 | SimdOp::prefix66,
    paddsb = 0xec | SimdOp::prefix66,
    paddsw = 0xed | SimdOp::prefix66,
    paddusb = 0xdc | SimdOp::prefix66,
    paddusw = 0xdd | SimdOp::prefix66,
    paddw = 0xfd | SimdOp::prefix66,
    pand = 0xdb | SimdOp::prefix66,
    pandn = 0xdf | SimdOp::prefix66,
    pblendw = 0x0e | SimdOp::opcode3A | SimdOp::prefix66,
    pcmpeqb = 0x74 | SimdOp::prefix66,
    pcmpeqd = 0x76 | SimdOp::prefix66,
    pcmpeqq = 0x29 | SimdOp::opcode38 | SimdOp::prefix66,
    pcmpeqw = 0x75 | SimdOp::prefix66,
    pcmpgtb = 0x64 | SimdOp::prefix66,
    pcmpgtd = 0x66 | SimdOp::prefix66,
    pcmpgtq = 0x37 | SimdOp::opcode38 | SimdOp::prefix66, // SSE4.2
    pcmpgtw = 0x65 | SimdOp::prefix66,
    pmaddubsw = 0x04 | SimdOp::opcode38 | SimdOp::prefix66,
    pmaddwd = 0xf5 | SimdOp::prefix66,
    pmaxsb = 0x3c | SimdOp::opcode38 | SimdOp::prefix66,
    pmaxsd = 0x3d | SimdOp::opcode38 | SimdOp::prefix66,
    pmaxsw = 0xee | SimdOp::prefix66,
    pmaxub = 0xde | SimdOp::prefix66,
    pmaxud = 0x3f | SimdOp::opcode38 | SimdOp::prefix66,
    pmaxuw = 0x3e | SimdOp::opcode38 | SimdOp::prefix66,
    pminsb = 0x38 | SimdOp::opcode38 | SimdOp::prefix66,
    pminsd = 0x39 | SimdOp::opcode38 | SimdOp::prefix66,
    pminsw = 0xea | SimdOp::prefix66,
    pminub = 0xda | SimdOp::prefix66,
    pminud = 0x3b | SimdOp::opcode38 | SimdOp::prefix66,
    pminuw = 0x3a | SimdOp::opcode38 | SimdOp::prefix66,
    pmovsxbw = 0x20 | SimdOp::opcode38 | SimdOp::prefix66,
    pmovsxwd = 0x23 | SimdOp::opcode38 | SimdOp::prefix66,
    pmovzxbw = 0x30 | SimdOp::opcode38 | SimdOp::prefix66,
    pmovzxwd = 0x33 | SimdOp::opcode38 | SimdOp::prefix66,
    pmuldq = 0x28 | SimdOp::opcode38 | SimdOp::prefix66,
    pmulhrsw = 0x0b | SimdOp::opcode38 | SimdOp::prefix66,
    pmulhuw = 0xe4 | SimdOp::prefix66,
    pmulhw = 0xe5 | SimdOp::prefix66,
    pmulld = 0x40 | SimdOp::opcode38 | SimdOp::prefix66,
    pmullw = 0xd5 | SimdOp::prefix66,
    pmuludq = 0xf4 | SimdOp::prefix66,
    por = 0xeb | SimdOp::prefix66,
    pshufb = 0x00 | SimdOp::opcode38 | SimdOp::prefix66,
    pshufd = 0x70 | SimdOp::prefix66,
    psll_i_arg = 6,
    psllw = 0xf1 | SimdOp::prefix66,
    psllw_i = 0x71 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    pslld = 0xf2 | SimdOp::prefix66,
    pslld_i = 0x72 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    psllq = 0xf3 | SimdOp::prefix66,
    psllq_i = 0x73 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    psignb = 0x08 | SimdOp::opcode38 | SimdOp::prefix66,
    psignd = 0x0a | SimdOp::opcode38 | SimdOp::prefix66,
    psignw = 0x09 | SimdOp::opcode38 | SimdOp::prefix66,
    psra_i_arg = 4,
    psrad = 0xe2 | SimdOp::prefix66,
    psrad_i = 0x72 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    psraw = 0xe1 | SimdOp::prefix66,
    psraw_i = 0x71 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    psrl_i_arg = 2,
    psrld = 0xd2 | SimdOp::prefix66,
    psrld_i = 0x72 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    psrlq = 0xd3 | SimdOp::prefix66,
    psrlq_i = 0x73 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    psrlw = 0xd1 | SimdOp::prefix66,
    psrlw_i = 0x71 | SimdOp::opcodeHasArg | SimdOp::prefix66,
    psubb = 0xf8 | SimdOp::prefix66,
    psubd = 0xfa | SimdOp::prefix66,
    psubq = 0xfb | SimdOp::prefix66,
    psubsb = 0xe8 | SimdOp::prefix66,
    psubsw = 0xe9 | SimdOp::prefix66,
    psubusb = 0xd8 | SimdOp::prefix66,
    psubusw = 0xd9 | SimdOp::prefix66,
    psubw = 0xf9 | SimdOp::prefix66,
    ptest = 0x17 | SimdOp::opcode38 | prefix66,
    punpckhbw = 0x68 | SimdOp::prefix66,
    punpckhwd = 0x69 | SimdOp::prefix66,
    punpcklbw = 0x60 | SimdOp::prefix66,
    punpcklwd = 0x61 | SimdOp::prefix66,
    pxor = 0xef | SimdOp::prefix66,
    roundps = 0x08 | SimdOp::opcode3A | SimdOp::prefix66,
    roundpd = 0x09 | SimdOp::opcode3A | SimdOp::prefix66,
    shufps = 0xc6,
    sqrtpd = 0x51 | SimdOp::prefix66,
    sqrtps = 0x51,
    subpd = 0x5c | SimdOp::prefix66,
    subps = 0x5c,
    xorpd = 0x57 | SimdOp::prefix66,
    xorps = 0x57,
};

enum Helpers : uint32_t {
    is64 = 1 << 0,
    isInvert = 1 << 0,
    isHigh = 1 << 0,
    isMin = 1 << 1,
    isGreater = 1 << 1,
    isSigned = 1 << 1,
};

}; // namespace SimdOp

static void simdEmitSSEOp(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    uint8_t buf[16];
    uint8_t* ptr = buf;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
    uint8_t rex = 0;
#endif /* SLJIT_CONFIG_X86_64 */

    if (!(opcode & SimdOp::opcodeHasArg)) {
        rd = sljit_get_register_index(SLJIT_SIMD_REG_128, rd);
    }
    rn = sljit_get_register_index(SLJIT_SIMD_REG_128, rn);

    switch (opcode & SimdOp::prefixMask) {
    case SimdOp::prefix66:
        *ptr++ = 0x66;
        break;
    case SimdOp::prefixF2:
        *ptr++ = 0xf2;
        break;
    case SimdOp::prefixF3:
        *ptr++ = 0xf3;
        break;
    }

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
    if (rd >= 8) {
        rex |= 0x44;
        rd -= 8;
    }

    if (rn >= 8) {
        rex |= 0x41;
        rn -= 8;
    }

    if (rex != 0) {
        *ptr++ = rex;
    }
#endif /* SLJIT_CONFIG_X86_64 */

    *ptr++ = 0x0f;

    if (opcode & SimdOp::opcode38) {
        *ptr++ = 0x38;
    } else if (opcode & SimdOp::opcode3A) {
        *ptr++ = 0x3a;
    }

    ptr[0] = static_cast<uint8_t>(opcode & 0xff);
    ptr[1] = static_cast<uint8_t>(0xc0 | (rd << 3) | rn);
    ptr += 2;

    if (opcode & SimdOp::opcodeHasImm) {
        *ptr++ = static_cast<uint8_t>((opcode >> 8) & 0xff);
    }

    sljit_emit_op_custom(compiler, buf, static_cast<sljit_u32>(ptr - buf));
}

static void simdEmitVexOp(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    uint8_t buf[16];
    uint8_t* ptr = buf;
    uint8_t vex = 0;

    ASSERT(sljit_has_cpu_feature(SLJIT_HAS_AVX) == 1);

    if (!(opcode & SimdOp::opcodeHasArg)) {
        rd = sljit_get_register_index(SLJIT_SIMD_REG_128, rd);
    }
    rm = sljit_get_register_index(SLJIT_SIMD_REG_128, rm);

    if (opcode & SimdOp::opcode38) {
        vex = 0x2;
    } else if (opcode & SimdOp::opcode3A) {
        vex = 0x3;
    } else if (opcode & (SimdOp::opcodeW0 | SimdOp::opcodeW1)) {
        vex = 0x1;
    }

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
    if (rm >= 8) {
        if (vex == 0) {
            vex = 0x1;
        }

        vex |= 0x20;
        rm -= 8;
    }

    if (rd >= 8 && vex != 0) {
        vex |= 0x80;
        rd -= 8;
    }
#endif /* SLJIT_CONFIG_X86_64 */

    if (vex == 0) {
        *ptr++ = 0xc5;

        vex = 0x80;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
        if (rd >= 8) {
            vex = 0;
            rd -= 8;
        }
#endif /* SLJIT_CONFIG_X86_64 */
    } else {
        ptr[0] = 0xc4;
        ptr[1] = static_cast<uint8_t>(vex ^ 0xe0);
        ptr += 2;

        vex = 0;

        if (opcode & SimdOp::opcodeW1) {
            vex = 0x80;
        }
    }

    rn = (rn < 0) ? 0 : sljit_get_register_index(SLJIT_SIMD_REG_128, rn);
    vex |= static_cast<uint8_t>((rn ^ 0xf) << 3);

    switch (opcode & SimdOp::prefixMask) {
    case SimdOp::prefix66:
        vex |= 0x1;
        break;
    case SimdOp::prefixF2:
        vex |= 0x3;
        break;
    case SimdOp::prefixF3:
        vex |= 0x2;
        break;
    }

    if (opcode & SimdOp::reg256) {
        vex |= 0x40;
    }

    ptr[0] = vex;
    ptr[1] = static_cast<uint8_t>(opcode & 0xff);
    ptr[2] = static_cast<uint8_t>(0xc0 | (rd << 3) | rm);
    ptr += 3;

    if (opcode & SimdOp::opcodeHasImm) {
        *ptr++ = static_cast<uint8_t>((opcode >> 8) & 0xff);
    }

    sljit_emit_op_custom(compiler, buf, static_cast<sljit_u32>(ptr - buf));
}

static void simdEmitOp(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    ASSERT(rd != rm && !(opcode & SimdOp::opcodeHasArg));

    if (rd == rn) {
        return simdEmitSSEOp(compiler, opcode, rd, rm);
    }

    return simdEmitVexOp(compiler, opcode, rd, rn, rm);
}

static void simdEmitOpArg(sljit_compiler* compiler, uint32_t opcode, sljit_s32 arg, sljit_s32 rd, sljit_s32 rn)
{
    ASSERT(opcode & SimdOp::opcodeHasArg);

    if (rd == rn) {
        return simdEmitSSEOp(compiler, opcode, arg, rd);
    }

    return simdEmitVexOp(compiler, opcode, arg, rd, rn);
}

static void simdEmitINot(sljit_compiler* compiler, uint32_t operation, sljit_s32 rd, sljit_s32 tmp)
{
    simdEmitSSEOp(compiler, operation, tmp, tmp);
    simdEmitSSEOp(compiler, SimdOp::pxor, rd, tmp);
}

static void simdEmitINeg(sljit_compiler* compiler, uint32_t signOpcode, uint32_t subOpcode, sljit_s32 rd, sljit_s32 rn)
{
    if (rd == rn) {
        sljit_s32 tmp = SLJIT_FR1;

        if (signOpcode != 0) {
            simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp, tmp);
            simdEmitSSEOp(compiler, signOpcode, rd, tmp);
            return;
        }

        ASSERT(subOpcode == SimdOp::psubq);

        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitSSEOp(compiler, SimdOp::pxor, tmp, tmp);
            simdEmitVexOp(compiler, SimdOp::psubq, rd, tmp, rn);
            return;
        }

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, tmp, rn, 0);
        rn = tmp;
    }

    simdEmitSSEOp(compiler, SimdOp::pxor, rd, rd);
    simdEmitSSEOp(compiler, subOpcode, rd, rn);
}

static void simdEmitUnaryImm(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_FR1;

    if (rd != rn) {
        tmp = rd;
    }

    simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp, tmp);

    uint32_t inst = (opcode == SimdOp::xorps) ? OPCODE_AND_IMM(SimdOp::pslld_i, 31) : OPCODE_AND_IMM(SimdOp::psllq_i, 63);
    simdEmitSSEOp(compiler, inst, SimdOp::psll_i_arg, tmp);

    simdEmitSSEOp(compiler, opcode, rd, tmp);
}

static void simdEmitAllTrue(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_FR1;

    simdEmitSSEOp(compiler, SimdOp::pxor, tmp, tmp);
    simdEmitSSEOp(compiler, opcode, tmp, rn);
    simdEmitSSEOp(compiler, SimdOp::ptest, tmp, tmp);
    sljit_set_current_flags(compiler, SLJIT_SET_Z);
    sljit_emit_op_flags(compiler, SLJIT_MOV32, rd, 0, SLJIT_ZERO);
}

static void simdEmitExtAdd(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_FR2;

    ASSERT(opcode == SimdOp::pmaddubsw || opcode == SimdOp::pmaddwd);

    if (opcode == SimdOp::pmaddubsw) {
        sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, tmp, SLJIT_IMM, 1);
    } else {
        simdEmitSSEOp(compiler, SimdOp::pcmpeqw, tmp, tmp);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psrlw_i, 15), SimdOp::psrl_i_arg, tmp);
    }

    if (rd != rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, opcode, rd, rn, tmp);
            return;
        }

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, rd, rn, 0);
    }

    simdEmitSSEOp(compiler, opcode, rd, tmp);
}

static void simdEmit16X8ExtAddS(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_FR2;

    if (rd == rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, tmp, SLJIT_IMM, 1);
            simdEmitVexOp(compiler, SimdOp::pmaddubsw, rd, tmp, rn);
            return;
        }

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, tmp, rn, 0);
        rn = tmp;
    }

    sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, rd, SLJIT_IMM, 1);
    simdEmitSSEOp(compiler, SimdOp::pmaddubsw, rd, rn);
}

static void simdEmit32X4ExtAddU(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_FR2;

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, OPCODE_AND_IMM(SimdOp::psrld_i, 16), SimdOp::psrl_i_arg, tmp, rn);
    } else {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16, tmp, rn, 0);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psrld_i, 16), SimdOp::psrl_i_arg, tmp);
    }

    if (rd != rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, OPCODE_AND_IMM(SimdOp::pblendw, 0xaa), rd, rn, tmp);
            simdEmitSSEOp(compiler, SimdOp::paddd, rd, tmp);
            return;
        }

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16, rd, rn, 0);
    }

    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pblendw, 0xaa), rd, tmp);
    simdEmitSSEOp(compiler, SimdOp::paddd, rd, tmp);
}

static void simdEmitConvertI32X4U(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, bool toSingle)
{
    sljit_s32 tmp1 = SLJIT_FR1;
    sljit_s32 tmp2 = SLJIT_FR2;

    // Special constant: 2^53
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO, tmp1, 0, SLJIT_IMM, 0x43300000);

    if (toSingle) {
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0x32), tmp2, rn);
    }

    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0x10), rd, rn);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0x11), tmp1, tmp1);

    if (toSingle) {
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::blendps, 0xa), tmp2, tmp1);
    }

    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::blendps, 0xa), rd, tmp1);

    if (toSingle) {
        simdEmitSSEOp(compiler, SimdOp::subpd, tmp2, tmp1);
    }

    simdEmitSSEOp(compiler, SimdOp::subpd, rd, tmp1);

    if (toSingle) {
        simdEmitSSEOp(compiler, SimdOp::cvtpd2ps, tmp2, tmp2);
        simdEmitSSEOp(compiler, SimdOp::cvtpd2ps, rd, rd);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::shufps, 0x44), rd, tmp2);
    }
}

static void simdEmitTruncSatS(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, bool is32)
{
    sljit_s32 tmp = SLJIT_FR2;

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, is32 ? SimdOp::cmpeqps : SimdOp::cmpeqpd, tmp, rn, rn);
        simdEmitVexOp(compiler, is32 ? SimdOp::andps : SimdOp::andpd, rd, rn, tmp);
    } else {
        sljit_s32 type = (is32 ? SLJIT_SIMD_ELEM_32 : SLJIT_SIMD_ELEM_64) | SLJIT_SIMD_FLOAT;

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | type, tmp, rn, 0);
        simdEmitSSEOp(compiler, is32 ? SimdOp::cmpeqps : SimdOp::cmpeqpd, tmp, tmp);

        if (rd != rn) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | type, rd, rn, 0);
        }

        simdEmitSSEOp(compiler, is32 ? SimdOp::andps : SimdOp::andpd, rd, tmp);
    }

    if (!is32) {
        sljit_emit_fset64(compiler, tmp, 2147483647.0);
        sljit_emit_simd_lane_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT, tmp, tmp, 0);
        simdEmitSSEOp(compiler, SimdOp::minpd, rd, tmp);
        simdEmitSSEOp(compiler, SimdOp::cvttpd2dq, rd, rd);
        return;
    }

    sljit_emit_fset32(compiler, tmp, 2147483520.0f);
    sljit_emit_simd_lane_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT, tmp, tmp, 0);
    simdEmitSSEOp(compiler, SimdOp::cmpltps, tmp, rd);
    simdEmitSSEOp(compiler, SimdOp::cvttps2dq, rd, rd);
    simdEmitSSEOp(compiler, SimdOp::xorps, rd, tmp);
}

static void simdEmitTruncSatF64x2U(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp = SLJIT_FR2;

    simdEmitSSEOp(compiler, SimdOp::xorpd, tmp, tmp);

    if (rd != rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, SimdOp::maxpd, rd, rn, tmp);
        } else {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT, rd, rn, 0);
            rn = rd;
        }
    }

    if (rd == rn) {
        simdEmitSSEOp(compiler, SimdOp::maxpd, rd, tmp);
    }

    sljit_emit_fset64(compiler, tmp, 4294967295.0);
    sljit_emit_simd_lane_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT, tmp, tmp, 0);
    simdEmitSSEOp(compiler, SimdOp::minpd, rd, tmp);

    // Round to zero.
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundpd, 3), rd, rd);

    // Special constant: 2^53
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO, tmp, 0, SLJIT_IMM, 0x43300000);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0x11), tmp, tmp);

    simdEmitSSEOp(compiler, SimdOp::addpd, rd, tmp);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::shufps, 0x08), rd, tmp);
}

static void simdEmitTruncSatF32x4U(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmp1 = SLJIT_FR2;
    sljit_s32 tmp2 = SLJIT_FR3;

    simdEmitSSEOp(compiler, SimdOp::xorps, tmp1, tmp1);
    // Floating point representation of 0x80000000
    sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32, tmp2, SLJIT_IMM, 0x4f000000);

    // Negative and NaN are converted to zero.
    if (rd != rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, SimdOp::maxpd, rd, rn, tmp1);
        } else {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT, rd, rn, 0);
            rn = rd;
        }
    }

    if (rd == rn) {
        simdEmitSSEOp(compiler, SimdOp::maxps, rd, tmp1);
    }

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, SimdOp::subps, tmp1, rd, tmp2);
    } else {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT, tmp1, rd, 0);
        simdEmitSSEOp(compiler, SimdOp::subps, tmp1, tmp2);
    }

    // Values >= 0x80000000 are converted to 0x7fffffff.
    simdEmitSSEOp(compiler, SimdOp::cmpleps, tmp2, tmp1);
    simdEmitSSEOp(compiler, SimdOp::cvttps2dq, tmp1, tmp1);
    simdEmitSSEOp(compiler, SimdOp::xorps, tmp1, tmp2);

    // Values < 0 are droppped.
    simdEmitSSEOp(compiler, SimdOp::xorps, tmp2, tmp2);
    simdEmitSSEOp(compiler, SimdOp::pmaxsd, tmp1, tmp2);

    // All values above int_max are converted to 0x80000000.
    simdEmitSSEOp(compiler, SimdOp::cvttps2dq, rd, rd);
    // The tmp1 has values between 0 and 0x7fffffff.
    simdEmitSSEOp(compiler, SimdOp::paddd, rd, tmp1);
}

static void simdEmitAbs(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, bool is64)
{
    sljit_s32 tmp = SLJIT_FR1;
    simdEmitSSEOp(compiler, (is64 ? SimdOp::pcmpeqq : SimdOp::pcmpeqd), tmp, tmp);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM((is64 ? SimdOp::psrlq_i : SimdOp::psrld_i), 1), SimdOp::psrl_i_arg, tmp);
    if (rd != rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, (is64 ? SimdOp::andpd : SimdOp::andps), rd, rn, tmp);
            return;
        }
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, rd, rn, 0);
    }
    simdEmitSSEOp(compiler, (is64 ? SimdOp::andpd : SimdOp::andps), rd, tmp);
}

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = -1;
        break;
    case ByteCode::I8X16NegOpcode:
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
    case ByteCode::I16X8AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = -1;
        break;
    case ByteCode::I16X8NegOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = -1;
        break;
    case ByteCode::I32X4NegOpcode:
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
    case ByteCode::I64X2AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = -1;
        break;
    case ByteCode::I64X2NegOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
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
    case ByteCode::V128AnyTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_128;
        dstType = -1;
        break;
    case ByteCode::V128NotOpcode:
        srcType = SLJIT_SIMD_ELEM_128;
        dstType = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, SLJIT_FR0);

    args[1].set(operands + 1);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, dstType == 0 ? SLJIT_R0 : SLJIT_FR0);

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
        simdEmitAllTrue(compiler, SimdOp::pcmpeqb, dst, args[0].arg);
        break;
    case ByteCode::I8X16NegOpcode:
        simdEmitINeg(compiler, SimdOp::psignb, SimdOp::psubb, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
        simdEmitSSEOp(compiler, SimdOp::pmovsxbw, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0xe), dst, args[0].arg);
        simdEmitSSEOp(compiler, SimdOp::pmovsxbw, dst, dst);
        break;
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
        simdEmitSSEOp(compiler, SimdOp::pmovzxbw, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0xe), dst, args[0].arg);
        simdEmitSSEOp(compiler, SimdOp::pmovzxbw, dst, dst);
        break;
    case ByteCode::I16X8AllTrueOpcode:
        simdEmitAllTrue(compiler, SimdOp::pcmpeqw, dst, args[0].arg);
        break;
    case ByteCode::I16X8NegOpcode:
        simdEmitINeg(compiler, SimdOp::psignw, SimdOp::psubw, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
        simdEmit16X8ExtAddS(compiler, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        simdEmitExtAdd(compiler, SimdOp::pmaddubsw, dst, args[0].arg);
        break;
    case ByteCode::I32X4AllTrueOpcode:
        simdEmitAllTrue(compiler, SimdOp::pcmpeqd, dst, args[0].arg);
        break;
    case ByteCode::I32X4NegOpcode:
        simdEmitINeg(compiler, SimdOp::psignd, SimdOp::psubd, dst, args[0].arg);
        break;
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
        simdEmitExtAdd(compiler, SimdOp::pmaddwd, dst, args[0].arg);
        break;
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        simdEmit32X4ExtAddU(compiler, dst, args[0].arg);
        break;
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
        simdEmitSSEOp(compiler, SimdOp::pmovsxwd, dst, args[0].arg);
        break;
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0xe), dst, args[0].arg);
        simdEmitSSEOp(compiler, SimdOp::pmovsxwd, dst, dst);
        break;
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
        simdEmitSSEOp(compiler, SimdOp::pmovzxwd, dst, args[0].arg);
        break;
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0xe), dst, args[0].arg);
        simdEmitSSEOp(compiler, SimdOp::pmovzxwd, dst, dst);
        break;
    case ByteCode::F32X4AbsOpcode:
        simdEmitAbs(compiler, dst, args[0].arg, false);
        break;
    case ByteCode::F32X4NegOpcode:
        simdEmitUnaryImm(compiler, SimdOp::xorps, dst, args[0].arg);
        break;
    case ByteCode::F32X4CeilOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundps, 0b0010), dst, args[0].arg);
        break;
    case ByteCode::F32X4FloorOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundps, 0b0001), dst, args[0].arg);
        break;
    case ByteCode::F32X4TruncOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundps, 0b0000), dst, args[0].arg);
        break;
    case ByteCode::F32X4NearestOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundps, 0b0011), dst, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF32X4SOpcode:
        simdEmitTruncSatS(compiler, dst, args[0].arg, true);
        break;
    case ByteCode::I32X4TruncSatF32X4UOpcode:
        simdEmitTruncSatF32x4U(compiler, dst, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
        simdEmitTruncSatS(compiler, dst, args[0].arg, false);
        break;
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        simdEmitTruncSatF64x2U(compiler, dst, args[0].arg);
        break;
    case ByteCode::I64X2AllTrueOpcode:
        simdEmitAllTrue(compiler, SimdOp::pcmpeqq, dst, args[0].arg);
        break;
    case ByteCode::I64X2NegOpcode:
        simdEmitINeg(compiler, 0, SimdOp::psubq, dst, args[0].arg);
        break;
    case ByteCode::F32X4SqrtOpcode:
        simdEmitSSEOp(compiler, SimdOp::sqrtps, dst, args[0].arg);
        break;
    case ByteCode::F32X4ConvertI32X4SOpcode:
        simdEmitSSEOp(compiler, SimdOp::cvtdq2ps, dst, args[0].arg);
        break;
    case ByteCode::F32X4ConvertI32X4UOpcode:
        simdEmitConvertI32X4U(compiler, dst, args[0].arg, true);
        break;
    case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        simdEmitSSEOp(compiler, SimdOp::cvtpd2ps, dst, args[0].arg);
        break;
    case ByteCode::F64X2AbsOpcode:
        simdEmitAbs(compiler, dst, args[0].arg, true);
        break;
    case ByteCode::F64X2NegOpcode:
        simdEmitUnaryImm(compiler, SimdOp::xorpd, dst, args[0].arg);
        break;
    case ByteCode::F64X2CeilOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundpd, 0b0010), dst, args[0].arg);
        break;
    case ByteCode::F64X2FloorOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundpd, 0b0001), dst, args[0].arg);
        break;
    case ByteCode::F64X2TruncOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundpd, 0b0000), dst, args[0].arg);
        break;
    case ByteCode::F64X2NearestOpcode:
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::roundpd, 0b0011), dst, args[0].arg);
        break;
    case ByteCode::F64X2SqrtOpcode:
        simdEmitSSEOp(compiler, SimdOp::sqrtpd, dst, args[0].arg);
        break;
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
        simdEmitSSEOp(compiler, SimdOp::cvtdq2pd, dst, args[0].arg);
        break;
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        simdEmitConvertI32X4U(compiler, dst, args[0].arg, false);
        break;
    case ByteCode::F64X2PromoteLowF32X4Opcode:
        simdEmitSSEOp(compiler, SimdOp::cvtps2pd, dst, args[0].arg);
        break;
    case ByteCode::V128AnyTrueOpcode:
        simdEmitSSEOp(compiler, SimdOp::ptest, args[0].arg, args[0].arg);
        sljit_set_current_flags(compiler, SLJIT_SET_Z);
        sljit_emit_op_flags(compiler, SLJIT_MOV32, dst, 0, SLJIT_NOT_ZERO);
        break;
    case ByteCode::V128NotOpcode:
        if (dst == args[0].arg) {
            sljit_s32 tmp = SLJIT_FR1;
            simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp, tmp);
            simdEmitSSEOp(compiler, SimdOp::pxor, dst, tmp);
        } else {
            simdEmitSSEOp(compiler, SimdOp::pcmpeqd, dst, dst);
            simdEmitSSEOp(compiler, SimdOp::pxor, dst, args[0].arg);
        }
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[1].arg)) {
        if (dstType != -1) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[1].arg, args[1].argw);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, dst, 0);
        }
    }
}

static void simdEmitPMinMax(sljit_compiler* compiler, uint32_t operation, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp = SLJIT_FR2;
    bool is64 = (operation & SimdOp::is64) != 0;
    bool isMin = (operation & SimdOp::isMin) != 0;

    if (!sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        sljit_s32 type = SLJIT_SIMD_REG_128 | (is64 ? SLJIT_SIMD_ELEM_64 : SLJIT_SIMD_ELEM_32);

        sljit_emit_simd_mov(compiler, type, tmp, isMin ? rm : rn, 0);

        if (rd != rn) {
            sljit_emit_simd_mov(compiler, type, rd, rn, 0);
        }

        simdEmitSSEOp(compiler, is64 ? SimdOp::cmpnltpd : SimdOp::cmpnltps, tmp, isMin ? rn : rm);
        simdEmitSSEOp(compiler, is64 ? SimdOp::andpd : SimdOp::andps, rd, tmp);
    } else {
        simdEmitVexOp(compiler, is64 ? SimdOp::cmpnltpd : SimdOp::cmpnltps, tmp, isMin ? rm : rn, isMin ? rn : rm);

        if (rd == rn) {
            simdEmitSSEOp(compiler, is64 ? SimdOp::andpd : SimdOp::andps, rd, tmp);
        } else {
            simdEmitVexOp(compiler, is64 ? SimdOp::andpd : SimdOp::andps, rd, rn, tmp);
        }
    }

    simdEmitSSEOp(compiler, is64 ? SimdOp::andnpd : SimdOp::andnps, tmp, rm);
    simdEmitSSEOp(compiler, is64 ? SimdOp::orpd : SimdOp::orps, rd, tmp);
}

static void simdEmitFloatMaxMinHelper(sljit_compiler* compiler, SimdOp::Type opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, sljit_s32 tmp)
{
    if (rd == rn || rd == rm) {
        sljit_s32 src = (rd == rn) ? rm : rn;
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, opcode, tmp, src, rd);
            simdEmitVexOp(compiler, opcode, rd, rd, src);
        } else {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, tmp, src, 0);
            simdEmitSSEOp(compiler, opcode, tmp, rd);
            simdEmitSSEOp(compiler, opcode, rd, src);
        }
    } else {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, opcode, tmp, rn, rm);
            simdEmitVexOp(compiler, opcode, rd, rm, rn);
        } else {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, tmp, rn, 0);
            simdEmitSSEOp(compiler, opcode, tmp, rm);
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, rd, rm, 0);
            simdEmitSSEOp(compiler, opcode, rd, rn);
        }
    }
}

static void simdEmitFloatMax(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, bool is64)
{
    sljit_s32 tmp = SLJIT_FR2;
    simdEmitFloatMaxMinHelper(compiler, (is64 ? SimdOp::maxpd : SimdOp::maxps), rd, rn, rm, tmp);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::xorpd : SimdOp::xorps), rd, tmp);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::orpd : SimdOp::orps), tmp, rd);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::subpd : SimdOp::subps), tmp, rd);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::cmpunordpd : SimdOp::cmpunordps), rd, tmp);
    simdEmitSSEOp(compiler, (is64 ? OPCODE_AND_IMM(SimdOp::psrlq_i, 13) : OPCODE_AND_IMM(SimdOp::psrld_i, 10)), SimdOp::psrl_i_arg, rd);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::andnpd : SimdOp::andnps), rd, tmp);
}

static void simdEmitFloatMin(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, bool is64)
{
    sljit_s32 tmp = SLJIT_FR2;
    simdEmitFloatMaxMinHelper(compiler, (is64 ? SimdOp::minpd : SimdOp::minps), rd, rn, rm, tmp);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::orpd : SimdOp::orps), tmp, rd);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::cmpunordpd : SimdOp::cmpunordps), rd, tmp);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::orpd : SimdOp::orps), tmp, rd);
    simdEmitSSEOp(compiler, (is64 ? OPCODE_AND_IMM(SimdOp::psrlq_i, 13) : OPCODE_AND_IMM(SimdOp::psrld_i, 10)), SimdOp::psrl_i_arg, rd);
    simdEmitSSEOp(compiler, (is64 ? SimdOp::andnpd : SimdOp::andnps), rd, tmp);
}

static void simdEmitI64X2Mul(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp1 = SLJIT_FR2;
    sljit_s32 tmp2 = SLJIT_FR3;

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, OPCODE_AND_IMM(SimdOp::psrlq_i, 32), SimdOp::psrl_i_arg, tmp1, rn);
        simdEmitVexOp(compiler, OPCODE_AND_IMM(SimdOp::psrlq_i, 32), SimdOp::psrl_i_arg, tmp2, rm);
        simdEmitSSEOp(compiler, SimdOp::pmuludq, tmp1, rm);
        simdEmitSSEOp(compiler, SimdOp::pmuludq, tmp2, rn);

        simdEmitSSEOp(compiler, SimdOp::paddq, tmp2, tmp1);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psllq_i, 32), SimdOp::psll_i_arg, tmp2);

        if (rd == rn || rd == rm) {
            simdEmitSSEOp(compiler, SimdOp::pmuludq, rd, rd == rn ? rm : rn);
        } else {
            simdEmitVexOp(compiler, SimdOp::pmuludq, rd, rn, rm);
        }

        simdEmitSSEOp(compiler, SimdOp::paddq, rd, tmp2);
        return;
    }

    sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, tmp1, rn, 0);
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, tmp2, rm, 0);

    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psrlq_i, 32), SimdOp::psrl_i_arg, tmp1);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psrlq_i, 32), SimdOp::psrl_i_arg, tmp2);
    simdEmitSSEOp(compiler, SimdOp::pmuludq, tmp1, rm);
    simdEmitSSEOp(compiler, SimdOp::pmuludq, tmp2, rn);

    simdEmitSSEOp(compiler, SimdOp::paddq, tmp2, tmp1);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psllq_i, 32), SimdOp::psll_i_arg, tmp2);

    if (rd != rn) {
        if (rd == rm) {
            rm = rn;
        } else {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, rd, rn, 0);
        }
    }

    simdEmitSSEOp(compiler, SimdOp::pmuludq, rd, rm);
    simdEmitSSEOp(compiler, SimdOp::paddq, rd, tmp2);
}

static void simdEmitCmp(sljit_compiler* compiler, uint32_t leftOperation, uint32_t rightOperation, uint32_t eqOperation, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    if (rd != rn) {
        if (rd == rm) {
            simdEmitSSEOp(compiler, rightOperation, rd, rn);
            simdEmitSSEOp(compiler, eqOperation, rd, rn);
            return;
        }

        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, leftOperation, rd, rn, rm);
            simdEmitSSEOp(compiler, eqOperation, rd, rm);
            return;
        }

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, rd, rn, 0);
    }

    simdEmitSSEOp(compiler, leftOperation, rd, rm);
    simdEmitSSEOp(compiler, eqOperation, rd, rm);
}

static void simdEmitOpI64X2Cmp(sljit_compiler* compiler, uint32_t operation, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    bool isGreater = (operation & SimdOp::isGreater) != 0;
    bool isInvert = (operation & SimdOp::isInvert) != 0;
    sljit_s32 tmp1 = SLJIT_FR2;
    sljit_s32 tmp2 = SLJIT_FR3;

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        if (!isGreater) {
            simdEmitVexOp(compiler, SimdOp::pcmpeqq, tmp1, rn, rm);
        }

        if (rd == rn) {
            simdEmitSSEOp(compiler, SimdOp::pcmpgtq, rd, rm);
        } else {
            simdEmitVexOp(compiler, SimdOp::pcmpgtq, rd, rn, rm);
        }

        if (!isGreater) {
            simdEmitSSEOp(compiler, SimdOp::por, rd, tmp1);
        }

        if (isInvert) {
            simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp1, tmp1);
            simdEmitSSEOp(compiler, SimdOp::pxor, rd, tmp1);
        }
        return;
    }

    sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64, tmp1, isGreater ? rm : rn, 0);
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64, tmp2, rn, 0);
    // Sets the upper 32 bit to all 0 or 1, if the upper32 bit is the same.
    simdEmitSSEOp(compiler, SimdOp::psubq, tmp1, isGreater ? rm : rn);

    simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp2, rm);

    if (rd != rn) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64, rd, rn, 0);
    }

    simdEmitSSEOp(compiler, isGreater ? SimdOp::pand : SimdOp::pandn, tmp1, tmp2);
    if (isInvert) {
        simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp2, tmp2);
    }

    simdEmitSSEOp(compiler, SimdOp::pcmpgtd, rd, rm);
    simdEmitSSEOp(compiler, SimdOp::por, rd, tmp1);

    if (isInvert) {
        simdEmitSSEOp(compiler, SimdOp::pxor, rd, tmp2);
    }

    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0xf5), rd, rd);
}

static void simdEmitSwizzle(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp = SLJIT_FR2;

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, tmp, SLJIT_IMM, 15);
        // All negative numbers have their highest bit set.
        simdEmitVexOp(compiler, SimdOp::pcmpgtb, tmp, rm, tmp);
    } else {
        sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, tmp, SLJIT_IMM, 16);
        simdEmitSSEOp(compiler, SimdOp::pmaxub, tmp, rm);
        simdEmitSSEOp(compiler, SimdOp::pcmpeqb, tmp, rm);
    }
    simdEmitSSEOp(compiler, SimdOp::por, tmp, rm);

    if (rd != rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, SimdOp::pshufb, rd, rn, tmp);
        }

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, rd, rn, 0);
    }

    simdEmitSSEOp(compiler, SimdOp::pshufb, rd, tmp);
}

static void simdEmitI16X8Extmul(sljit_compiler* compiler, uint32_t operation, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    bool isSigned = (operation & SimdOp::isSigned) != 0;
    sljit_s32 tmp = SLJIT_FR2;

    if (operation & SimdOp::isHigh) {
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0xe), tmp, rm);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, 0xe), rd, rn);
        rm = tmp;
        rn = rd;
    }

    if (isSigned) {
        simdEmitSSEOp(compiler, SimdOp::pmovsxbw, tmp, rm);
        simdEmitSSEOp(compiler, SimdOp::pmovsxbw, rd, rn);
    } else {
        simdEmitSSEOp(compiler, SimdOp::pmovzxbw, tmp, rm);
        simdEmitSSEOp(compiler, SimdOp::pmovzxbw, rd, rn);
    }

    simdEmitSSEOp(compiler, SimdOp::pmullw, rd, tmp);
}

static void simdEmitI32X4Extmul(sljit_compiler* compiler, uint32_t operation, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    bool isSigned = (operation & SimdOp::isSigned) != 0;
    sljit_s32 tmp = SLJIT_FR2;

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, isSigned ? SimdOp::pmulhw : SimdOp::pmulhuw, tmp, rn, rm);
        simdEmitVexOp(compiler, SimdOp::pmullw, rd, rn, rm);
    } else {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16, tmp, rn, 0);
        simdEmitSSEOp(compiler, isSigned ? SimdOp::pmulhw : SimdOp::pmulhuw, tmp, rm);

        if (rd != rn) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16, rd, rn, 0);
        }

        simdEmitSSEOp(compiler, SimdOp::pmullw, rd, rm);
    }

    simdEmitSSEOp(compiler, operation & SimdOp::isHigh ? SimdOp::punpckhwd : SimdOp::punpcklwd, rd, tmp);
}

static void simdEmitI64X2Extmul(sljit_compiler* compiler, uint32_t operation, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp = SLJIT_FR2;
    uint8_t imm = (operation & SimdOp::isHigh) ? 0xfa : 0x50;

    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, imm), tmp, rm);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::pshufd, imm), rd, rn);
    simdEmitSSEOp(compiler, (operation & SimdOp::isSigned) ? SimdOp::pmuldq : SimdOp::pmuludq, rd, tmp);
}

static void simdEmitQ15mulrSatS(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    sljit_s32 tmp = SLJIT_FR2;

    simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp, tmp);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psllw_i, 15), SimdOp::psll_i_arg, tmp);

    if (rd != rn) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, SimdOp::pmulhrsw, rd, rn, rm);
        } else {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16, rd, rn, 0);
            rn = rd;
        }
    }

    if (rd == rn) {
        simdEmitSSEOp(compiler, SimdOp::pmulhrsw, rd, rm);
    }

    simdEmitSSEOp(compiler, SimdOp::pcmpeqw, tmp, rd);
    simdEmitSSEOp(compiler, SimdOp::pxor, rd, tmp);
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];
    int reverseArgs = 0;

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
    case ByteCode::I8X16SwizzleOpcode:
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
    case ByteCode::F32X4GtOpcode:
    case ByteCode::F32X4GeOpcode:
        reverseArgs = 1;
        FALLTHROUGH;
    case ByteCode::F32X4AddOpcode:
    case ByteCode::F32X4SubOpcode:
    case ByteCode::F32X4MulOpcode:
    case ByteCode::F32X4DivOpcode:
    case ByteCode::F32X4EqOpcode:
    case ByteCode::F32X4NeOpcode:
    case ByteCode::F32X4LtOpcode:
    case ByteCode::F32X4LeOpcode:
    case ByteCode::F32X4PMinOpcode:
    case ByteCode::F32X4PMaxOpcode:
    case ByteCode::F32X4MaxOpcode:
    case ByteCode::F32X4MinOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2GtOpcode:
    case ByteCode::F64X2GeOpcode:
        reverseArgs = 1;
        FALLTHROUGH;
    case ByteCode::F64X2AddOpcode:
    case ByteCode::F64X2SubOpcode:
    case ByteCode::F64X2MulOpcode:
    case ByteCode::F64X2DivOpcode:
    case ByteCode::F64X2SqrtOpcode:
    case ByteCode::F64X2EqOpcode:
    case ByteCode::F64X2NeOpcode:
    case ByteCode::F64X2LtOpcode:
    case ByteCode::F64X2LeOpcode:
    case ByteCode::F64X2PMinOpcode:
    case ByteCode::F64X2PMaxOpcode:
    case ByteCode::F64X2MaxOpcode:
    case ByteCode::F64X2MinOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::V128AndnotOpcode:
        reverseArgs = 1;
        FALLTHROUGH;
    case ByteCode::V128AndOpcode:
    case ByteCode::V128OrOpcode:
    case ByteCode::V128XorOpcode:
        srcType = SLJIT_SIMD_ELEM_128;
        dstType = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    ASSERT(reverseArgs == 0 || reverseArgs == 1);
    simdOperandToArg(compiler, operands + reverseArgs, args[0], srcType, SLJIT_FR0);
    simdOperandToArg(compiler, operands + (1 - reverseArgs), args[1], srcType, SLJIT_FR1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR0);

    if (!sljit_has_cpu_feature(SLJIT_HAS_AVX) && dst != args[0].arg) {
        sljit_emit_simd_mov(compiler, srcType, dst, args[0].arg, 0);
        args[0].arg = dst;
    }

    switch (instr->opcode()) {
    case ByteCode::I8X16AddOpcode:
        simdEmitOp(compiler, SimdOp::paddb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubOpcode:
        simdEmitOp(compiler, SimdOp::psubb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatSOpcode:
        simdEmitOp(compiler, SimdOp::paddsb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatUOpcode:
        simdEmitOp(compiler, SimdOp::paddusb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatSOpcode:
        simdEmitOp(compiler, SimdOp::psubsb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatUOpcode:
        simdEmitOp(compiler, SimdOp::psubusw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16EqOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NeOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqb, dst, SLJIT_FR2);
        break;
    case ByteCode::I8X16LtSOpcode:
        simdEmitCmp(compiler, SimdOp::pminsb, SimdOp::pmaxsb, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqb, dst, SLJIT_FR2);
        break;
    case ByteCode::I8X16LtUOpcode:
        simdEmitCmp(compiler, SimdOp::pminub, SimdOp::pmaxub, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqb, dst, SLJIT_FR2);
        break;
    case ByteCode::I8X16LeSOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxsb, SimdOp::pminsb, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16LeUOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxub, SimdOp::pminub, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GtSOpcode:
        simdEmitOp(compiler, SimdOp::pcmpgtb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GtUOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxub, SimdOp::pminub, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqb, dst, SLJIT_FR2);
        break;
    case ByteCode::I8X16GeSOpcode:
        simdEmitCmp(compiler, SimdOp::pminsb, SimdOp::pmaxsb, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GeUOpcode:
        simdEmitCmp(compiler, SimdOp::pminub, SimdOp::pmaxub, SimdOp::pcmpeqb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SwizzleOpcode:
        simdEmitSwizzle(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NarrowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::packsswb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NarrowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::packuswb, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AddOpcode:
        simdEmitOp(compiler, SimdOp::paddw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubOpcode:
        simdEmitOp(compiler, SimdOp::psubw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MulOpcode:
        simdEmitOp(compiler, SimdOp::pmullw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AddSatSOpcode:
        simdEmitOp(compiler, SimdOp::paddsw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AddSatUOpcode:
        simdEmitOp(compiler, SimdOp::paddusw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubSatSOpcode:
        simdEmitOp(compiler, SimdOp::psubsw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubSatUOpcode:
        simdEmitOp(compiler, SimdOp::psubusw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8EqOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8NeOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqw, dst, SLJIT_FR2);
        break;
    case ByteCode::I16X8LtSOpcode:
        simdEmitCmp(compiler, SimdOp::pminsw, SimdOp::pmaxsw, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqw, dst, SLJIT_FR2);
        break;
    case ByteCode::I16X8LtUOpcode:
        simdEmitCmp(compiler, SimdOp::pminuw, SimdOp::pmaxuw, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqw, dst, SLJIT_FR2);
        break;
    case ByteCode::I16X8LeSOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxsw, SimdOp::pminsw, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8LeUOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxuw, SimdOp::pminuw, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GtSOpcode:
        simdEmitOp(compiler, SimdOp::pcmpgtw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GtUOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxuw, SimdOp::pminuw, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqw, dst, SLJIT_FR2);
        break;
    case ByteCode::I16X8GeSOpcode:
        simdEmitCmp(compiler, SimdOp::pminsw, SimdOp::pmaxsw, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GeUOpcode:
        simdEmitCmp(compiler, SimdOp::pminuw, SimdOp::pmaxuw, SimdOp::pcmpeqw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8Q15mulrSatSOpcode:
        simdEmitQ15mulrSatS(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulLowI8X16SOpcode:
        simdEmitI16X8Extmul(compiler, SimdOp::isSigned, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16SOpcode:
        simdEmitI16X8Extmul(compiler, SimdOp::isHigh | SimdOp::isSigned, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
        simdEmitI16X8Extmul(compiler, 0, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        simdEmitI16X8Extmul(compiler, SimdOp::isHigh, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8NarrowI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::packssdw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8NarrowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::packusdw, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4AddOpcode:
        simdEmitOp(compiler, SimdOp::paddd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4SubOpcode:
        simdEmitOp(compiler, SimdOp::psubd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MulOpcode:
        simdEmitOp(compiler, SimdOp::pmulld, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4EqOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4NeOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqd, dst, SLJIT_FR2);
        break;
    case ByteCode::I32X4LtSOpcode:
        simdEmitCmp(compiler, SimdOp::pminsd, SimdOp::pmaxsd, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqd, dst, SLJIT_FR2);
        break;
    case ByteCode::I32X4LtUOpcode:
        simdEmitCmp(compiler, SimdOp::pminud, SimdOp::pmaxud, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqd, dst, SLJIT_FR2);
        break;
    case ByteCode::I32X4LeSOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxsd, SimdOp::pminsd, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4LeUOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxud, SimdOp::pminud, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GtSOpcode:
        simdEmitOp(compiler, SimdOp::pcmpgtd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GtUOpcode:
        simdEmitCmp(compiler, SimdOp::pmaxud, SimdOp::pminud, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqd, dst, SLJIT_FR2);
        break;
    case ByteCode::I32X4GeSOpcode:
        simdEmitCmp(compiler, SimdOp::pminsd, SimdOp::pmaxsd, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GeUOpcode:
        simdEmitCmp(compiler, SimdOp::pminud, SimdOp::pmaxud, SimdOp::pcmpeqd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
        simdEmitI32X4Extmul(compiler, SimdOp::isSigned, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
        simdEmitI32X4Extmul(compiler, SimdOp::isHigh | SimdOp::isSigned, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
        simdEmitI32X4Extmul(compiler, 0, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
        simdEmitI32X4Extmul(compiler, SimdOp::isHigh, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4DotI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::pmaddwd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2AddOpcode:
        simdEmitOp(compiler, SimdOp::paddq, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2SubOpcode:
        simdEmitOp(compiler, SimdOp::psubq, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2MulOpcode:
        simdEmitI64X2Mul(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2EqOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqq, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2NeOpcode:
        simdEmitOp(compiler, SimdOp::pcmpeqq, dst, args[0].arg, args[1].arg);
        simdEmitINot(compiler, SimdOp::pcmpeqq, dst, SLJIT_FR2);
        break;
    case ByteCode::I64X2LtSOpcode:
        simdEmitOpI64X2Cmp(compiler, SimdOp::isInvert, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2LeSOpcode:
        simdEmitOpI64X2Cmp(compiler, SimdOp::isInvert | SimdOp::isGreater, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2GtSOpcode:
        simdEmitOpI64X2Cmp(compiler, SimdOp::isGreater, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2GeSOpcode:
        simdEmitOpI64X2Cmp(compiler, 0, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
        simdEmitI64X2Extmul(compiler, SimdOp::isSigned, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
        simdEmitI64X2Extmul(compiler, SimdOp::isHigh | SimdOp::isSigned, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
        simdEmitI64X2Extmul(compiler, 0, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        simdEmitI64X2Extmul(compiler, SimdOp::isHigh, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4AddOpcode:
        simdEmitOp(compiler, SimdOp::addps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4SubOpcode:
        simdEmitOp(compiler, SimdOp::subps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MulOpcode:
        simdEmitOp(compiler, SimdOp::mulps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4DivOpcode:
        simdEmitOp(compiler, SimdOp::divps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4EqOpcode:
        simdEmitOp(compiler, SimdOp::cmpeqps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4NeOpcode:
        simdEmitOp(compiler, SimdOp::cmpneqps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4LtOpcode:
        simdEmitOp(compiler, SimdOp::cmpltps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4LeOpcode:
        simdEmitOp(compiler, SimdOp::cmpleps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4GtOpcode:
        simdEmitOp(compiler, SimdOp::cmpltps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4GeOpcode:
        simdEmitOp(compiler, SimdOp::cmpleps, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4PMinOpcode:
        simdEmitPMinMax(compiler, SimdOp::isMin, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4PMaxOpcode:
        simdEmitPMinMax(compiler, 0, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MaxOpcode: {
        simdEmitFloatMax(compiler, dst, args[0].arg, args[1].arg, false);
        break;
    }
    case ByteCode::F32X4MinOpcode: {
        simdEmitFloatMin(compiler, dst, args[0].arg, args[1].arg, false);
        break;
    }
    case ByteCode::F64X2AddOpcode:
        simdEmitOp(compiler, SimdOp::addpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2SubOpcode:
        simdEmitOp(compiler, SimdOp::subpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MulOpcode:
        simdEmitOp(compiler, SimdOp::mulpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2DivOpcode:
        simdEmitOp(compiler, SimdOp::divpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2SqrtOpcode:
        simdEmitOp(compiler, SimdOp::sqrtpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2EqOpcode:
        simdEmitOp(compiler, SimdOp::cmpeqpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2NeOpcode:
        simdEmitOp(compiler, SimdOp::cmpneqpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2LtOpcode:
        simdEmitOp(compiler, SimdOp::cmpltpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2LeOpcode:
        simdEmitOp(compiler, SimdOp::cmplepd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2GtOpcode:
        simdEmitOp(compiler, SimdOp::cmpltpd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2GeOpcode:
        simdEmitOp(compiler, SimdOp::cmplepd, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2PMinOpcode:
        simdEmitPMinMax(compiler, SimdOp::is64 | SimdOp::isMin, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2PMaxOpcode:
        simdEmitPMinMax(compiler, SimdOp::is64, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MaxOpcode:
        simdEmitFloatMax(compiler, dst, args[0].arg, args[1].arg, true);
        break;
    case ByteCode::F64X2MinOpcode:
        simdEmitFloatMin(compiler, dst, args[0].arg, args[1].arg, true);
        break;
    case ByteCode::V128AndOpcode:
        simdEmitOp(compiler, SimdOp::pand, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128OrOpcode:
        simdEmitOp(compiler, SimdOp::por, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128XorOpcode:
        simdEmitOp(compiler, SimdOp::pxor, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128AndnotOpcode:
        if (dst != args[0].arg) {
            simdEmitVexOp(compiler, SimdOp::pandn, dst, args[0].arg, args[1].arg);
        } else {
            simdEmitSSEOp(compiler, SimdOp::pandn, dst, args[1].arg);
        }
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[2].arg, args[2].argw);
    }
}

static void emitSelectSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    sljit_s32 tmp = SLJIT_FR3;
    JITArg args[4];

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, SLJIT_FR2);
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, SLJIT_FR1);
    simdOperandToArg(compiler, operands + 2, args[2], SLJIT_SIMD_ELEM_128, SLJIT_FR0);

    args[3].set(operands + 3);
    sljit_s32 dst = GET_TARGET_REG(args[3].arg, SLJIT_FR0);

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, SimdOp::pandn, tmp, args[2].arg, args[1].arg);
        simdEmitVexOp(compiler, SimdOp::pand, dst, args[0].arg, args[2].arg);
    } else {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128, tmp, args[2].arg, 0);
        simdEmitSSEOp(compiler, SimdOp::pandn, tmp, args[1].arg);

        if (dst != args[2].arg) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128, dst, args[2].arg, 0);
        }

        simdEmitSSEOp(compiler, SimdOp::pand, dst, args[0].arg);
    }

    simdEmitSSEOp(compiler, SimdOp::por, dst, tmp);

    if (SLJIT_IS_MEM(args[3].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[3].arg, args[3].argw);
    }
}

static void emitShuffleSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    sljit_s32 tmp1 = SLJIT_FR2;
    sljit_s32 tmp2 = SLJIT_FR3;
    JITArg args[3];

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, SLJIT_FR0);
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, SLJIT_FR1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR1);

    I8X16Shuffle* shuffle = reinterpret_cast<I8X16Shuffle*>(instr->byteCode());
    const sljit_s32 type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | type, tmp1, SLJIT_MEM0(), reinterpret_cast<sljit_sw>(shuffle->value()));

    sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, tmp2, SLJIT_IMM, 0xf0);
    simdEmitSSEOp(compiler, SimdOp::paddb, tmp1, tmp2);

    if (dst != args[1].arg) {
        if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
            simdEmitVexOp(compiler, SimdOp::pshufb, dst, args[1].arg, tmp1);
        } else {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | type, dst, args[1].arg, 0);
            args[1].arg = dst;
        }
    }

    if (dst == args[1].arg) {
        simdEmitSSEOp(compiler, SimdOp::pshufb, dst, tmp1);
    }

    simdEmitSSEOp(compiler, SimdOp::pxor, tmp1, tmp2);

    if (sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, SimdOp::pshufb, tmp2, args[0].arg, tmp1);
    } else {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | type, tmp2, args[0].arg, 0);
        simdEmitSSEOp(compiler, SimdOp::pshufb, tmp2, tmp1);
    }

    simdEmitSSEOp(compiler, SimdOp::por, dst, tmp2);

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | type, dst, args[2].arg, args[2].argw);
    }
}

static void simdEmitI8X16ShiftUImm(sljit_compiler* compiler, bool isLeft, sljit_s32 rd, sljit_s32 rn, sljit_sw argw)
{
    sljit_s32 tmp1 = SLJIT_FR1;

    sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16, tmp1, SLJIT_IMM,
                              isLeft ? ((0xff00 << argw) | 0xff) : ((0xff >> argw) | 0xff00));
    simdEmitOpArg(compiler, OPCODE_AND_IMM(isLeft ? SimdOp::psllw_i : SimdOp::psrlw_i, argw & 0xff),
                  isLeft ? SimdOp::psll_i_arg : SimdOp::psrl_i_arg, rd, rn);
    simdEmitSSEOp(compiler, SimdOp::pand, rd, tmp1);
}

static void simdEmitI8X16Shl(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, JITArg& rm)
{
    sljit_s32 tmp1 = SLJIT_FR1;
    sljit_s32 tmp2 = SLJIT_FR2;

    ASSERT(!SLJIT_IS_IMM(rm.arg));

    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO, tmp1, 0, rm.arg, rm.argw);
    simdEmitSSEOp(compiler, SimdOp::pcmpeqw, tmp2, tmp2);
    simdEmitOp(compiler, SimdOp::psllw, rd, rn, tmp1);
    simdEmitSSEOp(compiler, SimdOp::psllw, tmp2, tmp1);
    sljit_emit_simd_lane_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, tmp2, tmp2, 0);
    simdEmitSSEOp(compiler, SimdOp::pand, rd, tmp2);
}

static void simdEmitI8X16Shr(sljit_compiler* compiler, bool isUnsigned, sljit_s32 rd, sljit_s32 rn, JITArg& rm)
{
    sljit_s32 tmp1 = SLJIT_FR1;
    sljit_s32 tmp2 = SLJIT_FR2;

    simdEmitSSEOp(compiler, SimdOp::punpckhbw, tmp1, rn);
    simdEmitSSEOp(compiler, SimdOp::punpcklbw, rd, rn);

    if (SLJIT_IS_IMM(rm.arg)) {
        ASSERT(!isUnsigned);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psraw_i, (rm.argw | 0x08) & 0xff), SimdOp::psra_i_arg, rd);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psraw_i, (rm.argw | 0x08) & 0xff), SimdOp::psra_i_arg, tmp1);
    } else {
        ASSERT(rm.arg == SLJIT_R0);
        sljit_emit_op2(compiler, SLJIT_OR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0x08);
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO, tmp2, 0, SLJIT_R0, 0);

        uint32_t opcode = isUnsigned ? SimdOp::psrlw : SimdOp::psraw;
        simdEmitSSEOp(compiler, opcode, rd, tmp2);
        simdEmitSSEOp(compiler, opcode, tmp1, tmp2);
    }

    simdEmitSSEOp(compiler, isUnsigned ? SimdOp::packuswb : SimdOp::packsswb, rd, tmp1);
}

static void simdEmitI64X2ShrS(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, JITArg& rm)
{
    // Convert signed to unsigned, and do an unsigned shift, then convert back.
    sljit_s32 tmp1 = SLJIT_FR1;
    sljit_s32 tmp2 = SLJIT_FR2;

    simdEmitSSEOp(compiler, SimdOp::pcmpeqd, tmp1, tmp1);
    simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psllq_i, 63), SimdOp::psll_i_arg, tmp1);

    if (!SLJIT_IS_IMM(rm.arg)) {
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO, tmp2, 0, rm.arg, rm.argw);
    }

    if (rd != rn && sljit_has_cpu_feature(SLJIT_HAS_AVX)) {
        simdEmitVexOp(compiler, SimdOp::pxor, rd, rn, tmp1);
    } else {
        if (rd != rn) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64, rd, rn, 0);
        }

        simdEmitSSEOp(compiler, SimdOp::pxor, rd, tmp1);
    }

    if (SLJIT_IS_IMM(rm.arg)) {
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psrlq_i, rm.argw & 0xff), SimdOp::psrl_i_arg, rd);
        simdEmitSSEOp(compiler, OPCODE_AND_IMM(SimdOp::psrlq_i, rm.argw & 0xff), SimdOp::psrl_i_arg, tmp1);
    } else {
        simdEmitSSEOp(compiler, SimdOp::psrlq, rd, tmp2);
        simdEmitSSEOp(compiler, SimdOp::psrlq, tmp1, tmp2);
    }

    simdEmitSSEOp(compiler, SimdOp::psubq, rd, tmp1);
}

static void emitShiftSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;
    sljit_sw mask = 0;

    switch (instr->opcode()) {
    case ByteCode::I8X16ShlOpcode:
    case ByteCode::I8X16ShrSOpcode:
    case ByteCode::I8X16ShrUOpcode:
        mask = 0x07;
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8ShlOpcode:
    case ByteCode::I16X8ShrSOpcode:
    case ByteCode::I16X8ShrUOpcode:
        mask = 0x0f;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4ShlOpcode:
    case ByteCode::I32X4ShrSOpcode:
    case ByteCode::I32X4ShrUOpcode:
        mask = 0x1f;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2ShlOpcode:
    case ByteCode::I64X2ShrSOpcode:
    case ByteCode::I64X2ShrUOpcode:
        mask = 0x3f;
        type = SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);
    args[1].set(operands + 1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR0);

    if (SLJIT_IS_IMM(args[1].arg)) {
        args[1].argw &= mask;

        if (args[1].argw == 0) {
            if (args[2].arg != args[0].arg) {
                sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, args[0].arg, args[2].arg, args[2].argw);
            }
            return;
        }
    } else {
        sljit_emit_op2(compiler, SLJIT_AND32, SLJIT_R0, 0, args[1].arg, args[1].argw, SLJIT_IMM, mask);
        args[1].arg = SLJIT_R0;
        args[1].argw = 0;
    }

    if (!sljit_has_cpu_feature(SLJIT_HAS_AVX) && dst != args[0].arg) {
        sljit_emit_simd_mov(compiler, type, dst, args[0].arg, 0);
        args[0].arg = dst;
    }

    uint32_t shiftOp = 0;
    uint32_t shiftImmOp = 0;
    uint32_t shiftImmArg = 0;

    switch (instr->opcode()) {
    case ByteCode::I8X16ShlOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitI8X16ShiftUImm(compiler, true, dst, args[0].arg, args[1].argw);
            break;
        }

        simdEmitI8X16Shl(compiler, dst, args[0].arg, args[1]);
        break;
    case ByteCode::I8X16ShrSOpcode:
        simdEmitI8X16Shr(compiler, false, dst, args[0].arg, args[1]);
        break;
    case ByteCode::I8X16ShrUOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitI8X16ShiftUImm(compiler, false, dst, args[0].arg, args[1].argw);
            break;
        }

        simdEmitI8X16Shr(compiler, true, dst, args[0].arg, args[1]);
        break;
    case ByteCode::I16X8ShlOpcode:
        shiftOp = SimdOp::psllw;
        shiftImmOp = SimdOp::psllw_i;
        shiftImmArg = SimdOp::psll_i_arg;
        break;
    case ByteCode::I16X8ShrSOpcode:
        shiftOp = SimdOp::psraw;
        shiftImmOp = SimdOp::psraw_i;
        shiftImmArg = SimdOp::psra_i_arg;
        break;
    case ByteCode::I16X8ShrUOpcode:
        shiftOp = SimdOp::psrlw;
        shiftImmOp = SimdOp::psrlw_i;
        shiftImmArg = SimdOp::psrl_i_arg;
        break;
    case ByteCode::I32X4ShlOpcode:
        shiftOp = SimdOp::pslld;
        shiftImmOp = SimdOp::pslld_i;
        shiftImmArg = SimdOp::psll_i_arg;
        break;
    case ByteCode::I32X4ShrSOpcode:
        shiftOp = SimdOp::psrad;
        shiftImmOp = SimdOp::psrad_i;
        shiftImmArg = SimdOp::psra_i_arg;
        break;
    case ByteCode::I32X4ShrUOpcode:
        shiftOp = SimdOp::psrld;
        shiftImmOp = SimdOp::psrld_i;
        shiftImmArg = SimdOp::psrl_i_arg;
        break;
    case ByteCode::I64X2ShlOpcode:
        shiftOp = SimdOp::psllq;
        shiftImmOp = SimdOp::psllq_i;
        shiftImmArg = SimdOp::psll_i_arg;
        break;
    case ByteCode::I64X2ShrSOpcode:
        simdEmitI64X2ShrS(compiler, dst, args[0].arg, args[1]);
        break;
    case ByteCode::I64X2ShrUOpcode:
        shiftOp = SimdOp::psrlq;
        shiftImmOp = SimdOp::psrlq_i;
        shiftImmArg = SimdOp::psrl_i_arg;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    if (shiftOp != 0) {
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOpArg(compiler, OPCODE_AND_IMM(shiftImmOp, args[1].argw & 0xff), shiftImmArg, dst, args[0].arg);
        } else {
            sljit_s32 tmp1 = SLJIT_FR1;

            sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO, tmp1, 0, args[1].arg, args[1].argw);
            simdEmitOp(compiler, shiftOp, dst, args[0].arg, tmp1);
        }
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}
