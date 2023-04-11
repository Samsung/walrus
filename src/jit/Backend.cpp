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

#include "Walrus.h"

#include "runtime/JITExec.h"
#include "jit/Compiler.h"

#include <math.h>
#include <map>

// Inlined platform independent assembler backend.
#define SLJIT_CONFIG_AUTO 1
#define SLJIT_CONFIG_STATIC 1
#define SLJIT_VERBOSE 0

#if defined(NDEBUG)
#define SLJIT_DEBUG 0
#else
#define SLJIT_DEBUG 1
#endif

extern "C" {
#include "../../third_party/sljit/sljit_src/sljitLir.c"
}

#if defined SLJIT_CONFIG_UNSUPPORTED && SLJIT_CONFIG_UNSUPPORTED
#error Unsupported architecture
#endif

#define OffsetOfContextField(field) \
    (static_cast<sljit_sw>(offsetof(ExecutionContext, field)))

#if !(defined SLJIT_INDIRECT_CALL && SLJIT_INDIRECT_CALL)
#define GET_FUNC_ADDR(type, func) (reinterpret_cast<type>(func))
#else
#define GET_FUNC_ADDR(type, func) (*reinterpret_cast<type*>(func))
#endif

namespace Walrus {

static const uint8_t kContextReg = SLJIT_S0;
static const uint8_t kFrameReg = SLJIT_S1;

struct JITArg {
    sljit_s32 arg;
    sljit_sw argw;
};

struct TrapBlock {
    TrapBlock(sljit_label* endLabel, sljit_label* handlerLabel)
        : endLabel(endLabel)
        , handlerLabel(handlerLabel)
    {
    }

    sljit_label* endLabel;
    sljit_label* handlerLabel;
};

class SlowCase {
public:
    enum class Type {
        SignedDivide,
        SignedDivide32,
        SignedModulo,
        SignedModulo32,
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

struct CompileContext {
    CompileContext(JITCompiler* compiler)
        : compiler(compiler)
        , trapLabel(nullptr)
    {
    }

    static CompileContext* get(sljit_compiler* compiler)
    {
        void* context = sljit_get_allocator_data(compiler);
        return reinterpret_cast<CompileContext*>(context);
    }

    void add(SlowCase* slowCase) { slowCases.push_back(slowCase); }
    void emitSlowCases(sljit_compiler* compiler);

    JITCompiler* compiler;
    sljit_label* trapLabel;
    sljit_label* returnToLabel;
    sljit_uw branchTableOffset;
    std::vector<TrapBlock> trapBlocks;
    std::vector<SlowCase*> slowCases;
    std::vector<sljit_jump*> earlyReturns;
};

class TrapHandlerList {
public:
    TrapHandlerList(std::vector<TrapBlock>& trapBlocks)
    {
        for (auto it : trapBlocks) {
            m_trapList.push_back(sljit_get_label_addr(it.endLabel));
            m_trapList.push_back(sljit_get_label_addr(it.handlerLabel));
        }
    }

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
};

static sljit_uw SLJIT_FUNC getTrapHandler(ExecutionContext* context, sljit_uw returnAddr)
{
    return context->currentInstanceConstData->trapHandlers->find(returnAddr);
}

static void operandToArg(Operand* operand, JITArg& arg)
{
    if (operand->item == nullptr || operand->item->group() != Instruction::Immediate) {
        arg.arg = SLJIT_MEM1(kFrameReg);
        arg.argw = static_cast<sljit_sw>(operand->offset << 2);
        return;
    }

    arg.arg = SLJIT_IMM;

    Instruction* instr = operand->item->asInstruction();

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    ASSERT(instr->opcode() == Const32Opcode);

    arg.argw = static_cast<sljit_s32>(reinterpret_cast<Const32*>(instr->byteCode())->value());
#else /* !SLJIT_32BIT_ARCHITECTURE */
    if (instr->opcode() == Const32Opcode) {
        arg.argw = static_cast<sljit_s32>(reinterpret_cast<Const32*>(instr->byteCode())->value());
        return;
    }

    arg.argw = static_cast<sljit_sw>(reinterpret_cast<Const64*>(instr->byteCode())->value());
#endif /* SLJIT_32BIT_ARCHITECTURE */
}

#define GET_TARGET_REG(arg, default_reg) \
    (((arg)&SLJIT_MEM) ? (default_reg) : (arg))
#define IS_SOURCE_REG(arg) (!((arg) & (SLJIT_MEM | SLJIT_IMM)))
#define GET_SOURCE_REG(arg, default_reg) \
    (IS_SOURCE_REG(arg) ? (arg) : (default_reg))
#define MOVE_TO_REG(compiler, mov_op, target_reg, arg, argw)              \
    if ((target_reg) != (arg)) {                                          \
        sljit_emit_op1(compiler, mov_op, (target_reg), 0, (arg), (argw)); \
    }
#define MOVE_FROM_REG(compiler, mov_op, arg, argw, source_reg)            \
    if ((source_reg) != (arg)) {                                          \
        sljit_emit_op1(compiler, mov_op, (arg), (argw), (source_reg), 0); \
    }

#include "FloatMathInl.h"

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
#include "IntMath32Inl.h"
#else /* !SLJIT_32BIT_ARCHITECTURE */
#include "IntMath64Inl.h"
#endif /* SLJIT_32BIT_ARCHITECTURE */

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
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::DivideByZeroError);

