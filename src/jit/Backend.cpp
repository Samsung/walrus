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

#if defined(WALRUS_ENABLE_JIT)

#include "Walrus.h"

#include "runtime/GCArray.h"
#include "runtime/GCStruct.h"
#include "runtime/Global.h"
#include "runtime/Function.h"
#include "runtime/Instance.h"
#include "runtime/JITExec.h"
#include "runtime/Memory.h"
#include "runtime/Table.h"
#include "runtime/Tag.h"
#include "jit/Compiler.h"
#ifdef WALRUS_JITPERF
#include "jit/PerfDump.h"
#endif
#include "util/MathOperation.h"

#include <math.h>
#include <map>

// Inlined platform independent assembler backend.
extern "C" {
#include "../../third_party/sljit/sljit_src/sljitLir.c"
}

#if defined SLJIT_CONFIG_UNSUPPORTED && SLJIT_CONFIG_UNSUPPORTED
#error Unsupported architecture
#endif

#define OffsetOfContextField(field) \
    (static_cast<sljit_sw>(offsetof(ExecutionContext, field)))

#define OffsetOfStackTmp(type, field) \
    (stackTmpStart + static_cast<sljit_sw>(offsetof(type, field)))

#if !(defined SLJIT_INDIRECT_CALL && SLJIT_INDIRECT_CALL)
#define GET_FUNC_ADDR(type, func) (reinterpret_cast<type>(func))
#else
#define GET_FUNC_ADDR(type, func) (*reinterpret_cast<type*>(func))
#endif

namespace Walrus {

static const uint8_t kFrameReg = SLJIT_S0;
static const uint8_t kInstanceReg = SLJIT_S1;
static const sljit_sw kContextOffset = 0;

struct JITArg {
    JITArg(Operand* operand)
    {
        this->set(operand);
    }

    JITArg() = default;

    void set(Operand* operand);

    static bool isImm(Operand* operand)
    {
        return VARIABLE_TYPE(*operand) == Instruction::ConstPtr;
    }

    sljit_s32 arg;
    sljit_sw argw;
};

void JITArg::set(Operand* operand)
{
    if (VARIABLE_TYPE(*operand) != Instruction::ConstPtr) {
        if (VARIABLE_TYPE(*operand) == Instruction::Register) {
            this->arg = VARIABLE_GET_REF(*operand);
            this->argw = 0;
            return;
        }
        this->arg = SLJIT_MEM1(kFrameReg);
        this->argw = static_cast<sljit_sw>(VARIABLE_GET_OFFSET(*operand));
        return;
    }

    this->arg = SLJIT_IMM;

    Instruction* instr = VARIABLE_GET_IMM(*operand);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    //    ASSERT(instr->opcode() == ByteCode::Const32Opcode);

    this->argw = static_cast<sljit_s32>(reinterpret_cast<Const32*>(instr->byteCode())->value());
#else /* !SLJIT_32BIT_ARCHITECTURE */
    if (instr->opcode() == ByteCode::Const32Opcode) {
        this->argw = static_cast<sljit_s32>(reinterpret_cast<Const32*>(instr->byteCode())->value());
        return;
    }

    this->argw = static_cast<sljit_sw>(reinterpret_cast<Const64*>(instr->byteCode())->value());
#endif /* SLJIT_32BIT_ARCHITECTURE */
}

class InstanceConstData {
public:
    static const size_t globalTryBlock = ~static_cast<size_t>(0);

    struct TryBlock {
        TryBlock(size_t catchStart, size_t catchCount, size_t parent, sljit_uw returnToAddr)
            : catchStart(catchStart)
            , catchCount(catchCount)
            , parent(parent)
            , returnToAddr(returnToAddr)
        {
        }

        size_t catchStart;
        size_t catchCount;
        size_t parent;
        sljit_uw returnToAddr;
    };

    struct CatchBlock {
        CatchBlock(sljit_uw handlerAddr, size_t stackSizeToBe, uint32_t tagIndex)
            : handlerAddr(handlerAddr)
            , stackSizeToBe(stackSizeToBe)
            , tagIndex(tagIndex)
        {
        }

        sljit_uw handlerAddr;
        size_t stackSizeToBe;
        uint32_t tagIndex;
    };

    InstanceConstData(std::vector<TrapBlock>& trapBlocks, std::vector<Walrus::TryBlock>& tryBlocks);
    void append(std::vector<TrapBlock>& trapBlocks, std::vector<Walrus::TryBlock>& tryBlocks);

    std::vector<TryBlock>& tryBlocks() { return m_tryBlocks; }
    std::vector<CatchBlock>& catchBlocks() { return m_catchBlocks; }

    sljit_uw find(sljit_uw return_addr)
    {
        size_t begin = 0;
        size_t end = m_trapList.size();

        while (true) {
            size_t mid = ((begin + end) >> 2) << 1;

            if (m_trapList[mid] < return_addr) {
                begin = mid + 2;
                continue;
            }

            if (mid == 0 || m_trapList[mid - 2] < return_addr) {
                return m_trapList[mid + 1];
            }

            end = mid - 2;
        }
    }

private:
    std::vector<sljit_uw> m_trapList;
    std::vector<TryBlock> m_tryBlocks;
    std::vector<CatchBlock> m_catchBlocks;
};

class JITFieldAccessor {
public:
    static sljit_sw globalValueOffset()
    {
        return offsetof(Global, m_value) + offsetof(Value, m_i32);
    }

    static sljit_sw tableSizeOffset()
    {
        return offsetof(Table, m_size);
    }

    static sljit_sw tableElements()
    {
        return offsetof(Table, m_elements);
    }

    static sljit_sw objectTypeInfo()
    {
        return offsetof(Object, m_typeInfo);
    }

    static sljit_sw arrayLength()
    {
        return offsetof(GCArray, m_length);
    }
};

class SlowCase {
public:
    enum class Type {
        SignedDivide,
        SignedDivide32,
        SignedModulo,
        SignedModulo32,
        ConvertIntFromFloat,
        ConvertUnsignedIntFromFloat,
    };

    SlowCase(Type type, sljit_jump* jump_from, sljit_label* resume_label, Instruction* instr)
        : m_type(type)
        , m_jumpFrom(jump_from)
        , m_resumeLabel(resume_label)
        , m_instr(instr)
    {
    }

    virtual ~SlowCase() {}

    void emit(sljit_compiler* compiler);

protected:
    Type m_type;
    sljit_jump* m_jumpFrom;
    sljit_label* m_resumeLabel;
    Instruction* m_instr;
};

CompileContext::CompileContext(Module* module, JITCompiler* compiler)
    : compiler(compiler)
    , branchTableOffset(0)
#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
    , shuffleOffset(0)
#endif /* SLJIT_CONFIG_X86 */
    , stackTmpStart(sizeof(sljit_sw))
    , nextTryBlock(0)
    , currentTryBlock(InstanceConstData::globalTryBlock)
    , trapBlocksStart(0)
    , module(module)
{
    // Compiler is not initialized yet.
    size_t offset = Instance::alignedSize();
    size_t numberOfMemoryTypes = module->numberOfMemoryTypes();
    targetBuffersStart = offset + numberOfMemoryTypes * sizeof(void*);
    globalsStart = targetBuffersStart + numberOfMemoryTypes * sizeof(Memory::TargetBuffer);
    tableStart = globalsStart + module->numberOfGlobalTypes() * sizeof(void*);
    functionsStart = tableStart + module->numberOfTableTypes() * sizeof(void*);
    dataSegmentsStart = functionsStart + (module->numberOfFunctions() + module->numberOfTagTypes()) * sizeof(void*);
    elementSegmentsStart = dataSegmentsStart + module->numberOfDataSegments() * sizeof(DataSegment);
}

CompileContext* CompileContext::get(sljit_compiler* compiler)
{
    void* context = sljit_compiler_get_user_data(compiler);
    return reinterpret_cast<CompileContext*>(context);
}

#define GET_TARGET_REG(arg, default_reg) \
    (SLJIT_IS_REG(arg) ? (arg) : (default_reg))
#define GET_SOURCE_REG(arg, default_reg) \
    (SLJIT_IS_REG(arg) ? (arg) : (default_reg))
#define MOVE_TO_REG(compiler, mov_op, target_reg, arg, argw)              \
    if ((target_reg) != (arg)) {                                          \
        sljit_emit_op1(compiler, mov_op, (target_reg), 0, (arg), (argw)); \
    }
#define MOVE_FROM_REG(compiler, mov_op, arg, argw, source_reg)            \
    if ((source_reg) != (arg)) {                                          \
        sljit_emit_op1(compiler, mov_op, (arg), (argw), (source_reg), 0); \
    }

static void moveIntToDest(sljit_compiler* compiler, sljit_s32 movOp, JITArg& dstArg, sljit_sw offset)
{
    if (SLJIT_IS_REG(dstArg.arg)) {
        sljit_emit_op1(compiler, movOp, dstArg.arg, dstArg.argw, SLJIT_MEM1(SLJIT_TMP_DEST_REG), offset);
        return;
    }

    sljit_emit_op1(compiler, movOp, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), offset);
    sljit_emit_op1(compiler, movOp, dstArg.arg, dstArg.argw, SLJIT_TMP_DEST_REG, 0);
}

static void moveFloatToDest(sljit_compiler* compiler, sljit_s32 movOp, JITArg& dstArg, sljit_sw offset)
{
    if (SLJIT_IS_REG(dstArg.arg)) {
        sljit_emit_fop1(compiler, movOp, dstArg.arg, dstArg.argw, SLJIT_MEM1(SLJIT_TMP_DEST_REG), offset);
        return;
    }

    sljit_emit_fop1(compiler, movOp, SLJIT_TMP_DEST_FREG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), offset);
    sljit_emit_fop1(compiler, movOp, dstArg.arg, dstArg.argw, SLJIT_TMP_DEST_FREG, 0);
}

static void emitInitR0R1(sljit_compiler* compiler, sljit_s32 movOp1, sljit_s32 movOp2, JITArg* params)
{
    if (params[1].arg != SLJIT_R0) {
        MOVE_TO_REG(compiler, movOp1, SLJIT_R0, params[0].arg, params[0].argw);
        MOVE_TO_REG(compiler, movOp2, SLJIT_R1, params[1].arg, params[1].argw);
        return;
    }

    if (params[0].arg != SLJIT_R1) {
        sljit_emit_op1(compiler, movOp2, SLJIT_R1, 0, SLJIT_R0, 0);
        MOVE_TO_REG(compiler, movOp1, SLJIT_R0, params[0].arg, params[0].argw);
        return;
    }

    // Swap arguments.
    sljit_emit_op1(compiler, movOp2, SLJIT_TMP_DEST_REG, 0, SLJIT_R0, 0);
    sljit_emit_op1(compiler, movOp1, SLJIT_R0, 0, SLJIT_R1, 0);
    sljit_emit_op1(compiler, movOp2, SLJIT_R1, 0, SLJIT_TMP_DEST_REG, 0);
}

static void emitInitR0R1R2(sljit_compiler* compiler, sljit_s32 movOp1, sljit_s32 movOp2, sljit_s32 movOp3, Operand* params)
{
    JITArg src[3] = { params, params + 1, params + 2 };
    int i = 0;

    if (src[1].arg == SLJIT_R0 || src[2].arg == SLJIT_R0) {
        i = 1;

        if (src[0].arg == SLJIT_R1 || src[2].arg == SLJIT_R1) {
            i = 2;

            if (src[0].arg == SLJIT_R2 || src[1].arg == SLJIT_R2) {
                // Rotating three registers.
                if (src[0].arg == SLJIT_R1) {
                    ASSERT(src[1].arg == SLJIT_R2 && src[2].arg == SLJIT_R0);

                    sljit_emit_op1(compiler, movOp3, SLJIT_TMP_DEST_REG, 0, SLJIT_R0, 0);
                    sljit_emit_op1(compiler, movOp1, SLJIT_R0, 0, SLJIT_R1, 0);
                    sljit_emit_op1(compiler, movOp2, SLJIT_R1, 0, SLJIT_R2, 0);
                    sljit_emit_op1(compiler, movOp3, SLJIT_R2, 0, SLJIT_TMP_DEST_REG, 0);
                    return;
                }

                ASSERT(src[0].arg == SLJIT_R2 && src[1].arg == SLJIT_R0 && src[2].arg == SLJIT_R1);
                sljit_emit_op1(compiler, movOp2, SLJIT_TMP_DEST_REG, 0, SLJIT_R0, 0);
                sljit_emit_op1(compiler, movOp1, SLJIT_R0, 0, SLJIT_R2, 0);
                sljit_emit_op1(compiler, movOp3, SLJIT_R2, 0, SLJIT_R1, 0);
                sljit_emit_op1(compiler, movOp2, SLJIT_R1, 0, SLJIT_TMP_DEST_REG, 0);
                return;
            }
        }
    }

    sljit_s32 movOps[3] = { movOp1, movOp2, movOp3 };

    int other1 = i > 0 ? 0 : 1;
    int other2 = i < 2 ? 2 : 1;
    int sljit_r1 = SLJIT_R(other1);
    int sljit_r2 = SLJIT_R(other2);

    // This operation does not destroy arguments.
    sljit_emit_op1(compiler, movOps[i], SLJIT_R(i), 0, src[i].arg, src[i].argw);

    ASSERT(i != other1 && i != other2 && other1 != other2);

    if (src[other2].arg != sljit_r1) {
        MOVE_TO_REG(compiler, movOps[other1], sljit_r1, src[other1].arg, src[other1].argw);
        MOVE_TO_REG(compiler, movOps[other2], sljit_r2, src[other2].arg, src[other2].argw);
    } else if (src[other1].arg != sljit_r2) {
        sljit_emit_op1(compiler, movOps[other2], sljit_r2, 0, sljit_r1, 0);
        MOVE_TO_REG(compiler, movOps[other1], sljit_r1, src[other1].arg, src[other1].argw);
    } else {
        // Swap arguments.
        sljit_emit_op1(compiler, movOps[other2], SLJIT_TMP_DEST_REG, 0, sljit_r1, 0);
        sljit_emit_op1(compiler, movOps[other1], sljit_r1, 0, sljit_r2, 0);
        sljit_emit_op1(compiler, movOps[other2], sljit_r2, 0, SLJIT_TMP_DEST_REG, 0);
    }
}

static void emitSelect128(sljit_compiler*, Instruction*, sljit_s32);
static void emitMove(sljit_compiler*, uint32_t type, Operand* from, Operand* to);
static void emitStoreImmediate(sljit_compiler* compiler, Operand* to, Instruction* instr, bool isFloat);
static ByteCodeStackOffset* emitStoreOntoStack(sljit_compiler* compiler, Operand* param, ByteCodeStackOffset* stackOffset, const TypeVector& types, bool isWordOffsets);

#if (defined SLJIT_CONFIG_ARM && SLJIT_CONFIG_ARM) || (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86) || (defined SLJIT_CONFIG_RISCV && SLJIT_CONFIG_RISCV && defined __riscv_vector)
#define HAS_SIMD

#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
constexpr uint8_t getHighRegister(sljit_s32 reg)
{
    return reg + 1;
}
#endif /* SLJIT_CONFIG_ARM_32 */

static void simdOperandToArg(sljit_compiler* compiler, Operand* operand, JITArg& arg, sljit_s32 type, sljit_s32 srcReg)
{
    VariableRef ref = *operand;

    if (VARIABLE_TYPE(ref) != Instruction::ConstPtr) {
        arg.set(operand);

        if (SLJIT_IS_MEM(arg.arg)) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | type, srcReg, arg.arg, arg.argw);

            arg.arg = srcReg;
            arg.argw = 0;
        }
        return;
    }

    Instruction* instr = VARIABLE_GET_IMM(ref);
    ASSERT(instr->opcode() == ByteCode::Const128Opcode);

    const uint8_t* value = reinterpret_cast<Const128*>(instr->byteCode())->value();
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | type, srcReg, SLJIT_MEM0(), (sljit_sw)value);

    arg.arg = srcReg;
    arg.argw = 0;
}

#endif /* SLJIT_CONFIG_ARM || SLJIT_CONFIG_X86 || SLJIT_CONFIG_RISCV */