        sljit_s32 current_flags = SLJIT_CURRENT_FLAGS_SUB | SLJIT_CURRENT_FLAGS_COMPARE | SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        if (m_type == Type::SignedDivide32) {
            current_flags |= SLJIT_32;
        }
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_set_current_flags(compiler, current_flags);
        /* Division by zero. */
        sljit_set_label(sljit_emit_jump(compiler, SLJIT_EQUAL), context->trapLabel);

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

        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::IntegerOverflowError);
        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->trapLabel);
        return;
    }
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case Type::SignedModulo32:
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case Type::SignedModulo: {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::DivideByZeroError);

        sljit_s32 current_flags = SLJIT_CURRENT_FLAGS_SUB | SLJIT_CURRENT_FLAGS_COMPARE | SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        if (m_type == Type::SignedModulo32) {
            current_flags |= SLJIT_32;
        }
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_set_current_flags(compiler, current_flags);
        /* Division by zero. */
        sljit_set_label(sljit_emit_jump(compiler, SLJIT_EQUAL), context->trapLabel);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op1(compiler, (m_type == Type::SignedModulo32) ? SLJIT_MOV32 : SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, 0);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, 0);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), m_resumeLabel);
        return;
    }
    default: {
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    }
}

static void emitImmediate(sljit_compiler* compiler, Instruction* instr)
{
    Operand* result = instr->operands();

    ASSERT(instr->group() == Instruction::Immediate);

    if (!(instr->info() & Instruction::kKeepInstruction)) {
        return;
    }

    emitStoreImmediate(compiler, result, instr);
}

static void emitEnd(sljit_compiler* compiler, Instruction* instr)
{
    Operand* param = instr->params();
    Operand* paramEnd = param + instr->paramCount();

    while (param < paramEnd) {
        if (param->item != nullptr && param->item->group() == Instruction::Immediate && !(param->item->info() & Instruction::kKeepInstruction)) {
            emitStoreImmediate(compiler, param, param->item->asInstruction());
        }

        param++;
    }

    End* end = reinterpret_cast<End*>(instr->byteCode());
    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(end->resultOffsets()));

    if (instr->info() & Instruction::kEarlyReturn) {
        CompileContext* context = CompileContext::get(compiler);
        context->earlyReturns.push_back(sljit_emit_jump(compiler, SLJIT_JUMP));
    }
}

static void emitDirectBranch(sljit_compiler* compiler, Instruction* instr)
{
    sljit_jump* jump;

    if (instr->opcode() == JumpOpcode) {
        jump = sljit_emit_jump(compiler, SLJIT_JUMP);

        CompileContext::get(compiler)->emitSlowCases(compiler);
    } else {
        JITArg src;

        operandToArg(instr->operands(), src);

        sljit_s32 type = (instr->opcode() == JumpIfTrueOpcode) ? SLJIT_NOT_EQUAL : SLJIT_EQUAL;

        jump = sljit_emit_cmp(compiler, type | SLJIT_32, src.arg, src.argw, SLJIT_IMM, 0);
    }

    instr->asExtended()->value().targetLabel->jumpFrom(jump);
}