#include "FloatMathInl.h"

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
#include "IntMath32Inl.h"
#else /* !SLJIT_32BIT_ARCHITECTURE */
#include "IntMath64Inl.h"
#endif /* SLJIT_32BIT_ARCHITECTURE */

#include "FloatConvInl.h"
#include "CallInl.h"
#include "MemoryInl.h"
#include "MemoryUtilInl.h"
#include "TableInl.h"
#include "TryCatchInl.h"
#include "GarbageCollectorInl.h"

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
#include "SimdX86Inl.h"
#elif (defined SLJIT_CONFIG_ARM_64 && SLJIT_CONFIG_ARM_64)
#include "SimdArm64Inl.h"
#elif (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
#include "SimdArm32Inl.h"
#elif (defined SLJIT_CONFIG_RISCV && SLJIT_CONFIG_RISCV && defined __riscv_vector)
#include "SimdRiscvInl.h"
#endif /* SLJIT_CONFIG_ARM */

#ifdef HAS_SIMD
#include "SimdInl.h"
#endif /* HAS_SIMD */

void CompileContext::emitSlowCases(sljit_compiler* compiler)
{
    for (auto it : slowCases) {
        SlowCase* slowCase = it;

        slowCase->emit(compiler);
        delete slowCase;
    }
    slowCases.clear();
}

void SlowCase::emit(sljit_compiler* compiler)
{
    sljit_set_label(m_jumpFrom, sljit_emit_label(compiler));

    CompileContext* context = CompileContext::get(compiler);

    switch (m_type) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case Type::SignedDivide32:
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case Type::SignedDivide: {
        sljit_s32 current_flags = SLJIT_CURRENT_FLAGS_SUB | SLJIT_CURRENT_FLAGS_COMPARE | SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        if (m_type == Type::SignedDivide32) {
            current_flags |= SLJIT_32;
        }
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_set_current_flags(compiler, current_flags);

        /* Division by zero. */
        context->appendTrapJump(ExecutionContext::DivideByZeroError, sljit_emit_jump(compiler, SLJIT_EQUAL));

        sljit_s32 type = SLJIT_NOT_EQUAL;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_sw intMin = static_cast<sljit_sw>(INT64_MIN);

        if (m_type == Type::SignedDivide32) {
            type |= SLJIT_32;
            intMin = static_cast<sljit_sw>(INT32_MIN);
        }
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_sw intMin = static_cast<sljit_sw>(INT32_MIN);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_jump* cmp = sljit_emit_cmp(compiler, type, SLJIT_R0, 0, SLJIT_IMM, intMin);
        sljit_set_label(cmp, m_resumeLabel);

        context->appendTrapJump(ExecutionContext::IntegerOverflowError, sljit_emit_jump(compiler, SLJIT_JUMP));
        return;
    }
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case Type::SignedModulo32:
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case Type::SignedModulo: {
        sljit_s32 current_flags = SLJIT_CURRENT_FLAGS_SUB | SLJIT_CURRENT_FLAGS_COMPARE | SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        if (m_type == Type::SignedModulo32) {
            current_flags |= SLJIT_32;
        }
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_set_current_flags(compiler, current_flags);

        /* Division by zero. */
        context->appendTrapJump(ExecutionContext::DivideByZeroError, sljit_emit_jump(compiler, SLJIT_EQUAL));

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op1(compiler, (m_type == Type::SignedModulo32) ? SLJIT_MOV32 : SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, 0);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, 0);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), m_resumeLabel);
        return;
    }
    case Type::ConvertIntFromFloat: {
        reinterpret_cast<ConvertIntFromFloatSlowCase*>(this)->emitSlowCase(compiler);
        return;
    }
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE) && (SLJIT_CONV_NAN_FLOAT != SLJIT_CONV_RESULT_ZERO)
    case Type::ConvertUnsignedIntFromFloat: {
        sljit_jump* cmp = reinterpret_cast<ConvertUnsignedIntFromFloatSlowCase*>(this)->emitCompareUnordered(compiler);
        context->appendTrapJump(ExecutionContext::InvalidConversionToIntegerError, cmp);
        context->appendTrapJump(ExecutionContext::IntegerOverflowError, sljit_emit_jump(compiler, SLJIT_JUMP));
        return;
    }
#endif /* SLJIT_64BIT_ARCHITECTURE */
    default: {
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    }
}

static void emitStoreImmediate(sljit_compiler* compiler, Operand* to, Instruction* instr, bool isFloat)
{
    if (VARIABLE_TYPE(*to) == Instruction::Offset) {
        sljit_sw offset = VARIABLE_GET_OFFSET(*to);

        switch (instr->opcode()) {
#ifdef HAS_SIMD
        case ByteCode::Const128Opcode: {
            const uint8_t* value = reinterpret_cast<Const128*>(instr->byteCode())->value();

            sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_TMP_DEST_FREG, SLJIT_MEM0(), (sljit_sw)value);
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_TMP_DEST_FREG, SLJIT_MEM1(kFrameReg), offset);
            return;
        }
#endif /* HAS_SIMD */
        case ByteCode::Const32Opcode: {
            uint32_t value32 = reinterpret_cast<Const32*>(instr->byteCode())->value();
            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kFrameReg), offset, SLJIT_IMM, static_cast<sljit_s32>(value32));
            return;
        }
        default: {
            ASSERT(instr->opcode() == ByteCode::Const64Opcode);

            uint64_t value64 = reinterpret_cast<Const64*>(instr->byteCode())->value();
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kFrameReg), offset + WORD_LOW_OFFSET, SLJIT_IMM, static_cast<sljit_sw>(value64));
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kFrameReg), offset + WORD_HIGH_OFFSET, SLJIT_IMM, static_cast<sljit_sw>(value64 >> 32));
#else /* !SLJIT_32BIT_ARCHITECTURE */
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kFrameReg), offset, SLJIT_IMM, static_cast<sljit_sw>(value64));
#endif /* SLJIT_32BIT_ARCHITECTURE */
            return;
        }
        }
    }

    sljit_s32 reg = static_cast<sljit_s32>(VARIABLE_GET_REF(*to));

    switch (instr->opcode()) {
#ifdef HAS_SIMD
    case ByteCode::Const128Opcode: {
        const uint8_t* value = reinterpret_cast<Const128*>(instr->byteCode())->value();

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, reg, SLJIT_MEM0(), (sljit_sw)value);
        return;
    }
#endif /* HAS_SIMD */
    case ByteCode::Const32Opcode: {
        uint32_t value32 = reinterpret_cast<Const32*>(instr->byteCode())->value();

        if (isFloat) {
            union {
                uint32_t valueI32;
                sljit_f32 valueF32;
            } u;

            u.valueI32 = value32;
            sljit_emit_fset32(compiler, reg, u.valueF32);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV32, reg, 0, SLJIT_IMM, static_cast<sljit_s32>(value32));
        }
        return;
    }
    default: {
        ASSERT(instr->opcode() == ByteCode::Const64Opcode);

        uint64_t value64 = reinterpret_cast<Const64*>(instr->byteCode())->value();

        if (isFloat) {
            union {
                uint64_t valueI64;
                sljit_f64 valueF64;
            } u;

            u.valueI64 = value64;
            sljit_emit_fset64(compiler, reg, u.valueF64);
        } else {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            sljit_emit_op1(compiler, SLJIT_MOV, reg & 0xff, 0, SLJIT_IMM, static_cast<sljit_sw>(value64));
            sljit_emit_op1(compiler, SLJIT_MOV, reg >> 8, 0, SLJIT_IMM, static_cast<sljit_sw>(value64 >> 32));
#else /* !SLJIT_32BIT_ARCHITECTURE */
            sljit_emit_op1(compiler, SLJIT_MOV, reg, 0, SLJIT_IMM, static_cast<sljit_sw>(value64));
#endif /* SLJIT_32BIT_ARCHITECTURE */
        }
        return;
    }
    }
}

static void emitMove(sljit_compiler* compiler, uint32_t type, Operand* from, Operand* to)
{
    ASSERT(VARIABLE_TYPE(*from) != Instruction::ConstPtr && VARIABLE_TYPE(*to) != Instruction::ConstPtr);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (type == Instruction::Int64Operand) {
        JITArgPair src(from);
        JITArgPair dst(to);

        sljit_emit_op1(compiler, SLJIT_MOV, dst.arg1, dst.arg1w, src.arg1, src.arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, dst.arg2, dst.arg2w, src.arg2, src.arg2w);
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    JITArg src(from);
    JITArg dst(to);

    switch (type) {
    case Instruction::Int32Operand:
        sljit_emit_op1(compiler, SLJIT_MOV32, dst.arg, dst.argw, src.arg, src.argw);
        return;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case Instruction::Int64Operand:
        sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, src.arg, src.argw);
        return;
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case Instruction::Float32Operand:
        sljit_emit_fop1(compiler, SLJIT_MOV_F32, dst.arg, dst.argw, src.arg, src.argw);
        return;
    case Instruction::Float64Operand:
        sljit_emit_fop1(compiler, SLJIT_MOV_F64, dst.arg, dst.argw, src.arg, src.argw);
        return;
    default:
        break;
    }

    ASSERT(type == Instruction::V128Operand);

    if (!SLJIT_IS_MEM(src.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128, src.arg, dst.arg, dst.argw);
        return;
    }

    sljit_s32 dstReg = GET_TARGET_REG(dst.arg, SLJIT_TMP_DEST_VREG);
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128, dstReg, src.arg, src.argw);

    if (dstReg == SLJIT_TMP_DEST_VREG) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128, SLJIT_TMP_DEST_VREG, dst.arg, dst.argw);
    }
}

static void emitImmediate(sljit_compiler* compiler, Instruction* instr)
{
    Operand* result = instr->operands();

    ASSERT(instr->group() == Instruction::Immediate);

    if (!(instr->info() & Instruction::kKeepInstruction)) {
        return;
    }

    emitStoreImmediate(compiler, result, instr, (instr->info() & Instruction::kHasFloatOperand) != 0);
}

static ByteCodeStackOffset* emitStoreOntoStack(sljit_compiler* compiler, Operand* param, ByteCodeStackOffset* stackOffset, const TypeVector& types, bool isWordOffsets)
{
    for (auto it : types.types()) {
        Operand dst = VARIABLE_SET(STACK_OFFSET(*stackOffset), Instruction::Offset);

        switch (VARIABLE_TYPE(*param)) {
        case Instruction::ConstPtr:
            ASSERT(!(VARIABLE_GET_IMM(*param)->info() & Instruction::kKeepInstruction));
            emitStoreImmediate(compiler, &dst, VARIABLE_GET_IMM(*param), false);
            break;
        case Instruction::Register:
            emitMove(compiler, Instruction::valueTypeToOperandType(it), param, &dst);
            break;
        }

        if (isWordOffsets) {
            stackOffset += ((valueSize(it) + (sizeof(size_t) - 1)) / sizeof(size_t)) - 1;
        }

        stackOffset++;
        param++;
    }

    return stackOffset;
}

static void emitEnd(sljit_compiler* compiler, Instruction* instr)
{
    End* end = reinterpret_cast<End*>(instr->byteCode());

    CompileContext* context = CompileContext::get(compiler);
    FunctionType* functionType = context->compiler->moduleFunction()->functionType();

    emitStoreOntoStack(compiler, instr->params(), end->resultOffsets(), functionType->result(), true);
    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(end->resultOffsets()));

    if (instr->info() & Instruction::kEarlyReturn) {
        context->earlyReturns.push_back(sljit_emit_jump(compiler, SLJIT_JUMP));
    }
}

static void emitDirectBranch(sljit_compiler* compiler, Instruction* instr)
{
    sljit_jump* jump;

    switch (instr->opcode()) {
    case ByteCode::JumpOpcode: {
        jump = sljit_emit_jump(compiler, SLJIT_JUMP);
        CompileContext::get(compiler)->emitSlowCases(compiler);
        break;
    }
    case ByteCode::JumpIfCastGenericOpcode:
        emitGCCastGeneric(compiler, instr);
        return;
    case ByteCode::JumpIfCastDefinedOpcode:
        emitGCCastDefined(compiler, instr);
        return;
    default: {
        JITArg src(instr->operands());

        sljit_s32 type = (instr->opcode() == ByteCode::JumpIfTrueOpcode || instr->opcode() == ByteCode::JumpIfNonNullOpcode) ? SLJIT_NOT_EQUAL : SLJIT_EQUAL;

        if (instr->opcode() == ByteCode::JumpIfTrueOpcode || instr->opcode() == ByteCode::JumpIfFalseOpcode) {
            type |= SLJIT_32;
        }

        jump = sljit_emit_cmp(compiler, type, src.arg, src.argw, SLJIT_IMM, 0);
        break;
    }
    }

    instr->asExtended()->value().targetLabel->jumpFrom(jump);
}

static void emitBrTable(sljit_compiler* compiler, BrTableInstruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    size_t targetLabelCount = instr->targetLabelCount();
    Label** label = instr->targetLabels();
    Label** end = label + targetLabelCount;
    sljit_s32 tmp = SLJIT_TMP_DEST_REG;
    JITArg src(instr->operands());

    sljit_emit_op1(compiler, SLJIT_MOV_U32, tmp, 0, src.arg, src.argw);

    if (sljit_has_cpu_feature(SLJIT_HAS_CMOV)) {
        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_GREATER_EQUAL, tmp, 0, SLJIT_IMM, static_cast<sljit_sw>(targetLabelCount - 1));
        sljit_emit_select(compiler, SLJIT_GREATER_EQUAL, tmp, SLJIT_IMM, static_cast<sljit_sw>(targetLabelCount - 1), tmp);
    } else {
        targetLabelCount--;
        end--;
        (*end)->jumpFrom(sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, tmp, 0, SLJIT_IMM, static_cast<sljit_sw>(targetLabelCount)));
    }

    sljit_emit_op2(compiler, SLJIT_SHL, tmp, 0, tmp, 0, SLJIT_IMM, SLJIT_WORD_SHIFT);

    if (targetLabelCount <= JITCompiler::kMaxInlinedBranchTable) {
        CompileContext* context = CompileContext::get(compiler);
        JITCompiler::BranchTableLabels* brTableLabels = new JITCompiler::BranchTableLabels(context->compiler, targetLabelCount);

        brTableLabels->header.size = targetLabelCount * sizeof(sljit_sw);

        brTableLabels->jump = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, tmp, 0);
        sljit_emit_ijump(compiler, SLJIT_JUMP, SLJIT_MEM1(tmp), 0);

        while (label < end) {
            brTableLabels->labels.push_back(JITCompiler::BranchTableLabels::Item(*label));
            label++;
        }
        return;
    }

    sljit_emit_ijump(compiler, SLJIT_JUMP, SLJIT_MEM1(tmp), static_cast<sljit_sw>(context->branchTableOffset));

    sljit_uw* target = reinterpret_cast<sljit_uw*>(context->branchTableOffset);

    while (label < end) {
        *target++ = reinterpret_cast<sljit_uw>(*label);
        label++;
    }

    context->branchTableOffset = reinterpret_cast<sljit_uw>(target);
}

static void emitMoveI32(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg src(operands);
    JITArg dst(operands + 1);

    sljit_emit_op1(compiler, SLJIT_MOV32, dst.arg, dst.argw, src.arg, src.argw);
}