static void emitBrTable(sljit_compiler* compiler, BrTableInstruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    size_t targetLabelCount = instr->targetLabelCount() - 1;
    Label** label = instr->targetLabels();
    Label** end = label + targetLabelCount;
    JITArg src;

    operandToArg(instr->operands(), src);

    sljit_s32 offsetReg = GET_SOURCE_REG(src.arg, SLJIT_R0);
    MOVE_TO_REG(compiler, SLJIT_MOV32, offsetReg, src.arg, src.argw);

    if (sljit_has_cpu_feature(SLJIT_HAS_CMOV)) {
        sljit_emit_op2u(compiler, SLJIT_SUB32 | SLJIT_SET_GREATER_EQUAL, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(targetLabelCount));
        sljit_emit_select(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, SLJIT_R0, SLJIT_IMM, static_cast<sljit_sw>(targetLabelCount), offsetReg);

        offsetReg = SLJIT_R0;
        end++;
    } else {
        sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(targetLabelCount));
        (*end)->jumpFrom(jump);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, static_cast<sljit_sw>(context->branchTableOffset));
    sljit_emit_ijump(compiler, SLJIT_JUMP, SLJIT_MEM2(SLJIT_R1, offsetReg), SLJIT_WORD_SHIFT);

    sljit_uw* target = reinterpret_cast<sljit_uw*>(context->branchTableOffset);

    while (label < end) {
        *target++ = reinterpret_cast<sljit_uw>(*label);
        label++;
    }

    context->branchTableOffset = reinterpret_cast<sljit_uw>(target);
}

static void emitMove32(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg src;
    JITArg dst;

    operandToArg(operands, src);
    operandToArg(operands + 1, dst);

    sljit_emit_op1(compiler, SLJIT_MOV32, dst.arg, dst.argw, src.arg, src.argw);
}