static void emitReinterpretOperation(sljit_compiler* compiler, Instruction* instr)
{
    Operand* src = instr->operands();
    Operand* dst = src + 1;

    bool fromFloat = (instr->opcode() == ByteCode::I32ReinterpretF32Opcode) || (instr->opcode() == ByteCode::I64ReinterpretF64Opcode);

    if (VARIABLE_TYPE(*src) == Instruction::ConstPtr) {
        emitStoreImmediate(compiler, dst, VARIABLE_GET_IMM(*src), !fromFloat);
        return;
    }

    uint32_t type;

    if (VARIABLE_TYPE(*src) != Instruction::Register) {
        // Source is memory.
        if (fromFloat) {
            type = instr->opcode() == ByteCode::I32ReinterpretF32Opcode ? Instruction::Int32Operand : Instruction::Int64Operand;
        } else {
            type = instr->opcode() == ByteCode::F32ReinterpretI32Opcode ? Instruction::Float32Operand : Instruction::Float64Operand;
        }

        emitMove(compiler, type, src, dst);
        return;
    }

    if (VARIABLE_TYPE(*dst) != Instruction::Register) {
        // Destination is memory.
        if (fromFloat) {
            type = instr->opcode() == ByteCode::I32ReinterpretF32Opcode ? Instruction::Float32Operand : Instruction::Float64Operand;
        } else {
            type = instr->opcode() == ByteCode::F32ReinterpretI32Opcode ? Instruction::Int32Operand : Instruction::Int64Operand;
        }
        emitMove(compiler, type, src, dst);
        return;
    }

    sljit_sw floatReg;
    sljit_sw intReg;
    sljit_s32 op;

    if (fromFloat) {
        floatReg = VARIABLE_GET_REF(*src);
        intReg = VARIABLE_GET_REF(*dst);

        op = (instr->opcode() == ByteCode::I32ReinterpretF32Opcode) ? SLJIT_COPY32_FROM_F32 : SLJIT_COPY_FROM_F64;
    } else {
        floatReg = VARIABLE_GET_REF(*dst);
        intReg = VARIABLE_GET_REF(*src);

        op = (instr->opcode() == ByteCode::F32ReinterpretI32Opcode) ? SLJIT_COPY32_TO_F32 : SLJIT_COPY_TO_F64;
    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (!SLJIT_IS_REG(intReg)) {
        /* Swap registers. */
        intReg = SLJIT_REG_PAIR(intReg >> 8, intReg & 0xff);
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    sljit_emit_fcopy(compiler, op, floatReg, intReg);
}

static void emitGlobalGet32(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    GlobalGet32* globalGet = reinterpret_cast<GlobalGet32*>(instr->byteCode());
    JITArg dstArg(instr->operands());

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->globalsStart + globalGet->index() * sizeof(void*));

    if (instr->info() & Instruction::kHasFloatOperand) {
        moveFloatToDest(compiler, SLJIT_MOV_F32, dstArg, JITFieldAccessor::globalValueOffset());
    } else {
        moveIntToDest(compiler, SLJIT_MOV32, dstArg, JITFieldAccessor::globalValueOffset());
    }
}

static void emitGlobalSet32(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    GlobalSet32* globalSet = reinterpret_cast<GlobalSet32*>(instr->byteCode());
    JITArg src;
    sljit_s32 baseReg;

    if (instr->info() & Instruction::kHasFloatOperand) {
        floatOperandToArg(compiler, instr->operands(), src, SLJIT_TMP_DEST_FREG);
        baseReg = SLJIT_TMP_DEST_REG;
    } else {
        src.set(instr->operands());
        baseReg = instr->requiredReg(0);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, baseReg, 0, SLJIT_MEM1(kInstanceReg), context->globalsStart + globalSet->index() * sizeof(void*));

    if (SLJIT_IS_MEM(src.arg)) {
        if (instr->info() & Instruction::kHasFloatOperand) {
            sljit_emit_fop1(compiler, SLJIT_MOV_F32, SLJIT_TMP_DEST_FREG, 0, src.arg, src.argw);
            src.arg = SLJIT_TMP_DEST_FREG;
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, src.arg, src.argw);
            src.arg = SLJIT_TMP_DEST_REG;
        }
        src.argw = 0;
    }

    if (instr->info() & Instruction::kHasFloatOperand) {
        sljit_emit_fop1(compiler, SLJIT_MOV_F32, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset(), src.arg, src.argw);
    } else {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset(), src.arg, src.argw);
    }
}

static void emitRefFunc(sljit_compiler* compiler, Instruction* instr)
{
    JITArg dstArg(instr->operands());

    CompileContext* context = CompileContext::get(compiler);

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, kInstanceReg, 0);
    moveIntToDest(compiler, SLJIT_MOV_P, dstArg, context->functionsStart + (sizeof(Function*) * (reinterpret_cast<RefFunc*>(instr->byteCode()))->funcIndex()));
}

static void emitRefAsNonNull(sljit_compiler* compiler, Instruction* instr)
{
    JITArg srcArg(instr->params());
    CompileContext* context = CompileContext::get(compiler);

    context->appendTrapJump(ExecutionContext::NullReferenceError, sljit_emit_cmp(compiler, SLJIT_EQUAL, srcArg.arg, srcArg.argw, SLJIT_IMM, 0));
}

static void emitStackInit(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t type;

    switch (instr->opcode()) {
    case ByteCode::MoveI32Opcode:
        type = Instruction::Int32Operand;
        break;
    case ByteCode::MoveI64Opcode:
        type = Instruction::Int64Operand;
        break;
    case ByteCode::MoveF32Opcode:
        type = Instruction::Float32Operand;
        break;
    case ByteCode::MoveF64Opcode:
        type = Instruction::Float64Operand;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::MoveV128Opcode);
        type = Instruction::V128Operand;
        break;
    }

    Operand src = instr->asExtended()->value().offset;
    emitMove(compiler, type, &src, instr->operands());
}

JITModule::~JITModule()
{
    delete m_instanceConstData;
    sljit_free_code(m_moduleStart, nullptr);

    for (auto it : m_codeBlocks) {
        sljit_free_code(it, nullptr);
    }
}

struct LabelJumpList {
    std::vector<sljit_jump*> jumpList;
};

void Label::jumpFrom(sljit_jump* jump)
{
    if (info() & Label::kHasLabelData) {
        sljit_set_label(jump, m_label);
        return;
    }

    if (!(info() & Label::kHasJumpList)) {
        m_jumpList = new LabelJumpList;
        addInfo(Label::kHasJumpList);
    }

    m_jumpList->jumpList.push_back(jump);
}

void Label::emit(sljit_compiler* compiler)
{
    ASSERT(!(info() & Label::kHasLabelData));

    sljit_label* label = sljit_emit_label(compiler);

    if (info() & Label::kHasJumpList) {
        for (auto it : m_jumpList->jumpList) {
            sljit_set_label(it, label);
        }

        delete m_jumpList;
        setInfo(info() ^ Label::kHasJumpList);
    }

    m_label = label;
    addInfo(Label::kHasLabelData);
}

JITCompiler::JITCompiler(Module* module, uint32_t JITFlags)
    : m_first(nullptr)
    , m_last(nullptr)
    , m_compiler(nullptr)
    , m_context(module, this)
    , m_module(module)
    , m_moduleFunction(nullptr)
    , m_variableList(nullptr)
    , m_brTableLabels(nullptr)
    , m_lastBrTableLabels(nullptr)
    , m_branchTableSize(0)
    , m_tryBlockStart(0)
    , m_tryBlockOffset(0)
    , m_JITFlags(JITFlags)
    , m_options(0)
    , m_savedIntegerRegCount(0)
    , m_savedFloatRegCount(0)
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    , m_savedVectorRegCount(0)
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */
    , m_stackTmpSize(0)
{
    if (module->m_jitModule != nullptr) {
        ASSERT(module->m_jitModule->m_instanceConstData != nullptr);
        m_tryBlockOffset = module->m_jitModule->m_instanceConstData->tryBlocks().size();
    }

    if (sljit_has_cpu_feature(SLJIT_HAS_CMOV)) {
        m_options |= JITCompiler::kHasCondMov;
    }
}