JITModule::~JITModule()
{
    delete m_instanceConstData->trapHandlers;
    free(m_instanceConstData);
    sljit_free_code(m_moduleStart, nullptr);
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

void JITCompiler::computeOptions()
{
    if (sljit_has_cpu_feature(SLJIT_HAS_CMOV)) {
        m_options |= JITCompiler::kHasCondMov;
    }
}

JITModule* JITCompiler::compile()
{
    CompileContext compileContext(this);
    m_compiler = sljit_create_compiler(reinterpret_cast<void*>(&compileContext), nullptr);

    // Follows the declaration of FunctionDescriptor::ExternalDecl().
    // Context stored in SLJIT_S0 (kContextReg)
    // Frame stored in SLJIT_S1 (kFrameReg)
    sljit_emit_enter(m_compiler, 0, SLJIT_ARGS3(P, P, P, P_R), 3, 2, 0, 0, 0);
    sljit_emit_icall(m_compiler, SLJIT_CALL_REG_ARG, SLJIT_ARGS0(P), SLJIT_R2, 0);
    sljit_label* returnToLabel = sljit_emit_label(m_compiler);
    sljit_emit_return(m_compiler, SLJIT_MOV_P, SLJIT_R0, 0);

    compileContext.trapBlocks.push_back(TrapBlock(sljit_emit_label(m_compiler), returnToLabel));

    size_t currentFunction = 0;

    for (InstructionListItem* item = m_functionListFirst; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();

            if (!(label->info() & Label::kNewFunction)) {
                label->emit(m_compiler);
            } else {
                if (label->prev() != nullptr) {
                    emitEpilog(currentFunction, compileContext);
                    currentFunction++;
                }
                emitProlog(currentFunction, compileContext);
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
        case Instruction::Move: {
            if (item->asInstruction()->opcode() == Move32Opcode) {
                emitMove32(m_compiler, item->asInstruction());
            } else {
                emitMove64(m_compiler, item->asInstruction());
            }
            break;
        }
        default: {
            switch (item->asInstruction()->opcode()) {
            case DropOpcode:
            case ReturnOpcode: {
                break;
            }
            case SelectOpcode: {
                emitSelect(m_compiler, item->asInstruction(), -1);
                break;
            }
            case NopOpcode: {
                sljit_emit_op0(m_compiler, SLJIT_NOP);
                break;
            }
            case UnreachableOpcode: {
                sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::UnreachableError);
                sljit_set_label(sljit_emit_jump(m_compiler, SLJIT_JUMP), compileContext.trapLabel);
                break;
            }
            case EndOpcode: {
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

    emitEpilog(currentFunction, compileContext);

    void* code = sljit_generate_code(m_compiler);
    JITModule* moduleDescriptor = nullptr;

    if (code != nullptr) {
        size_t size = sizeof(InstanceConstData);
        InstanceConstData* instanceConstData = reinterpret_cast<InstanceConstData*>(malloc(size));

        instanceConstData->trapHandlers = new TrapHandlerList(compileContext.trapBlocks);
        moduleDescriptor = new JITModule(instanceConstData, code);

        for (auto it : m_functionList) {
            it.jitFunc->m_module = moduleDescriptor;

            if (!it.isExported) {
                it.jitFunc->m_exportEntry = nullptr;
                continue;
            }

            it.jitFunc->m_exportEntry = reinterpret_cast<void*>(sljit_get_label_addr(it.exportEntryLabel));

            if (it.branchTableSize > 0) {
                sljit_uw* branchList = reinterpret_cast<sljit_uw*>(it.jitFunc->m_branchList);
                ASSERT(branchList != nullptr);

                sljit_uw* end = branchList + it.branchTableSize;

                do {
                    *branchList = sljit_get_label_addr(reinterpret_cast<Label*>(*branchList)->m_label);
                    branchList++;
                } while (branchList < end);
            }
        }
    }

    sljit_free_compiler(m_compiler);
    return moduleDescriptor;
}

void JITCompiler::releaseFunctionList()
{
    InstructionListItem* item = m_functionListFirst;

    while (item != nullptr) {
        if (item->isLabel()) {
            Label* label = item->asLabel();

            ASSERT((label->info() & Label::kNewFunction) || label->branches().size() > 0);

            if (label->info() & Label::kHasJumpList) {
                ASSERT(!(label->info() & Label::kHasLabelData));
                delete label->m_jumpList;
            }
        }

        InstructionListItem* next = item->next();

        ASSERT(next == nullptr || next->m_prev == item);

        delete item;
        item = next;
    }

    m_functionList.clear();
}

void JITCompiler::emitProlog(size_t index, CompileContext& context)
{
    FunctionList& func = m_functionList[index];
    sljit_s32 savedRegCount = 4;

    sljit_set_context(m_compiler, SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2), SLJIT_ARGS0(P),
                      SLJIT_NUMBER_OF_SCRATCH_REGISTERS, savedRegCount,
                      SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, 0, sizeof(ExecutionContext::CallFrame));

    context.trapLabel = sljit_emit_label(m_compiler);
    sljit_emit_op1(m_compiler, SLJIT_MOV_U32, SLJIT_MEM1(kContextReg), OffsetOfContextField(error), SLJIT_R2, 0);

    context.returnToLabel = sljit_emit_label(m_compiler);

    sljit_emit_op_dst(m_compiler, SLJIT_GET_RETURN_ADDRESS, SLJIT_R1, 0);
    sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R0, 0, kContextReg, 0);
    sljit_emit_icall(m_compiler, SLJIT_CALL, SLJIT_ARGS2(W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, getTrapHandler));
    sljit_emit_return_to(m_compiler, SLJIT_R0, 0);

    if (func.isExported) {
        func.exportEntryLabel = sljit_emit_label(m_compiler);
    }

    func.entryLabel->emit(m_compiler);

    sljit_emit_enter(m_compiler, SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2), SLJIT_ARGS0(P),
                     SLJIT_NUMBER_OF_SCRATCH_REGISTERS, savedRegCount,
                     SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, 0, sizeof(ExecutionContext::CallFrame));

    // Setup new frame.
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(lastFrame));

    sljit_get_local_base(m_compiler, SLJIT_R1, 0, 0);
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(kContextReg), OffsetOfContextField(lastFrame), SLJIT_R1, 0);
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), offsetof(ExecutionContext::CallFrame, frameStart), kFrameReg, 0);
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), offsetof(ExecutionContext::CallFrame, prevFrame), SLJIT_R0, 0);

    context.branchTableOffset = 0;

    if (func.branchTableSize > 0) {
        void* branchList = malloc(func.branchTableSize * sizeof(sljit_uw));

        func.jitFunc->m_branchList = branchList;
        context.branchTableOffset = reinterpret_cast<sljit_uw>(branchList);
    }
}

sljit_label* JITCompiler::emitEpilog(size_t index, CompileContext& context)
{
    FunctionList& func = m_functionList[index];

    ASSERT(context.branchTableOffset == reinterpret_cast<sljit_uw>(func.jitFunc->m_branchList) + func.branchTableSize * sizeof(sljit_sw));

    if (!context.earlyReturns.empty()) {
        sljit_label* label = sljit_emit_label(m_compiler);

        for (auto it : context.earlyReturns) {
            sljit_set_label(it, label);
        }

        context.earlyReturns.clear();
    }

    // Restore previous frame.
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_SP), offsetof(ExecutionContext::CallFrame, prevFrame));
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(kContextReg), OffsetOfContextField(lastFrame), SLJIT_R1, 0);

    sljit_emit_return(m_compiler, SLJIT_MOV_P, SLJIT_R0, 0);

    context.emitSlowCases(m_compiler);

    sljit_label* endLabel = sljit_emit_label(m_compiler);
    context.trapBlocks.push_back(TrapBlock(endLabel, context.returnToLabel));
    return endLabel;
}

} // namespace Walrus