void JITCompiler::compileFunction(JITFunction* jitFunc, bool isExternal)
{
    ASSERT(m_first != nullptr && m_last != nullptr);

    m_functionList.push_back(FunctionList(jitFunc, isExternal, m_branchTableSize));

    if (m_compiler == nullptr) {
        // First compiled function.
        m_compiler = sljit_create_compiler(nullptr);
        sljit_compiler_set_user_data(m_compiler, reinterpret_cast<void*>(&m_context));

        if (module()->m_jitModule == nullptr) {
            // Follows the declaration of FunctionDescriptor::ExternalDecl().
            // Frame stored in SLJIT_S0 (kFrameReg)
            // Instance stored in SLJIT_S1 (kInstanceReg)
            sljit_emit_enter(m_compiler, 0, SLJIT_ARGS3(P, P_R, P, P_R), 3, 2, 0);
            sljit_emit_op1(m_compiler, SLJIT_MOV_P, kInstanceReg, 0, SLJIT_MEM1(SLJIT_R0), OffsetOfContextField(instance));
            sljit_emit_icall(m_compiler, SLJIT_CALL_REG_ARG, SLJIT_ARGS1(P, P), SLJIT_R2, 0);
            sljit_label* returnToLabel = sljit_emit_label(m_compiler);
            sljit_emit_return(m_compiler, SLJIT_MOV_P, SLJIT_R0, 0);

            m_context.trapBlocks.push_back(TrapBlock(sljit_emit_label(m_compiler), returnToLabel));
            ASSERT(m_context.trapBlocksStart == 0);
            m_context.trapBlocksStart = 1;
        }

        if (sljit_emit_atomic_load(m_compiler, SLJIT_MOV_U16 | SLJIT_ATOMIC_TEST, SLJIT_R0, SLJIT_R1) != SLJIT_ERR_UNSUPPORTED) {
            m_options |= JITCompiler::kHasShortAtomic;
        }
    }

#ifdef WALRUS_JITPERF
    const bool perfEnabled = PerfDump::instance().perfEnabled();
#if !defined(NDEBUG)
    if (perfEnabled) {
        uint32_t line = PerfDump::instance().dumpProlog(module(), moduleFunction());
        m_debugEntries.push_back(DebugEntry(sljit_emit_label(m_compiler), line));
    }
#endif /* !NDEBUG */
#endif /* WALRUS_JITPERF */

    emitProlog();

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
#if defined(WALRUS_JITPERF) && !defined(NDEBUG)
        if (perfEnabled) {
            uint32_t line = PerfDump::instance().dumpByteCode(item);
            if (item->info() & Instruction::kIsMergeCompare) {
                line = PerfDump::instance().dumpByteCode(item->next());
            }

            ASSERT(!m_debugEntries.empty());
            sljit_label* label = sljit_emit_label(m_compiler);

            // Label is the same, if no instructions are emitted.
            if (m_debugEntries.back().u.label == label) {
                m_debugEntries.back().line = line;
            } else {
                m_debugEntries.push_back(DebugEntry(label, line));
            }
        }
#endif /* WALRUS_JITPERF && !NDEBUG */

        if (item->isLabel()) {
            Label* label = item->asLabel();

            if (UNLIKELY(label->info() & Label::kHasCatchInfo)) {
                ASSERT(tryBlocks()[m_context.currentTryBlock].catchBlocks[0].u.handler == label);
                emitCatch(m_compiler, &m_context);
            }

            label->emit(m_compiler);

            if (UNLIKELY(label->info() & Label::kHasTryInfo)) {
                emitTry(&m_context, label);
            }
            continue;
        }

        switch (item->group()) {
        case Instruction::Immediate: {
            emitImmediate(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::DirectBranch: {
            emitDirectBranch(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::BrTable: {
            emitBrTable(m_compiler, item->asInstruction()->asBrTable());
            break;
        }
        case Instruction::Call: {
            emitCall(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Binary: {
            emitBinary(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::BinaryFloat: {
            emitFloatBinary(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Unary: {
            emitUnary(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::UnaryFloat: {
            emitFloatUnary(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Compare: {
            if (emitCompare(m_compiler, item->asInstruction())) {
                item = item->next();
            }
            break;
        }
        case Instruction::CompareFloat: {
            if (emitFloatCompare(m_compiler, item->asInstruction())) {
                item = item->next();
            }
            break;
        }
        case Instruction::Convert: {
            emitConvert(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::ConvertFloat: {
            emitConvertFloat(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Load: {
            emitLoad(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Store: {
            emitStore(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Table: {
            emitTable(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Memory: {
            emitMemory(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Move: {
            switch (item->asInstruction()->opcode()) {
            case ByteCode::MoveI32Opcode:
                emitMoveI32(m_compiler, item->asInstruction());
                break;
            case ByteCode::MoveI64Opcode:
                emitMoveI64(m_compiler, item->asInstruction());
                break;
            case ByteCode::MoveF32Opcode:
            case ByteCode::MoveF64Opcode:
                emitMoveFloat(m_compiler, item->asInstruction());
                break;
#ifdef HAS_SIMD
            case ByteCode::MoveV128Opcode:
                emitMoveV128(m_compiler, item->asInstruction());
                break;
#endif /* HAS_SIMD */
            default:
                ASSERT(item->asInstruction()->opcode() == ByteCode::I32ReinterpretF32Opcode
                       || item->asInstruction()->opcode() == ByteCode::I64ReinterpretF64Opcode
                       || item->asInstruction()->opcode() == ByteCode::F32ReinterpretI32Opcode
                       || item->asInstruction()->opcode() == ByteCode::F64ReinterpretI64Opcode);
                emitReinterpretOperation(m_compiler, item->asInstruction());
                break;
            }
            break;
        }
#ifdef HAS_SIMD
        case Instruction::ExtractLaneSIMD: {
            emitExtractLaneSIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::ReplaceLaneSIMD: {
            emitReplaceLaneSIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::SplatSIMD: {
            emitSplatSIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::BinarySIMD: {
            emitBinarySIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::UnarySIMD: {
            emitUnarySIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::UnaryCondSIMD: {
            if (emitUnaryCondSIMD(m_compiler, item->asInstruction())) {
                item = item->next();
            }
            break;
        }
        case Instruction::LoadLaneSIMD: {
            emitLoadLaneSIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::BitMaskSIMD: {
            emitBitMaskSIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::ShiftSIMD: {
            emitShiftSIMD(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::TernarySIMD: {
            emitTernarySIMD(m_compiler, item->asInstruction());
            break;
        }
#endif /* HAS_SIMD */
        case Instruction::StackInit: {
            emitStackInit(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCUnary: {
            emitGCUnary(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCCastGeneric: {
            emitGCCastGeneric(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCCastDefined: {
            emitGCCastDefined(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCArrayNew: {
            emitGCArrayNew(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCArrayOp: {
            emitGCArrayOp(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCArrayInitFromExt: {
            GCArrayInitFromExt(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCArrayAccess: {
            emitGCArrayAccess(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCStructNew: {
            emitGCStructNew(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::GCStructAccess: {
            emitGCStructAccess(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Atomic: {
            emitAtomic(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::AtomicFence: {
            emitAtomicFence(m_compiler);
            break;
        }
        case Instruction::AtomicWait: {
            emitAtomicWait(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::AtomicNotify: {
            emitAtomicNotify(m_compiler, item->asInstruction());
            break;
        }
        default: {
            switch (item->asInstruction()->opcode()) {
            case ByteCode::SelectOpcode: {
                emitSelect(m_compiler, item->asInstruction(), -1);
                break;
            }
#ifdef HAS_SIMD
            case ByteCode::I8X16ShuffleOpcode: {
                emitShuffleSIMD(m_compiler, item->asInstruction());
                break;
            }
#endif /* HAS_SIMD */
            case ByteCode::GlobalGet32Opcode: {
                emitGlobalGet32(m_compiler, item->asInstruction());
                break;
            }
            case ByteCode::GlobalGet64Opcode: {
                emitGlobalGet64(m_compiler, item->asInstruction());
                break;
            }
#ifdef HAS_SIMD
            case ByteCode::GlobalGet128Opcode: {
                emitGlobalGet128(m_compiler, item->asInstruction());
                break;
            }
#endif /* HAS_SIMD */
            case ByteCode::GlobalSet32Opcode: {
                emitGlobalSet32(m_compiler, item->asInstruction());
                break;
            }
            case ByteCode::GlobalSet64Opcode: {
                emitGlobalSet64(m_compiler, item->asInstruction());
                break;
            }
#ifdef HAS_SIMD
            case ByteCode::GlobalSet128Opcode: {
                emitGlobalSet128(m_compiler, item->asInstruction());
                break;
            }
#endif /* HAS_SIMD */
            case ByteCode::RefFuncOpcode: {
                emitRefFunc(m_compiler, item->asInstruction());
                break;
            }
            case ByteCode::RefAsNonNullOpcode: {
                emitRefAsNonNull(m_compiler, item->asInstruction());
                break;
            }
            case ByteCode::ElemDropOpcode: {
                emitElemDrop(m_compiler, item->asInstruction());
                break;
            }
            case ByteCode::DataDropOpcode: {
                emitDataDrop(m_compiler, item->asInstruction());
                break;
            }
            case ByteCode::ThrowOpcode: {
                emitThrow(m_compiler, item->asInstruction());
                break;
            }
            case ByteCode::UnreachableOpcode: {
                sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::UnreachableError);
                m_context.appendTrapJump(ExecutionContext::GenericTrap, sljit_emit_jump(m_compiler, SLJIT_JUMP));
                break;
            }
#if !defined(NDEBUG)
            case ByteCode::NopOpcode: {
                sljit_emit_op0(m_compiler, SLJIT_NOP);
                break;
            }
#endif /* !NDEBUG */
            case ByteCode::EndOpcode: {
                emitEnd(m_compiler, item->asInstruction());
                break;
            }
            default: {
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            }
            break;
        }
        }
    }

#if defined(WALRUS_JITPERF) && !defined(NDEBUG)
    if (perfEnabled) {
        uint32_t line = PerfDump::instance().dumpEpilog();
        m_debugEntries.push_back(DebugEntry(sljit_emit_label(m_compiler), line));
        m_debugEntries.push_back(DebugEntry());
    }
#endif /* WALRUS_JITPERF && !NDEBUG */

    emitEpilog();
    clear();
}

void JITCompiler::generateCode()
{
    ASSERT(m_context.nextTryBlock == tryBlocks().size());

    if (m_compiler == nullptr) {
        // All functions are imported.
        return;
    }

    if (m_brTableLabels != nullptr) {
        /* Reverse the chain. */
        sljit_read_only_buffer* prev = nullptr;
        sljit_read_only_buffer* current = &m_brTableLabels->header;

        do {
            sljit_read_only_buffer* next = current->next;

            current->next = prev;
            prev = current;
            current = next;
        } while (current != nullptr);

        sljit_emit_aligned_label(m_compiler, SLJIT_LABEL_ALIGN_W, prev);

        BranchTableLabels* brTable = reinterpret_cast<BranchTableLabels*>(prev);
        m_brTableLabels = brTable;

        do {
            sljit_set_label(brTable->jump, brTable->header.u.label);
            brTable = reinterpret_cast<BranchTableLabels*>(brTable->header.next);
        } while (brTable != nullptr);
    }

    void* code = sljit_generate_code(m_compiler, 0, nullptr);

#ifdef WALRUS_JITPERF
    const bool perfEnabled = PerfDump::instance().perfEnabled();
    if (perfEnabled) {
        sljit_uw funcStart = SLJIT_FUNC_UADDR(code);
        sljit_uw funcEnd = sljit_get_label_addr(m_functionList[0].exportEntryLabel);
        PerfDump::instance().dumpCodeLoad(funcStart, funcStart, (funcEnd - funcStart), "*entrypoint*", (uint8_t*)funcStart);
    }
#endif

    if (code != nullptr) {
        if (m_brTableLabels != nullptr) {
            BranchTableLabels* brTable = m_brTableLabels;
            sljit_sw executable_offset = sljit_get_executable_offset(m_compiler);

            do {
                sljit_uw addr = sljit_get_label_abs_addr(brTable->header.u.label);
                sljit_uw size = brTable->header.size;

                ASSERT(brTable->labels.size() * sizeof(sljit_sw) == size);
                sljit_uw* values = reinterpret_cast<sljit_uw*>(sljit_read_only_buffer_start_writing(addr, size, executable_offset));

                for (auto it : brTable->labels) {
                    *values++ = sljit_get_label_addr(it.jitLabel);
                }

                sljit_read_only_buffer_end_writing(addr, size, executable_offset);
                brTable = reinterpret_cast<BranchTableLabels*>(brTable->header.next);
            } while (brTable != nullptr);
        }

        JITModule* moduleDescriptor = module()->m_jitModule;

        if (moduleDescriptor == nullptr) {
            InstanceConstData* instanceConstData = new InstanceConstData(m_context.trapBlocks, tryBlocks());
            moduleDescriptor = new JITModule(instanceConstData, code);
            module()->m_jitModule = moduleDescriptor;
        } else {
            moduleDescriptor->m_instanceConstData->append(m_context.trapBlocks, tryBlocks());
            moduleDescriptor->m_codeBlocks.push_back(code);
        }

        for (auto it : m_functionList) {
            it.jitFunc->m_module = moduleDescriptor;

            if (!it.isExported) {
                it.jitFunc->m_exportEntry = nullptr;
                continue;
            }

            it.jitFunc->m_exportEntry = reinterpret_cast<void*>(sljit_get_label_addr(it.exportEntryLabel));

            if (it.branchTableSize > 0) {
                sljit_up* branchList = reinterpret_cast<sljit_up*>(it.jitFunc->m_constData);
                ASSERT(branchList != nullptr);

                sljit_up* end = branchList + it.branchTableSize;

                do {
                    *branchList = sljit_get_label_addr(reinterpret_cast<sljit_label*>(*branchList));
                    branchList++;
                } while (branchList < end);
            }
        }
    }

#ifdef WALRUS_JITPERF
    if (perfEnabled) {
#if !defined(NDEBUG)
        size_t size = m_debugEntries.size();

        for (size_t i = 0; i < size; i++) {
            if (m_debugEntries[i].line != 0) {
                m_debugEntries[i].u.address = sljit_get_label_addr(m_debugEntries[i].u.label);
            } else {
                m_debugEntries[i].u.address = 0;
            }
        }

        size_t debugEntryStart = 0;
#endif /* !NDEBUG */

        for (size_t i = 0; i < m_functionList.size(); i++) {
            size_t size = module()->numberOfFunctions();
            int functionIndex = 0;

            for (size_t i = 0; i < size; i++) {
                if (module()->function(i)->jitFunction() == m_functionList[i].jitFunc) {
                    functionIndex = static_cast<int>(i);
                    break;
                }
            }

            std::string name = "function" + std::to_string(functionIndex);
            for (auto exp : module()->exports()) {
                if (exp->exportType() != Walrus::ExportType::Function) {
                    continue;
                }
                if (module()->function(exp->itemIndex())->jitFunction() == m_functionList[i].jitFunc) {
                    name += "_" + exp->name();
                    break;
                }
            }

            sljit_uw funcStart = sljit_get_label_addr(m_functionList[i].exportEntryLabel);
            sljit_uw funcEnd;

#if !defined(NDEBUG)
            debugEntryStart = PerfDump::instance().dumpDebugInfo(m_debugEntries, debugEntryStart, funcStart);
#endif /* !NDEBUG */

            if (i < m_functionList.size() - 1) {
                funcEnd = sljit_get_label_addr(m_functionList[i + 1].exportEntryLabel);
            } else {
                funcEnd = SLJIT_FUNC_UADDR(code) + sljit_get_generated_code_size(m_compiler);
            }

            PerfDump::instance().dumpCodeLoad(funcStart, funcStart, (funcEnd - funcStart), name, (uint8_t*)funcStart);
        }
    }
#endif
    sljit_free_compiler(m_compiler);
}

void JITCompiler::clear()
{
    InstructionListItem* item = m_first;

    m_first = nullptr;
    m_last = nullptr;
    m_branchTableSize = 0;
    m_stackTmpSize = 0;
#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
    m_context.shuffleOffset = 0;
#endif /* SLJIT_CONFIG_X86 */

    while (item != nullptr) {
        InstructionListItem* next = item->next();

        if (item->isLabel()) {
            Label* label = item->asLabel();

            if (label->info() & Label::kHasJumpList) {
                ASSERT(!(label->info() & Label::kHasLabelData));
                delete label->m_jumpList;
            }
        }

        item->deleteObject();
        item = next;
    }

    m_context.trapJumps.clear();
}

void JITCompiler::emitProlog()
{
    FunctionList& func = m_functionList.back();

    if (func.isExported) {
        func.exportEntryLabel = sljit_emit_label(m_compiler);
    }

    sljit_s32 options = SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2);
#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
    options |= SLJIT_ENTER_USE_VEX;
#endif /* !SLJIT_CONFIG_X86 */

#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
    ASSERT(m_stackTmpSize <= 32);
#else /* !SLJIT_CONFIG_ARM_32 */
    ASSERT(m_stackTmpSize <= 16);
#endif /* SLJIT_CONFIG_ARM_32 */

    sljit_s32 scratches = SLJIT_NUMBER_OF_SCRATCH_REGISTERS | SLJIT_ENTER_FLOAT(SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS) | SLJIT_ENTER_VECTOR(SLJIT_NUMBER_OF_SCRATCH_VECTOR_REGISTERS);
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    sljit_s32 saveds = (m_savedIntegerRegCount + 2) | SLJIT_ENTER_FLOAT(m_savedFloatRegCount) | SLJIT_ENTER_VECTOR(m_savedVectorRegCount);
#else /* !SLJIT_SEPARATE_VECTOR_REGISTERS */
    sljit_s32 saveds = (m_savedIntegerRegCount + 2) | SLJIT_ENTER_FLOAT(m_savedFloatRegCount) | SLJIT_ENTER_VECTOR(m_savedFloatRegCount);
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */
    sljit_emit_enter(m_compiler, options, SLJIT_ARGS1(P, P_R), scratches, saveds, m_context.stackTmpStart + m_stackTmpSize);

    sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), kContextOffset, SLJIT_R0, 0);

    m_context.branchTableOffset = 0;
    size_t size = func.branchTableSize * sizeof(sljit_up);
#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
    size += m_context.shuffleOffset;
#endif /* SLJIT_CONFIG_X86 */

    if (size > 0) {
        void* constData = malloc(size);

        func.jitFunc->m_constData = constData;
        m_context.branchTableOffset = reinterpret_cast<uintptr_t>(constData);

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
        // Requires 16 byte alignment.
        m_context.shuffleOffset = (reinterpret_cast<uintptr_t>(constData) + size - m_context.shuffleOffset + 0xf) & ~(uintptr_t)0xf;
#endif /* SLJIT_CONFIG_X86 */
    }

    ASSERT(m_lastBrTableLabels == nullptr);
}

void JITCompiler::emitEpilog()
{
    FunctionList& func = m_functionList.back();

    ASSERT(m_context.branchTableOffset == reinterpret_cast<sljit_uw>(func.jitFunc->m_constData) + func.branchTableSize * sizeof(sljit_sw));
    ASSERT(m_context.currentTryBlock == InstanceConstData::globalTryBlock);

    if (!m_context.earlyReturns.empty()) {
        sljit_label* label = sljit_emit_label(m_compiler);

        for (auto it : m_context.earlyReturns) {
            sljit_set_label(it, label);
        }

        m_context.earlyReturns.clear();
    }

    sljit_emit_return(m_compiler, SLJIT_MOV_P, SLJIT_R0, 0);

    m_context.emitSlowCases(m_compiler);

    std::vector<TrapJump>& trapJumps = m_context.trapJumps;
    // The actual maximum is smaller, but the extra stack consumption is small.
    sljit_jump* jumps[ExecutionContext::GenericTrap];
    size_t jumpCount = 0;
    size_t trapJumpIndex;
    sljit_label* lastLabel = nullptr;
    uint32_t lastJumpType = ExecutionContext::ErrorCodesEnd;

    std::sort(trapJumps.begin(), trapJumps.end());

    for (trapJumpIndex = 0; trapJumpIndex < trapJumps.size(); trapJumpIndex++) {
        TrapJump& it = trapJumps[trapJumpIndex];

        if (it.jumpType == lastJumpType) {
            sljit_set_label(it.jump, lastLabel);
            continue;
        }

        if (it.jumpType >= ExecutionContext::GenericTrap) {
            break;
        }

        if (lastJumpType != ExecutionContext::ErrorCodesEnd) {
            jumps[jumpCount++] = sljit_emit_jump(m_compiler, SLJIT_JUMP);
        }

        lastJumpType = it.jumpType;
        lastLabel = sljit_emit_label(m_compiler);

        sljit_set_label(it.jump, lastLabel);
        if (it.jumpType == ExecutionContext::AllocationError) {
            sljit_emit_op2(m_compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(ExecutionContext::AllocationError));
        } else {
            sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(it.jumpType));
        }
    }

    if (trapJumpIndex > 0 || (trapJumps.size() > 0 && trapJumps[0].jumpType == ExecutionContext::GenericTrap)) {
        lastLabel = sljit_emit_label(m_compiler);
        sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_SP), kContextOffset);
        sljit_emit_op1(m_compiler, SLJIT_MOV_U32, SLJIT_MEM1(SLJIT_R1), OffsetOfContextField(error), SLJIT_R0, 0);

        for (size_t i = 0; i < jumpCount; i++) {
            sljit_set_label(jumps[i], lastLabel);
        }

        while (trapJumpIndex < trapJumps.size()) {
            if (trapJumps[trapJumpIndex].jumpType != ExecutionContext::GenericTrap) {
                break;
            }

            sljit_set_label(trapJumps[trapJumpIndex++].jump, lastLabel);
        }
    }

    if (trapJumps.size() > 0 || m_tryBlockStart < m_tryBlocks.size()) {
        lastLabel = sljit_emit_label(m_compiler);

        sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_SP), kContextOffset);
        sljit_emit_op_dst(m_compiler, SLJIT_GET_RETURN_ADDRESS, SLJIT_R1, 0);
        sljit_emit_icall(m_compiler, SLJIT_CALL, SLJIT_ARGS2(W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, getTrapHandler));

        sljit_emit_return_to(m_compiler, SLJIT_R0, 0);

        while (trapJumpIndex < trapJumps.size()) {
            ASSERT(trapJumps[trapJumpIndex].jumpType == ExecutionContext::ReturnToLabel);
            sljit_set_label(trapJumps[trapJumpIndex++].jump, lastLabel);
        }
    }

    if (m_lastBrTableLabels != nullptr) {
        BranchTableLabels* brTable = m_brTableLabels;

        while (true) {
            size_t size = brTable->labels.size();

            for (size_t i = 0; i < size; i++) {
                brTable->labels[i].jitLabel = brTable->labels[i].label->label();
            }

            if (brTable == m_lastBrTableLabels) {
                break;
            }

            brTable = reinterpret_cast<BranchTableLabels*>(brTable->header.next);
        }

        m_lastBrTableLabels = nullptr;
    }

    if (func.branchTableSize > 0) {
        sljit_label** branchList = reinterpret_cast<sljit_label**>(func.jitFunc->m_constData);
        ASSERT(branchList != nullptr);

        sljit_label** end = branchList + func.branchTableSize;

        do {
            *branchList = reinterpret_cast<Label*>(*branchList)->m_label;
            branchList++;
        } while (branchList < end);
    }

    if (lastLabel == nullptr) {
        ASSERT(m_context.trapBlocksStart == m_context.trapBlocks.size() && m_tryBlockStart == m_tryBlocks.size());
        return;
    }

    sljit_label* endLabel = sljit_emit_label(m_compiler);
    std::vector<TrapBlock>& trapBlocks = m_context.trapBlocks;

    size_t end = trapBlocks.size();
    for (size_t i = m_context.trapBlocksStart; i < end; i++) {
        size_t tryBlockId = trapBlocks[i].u.tryBlockId;

        if (tryBlockId == InstanceConstData::globalTryBlock) {
            trapBlocks[i].u.handlerLabel = endLabel;
        } else {
            trapBlocks[i].u.handlerLabel = tryBlocks()[tryBlockId].findHandlerLabel;
        }
    }

    m_context.trapBlocksStart = end + 1;
    m_context.trapBlocks.push_back(TrapBlock(endLabel, lastLabel));

    end = m_tryBlocks.size();
    for (size_t i = m_tryBlockStart; i < end; i++) {
        ASSERT(m_tryBlocks[i].returnToLabel == nullptr);
        m_tryBlocks[i].returnToLabel = lastLabel;

        std::vector<TryBlock::CatchBlock>& catchBlocks = m_tryBlocks[i].catchBlocks;

        for (size_t j = 0; j < catchBlocks.size(); j++) {
            catchBlocks[j].u.handlerLabel = catchBlocks[j].u.handler->label();
        }
    }
}

} // namespace Walrus

#endif // WALRUS_ENABLE_JIT
