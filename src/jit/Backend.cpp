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
    (static_cast<sljit_sw>(offsetof(ExecutionContext, field)) - static_cast<sljit_sw>(sizeof(ExecutionContext)))

#if !(defined SLJIT_INDIRECT_CALL && SLJIT_INDIRECT_CALL)
#define GET_FUNC_ADDR(type, func) (reinterpret_cast<type>(func))
#else
#define GET_FUNC_ADDR(type, func) (*reinterpret_cast<type*>(func))
#endif

using namespace wabt;

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
    };

    SlowCase(Type type,
             sljit_jump* jump_from,
             sljit_label* resume_label,
             Instruction* instr)
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
        , frameSize(0)
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
    Index frameSize;
    sljit_label* trapLabel;
    sljit_label* returnToLabel;
    std::vector<TrapBlock> trapBlocks;
    std::vector<SlowCase*> slowCases;
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

static sljit_uw SLJIT_FUNC getTrapHandler(ExecutionContext* context,
                                          sljit_uw returnAddr)
{
    return context->currentInstanceConstData->trapHandlers->find(returnAddr);
}

static void operandToArg(Operand* operand, JITArg& arg)
{
    switch (operand->location.type) {
    case Operand::Stack: {
        arg.arg = SLJIT_MEM1(kFrameReg);
        arg.argw = static_cast<sljit_sw>(operand->value);
        break;
    }
    case Operand::Register: {
        arg.arg = static_cast<sljit_s32>(operand->value);
        arg.argw = 0;
        break;
    }
    default: {
        assert(operand->location.type == Operand::Immediate);
        arg.arg = SLJIT_IMM;
        assert(operand->item->group() == Instruction::Immediate);

        Instruction* instr = operand->item->asInstruction();

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        assert((operand->location.valueInfo & LocationInfo::kSizeMask) == 1);
        arg.argw = static_cast<sljit_s32>(instr->value().value32);
#else /* !SLJIT_32BIT_ARCHITECTURE */
        if ((operand->location.valueInfo & LocationInfo::kSizeMask) == 1) {
            arg.argw = static_cast<sljit_s32>(instr->value().value32);
        } else {
            arg.argw = static_cast<sljit_sw>(instr->value().value64);
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */
        break;
    }
    }
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
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                       ExecutionContext::DivideByZeroError);

        sljit_s32 current_flags = SLJIT_CURRENT_FLAGS_SUB | SLJIT_CURRENT_FLAGS_COMPARE | SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        if (m_type == Type::SignedDivide32) {
            current_flags |= SLJIT_32;
        }
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_set_current_flags(compiler, current_flags);
        /* Division by zero. */
        sljit_jump* jump = sljit_emit_jump(compiler, SLJIT_EQUAL);
        sljit_set_label(jump, context->trapLabel);

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

        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                       ExecutionContext::IntegerOverflowError);

        jump = sljit_emit_jump(compiler, SLJIT_JUMP);
        sljit_set_label(jump, context->trapLabel);
        return;
    }
    default: {
        WABT_UNREACHABLE;
        break;
    }
    }
}

static void emitDirectBranch(sljit_compiler* compiler, Instruction* instr)
{
    sljit_jump* jump;

    if (instr->opcode() == BrOpcode) {
        jump = sljit_emit_jump(compiler, SLJIT_JUMP);

        CompileContext::get(compiler)->emitSlowCases(compiler);
    } else {
        Operand* result = instr->operands();
        JITArg src;

        operandToArg(result, src);

        sljit_s32 type = (instr->opcode() == BrIfOpcode) ? SLJIT_NOT_EQUAL : SLJIT_EQUAL;

        if ((result->location.valueInfo & LocationInfo::kSizeMask) == 1) {
            type |= SLJIT_32;
        }

        jump = sljit_emit_cmp(compiler, type, src.arg, src.argw, SLJIT_IMM, 0);
    }

    instr->value().targetLabel->jumpFrom(jump);
}

class MoveContext {
public:
    MoveContext(sljit_compiler* compiler);

    void move(ValueInfo value_info, JITArg from, JITArg to);
    void done();

private:
    static const int m_delaySize = 2;

    void reset();

    sljit_compiler* m_compiler;
    int m_index;
    sljit_s32 m_reg[m_delaySize];
    sljit_s32 m_op[m_delaySize];
    JITArg m_target[m_delaySize];
};

MoveContext::MoveContext(sljit_compiler* compiler)
    : m_compiler(compiler)
{
    reset();
}

void MoveContext::move(ValueInfo value_info, JITArg from, JITArg to)
{
    sljit_s32 op;

    if (value_info & LocationInfo::kFloat) {
        op = (value_info & LocationInfo::kEightByteSize) ? SLJIT_MOV_F64
                                                         : SLJIT_MOV_F32;
    } else {
        op = (value_info & LocationInfo::kEightByteSize) ? SLJIT_MOV : SLJIT_MOV32;
    }

    if (!(from.arg & SLJIT_MEM) || !(to.arg & SLJIT_MEM)) {
        if (value_info & LocationInfo::kFloat) {
            sljit_emit_fop1(m_compiler, op, to.arg, to.argw, from.arg, from.argw);
            return;
        }

        sljit_emit_op1(m_compiler, op, to.arg, to.argw, from.arg, from.argw);
        return;
    }

    if (m_reg[m_index] != 0) {
        if ((m_op[m_index] | SLJIT_32) == SLJIT_MOV_F32) {
            sljit_emit_fop1(m_compiler, m_op[m_index], m_target[m_index].arg,
                            m_target[m_index].argw, m_reg[m_index], 0);
        } else {
            sljit_emit_op1(m_compiler, m_op[m_index], m_target[m_index].arg,
                           m_target[m_index].argw, m_reg[m_index], 0);
        }
    }

    if (value_info & LocationInfo::kFloat) {
        m_op[m_index] = op;
        m_reg[m_index] = (m_index == 0) ? SLJIT_FR0 : SLJIT_FR1;

        sljit_emit_fop1(m_compiler, op, m_reg[m_index], 0, from.arg, from.argw);
    } else {
        m_op[m_index] = op;
        m_reg[m_index] = (m_index == 0) ? SLJIT_R0 : SLJIT_R1;

        sljit_emit_op1(m_compiler, op, m_reg[m_index], 0, from.arg, from.argw);
    }

    m_target[m_index] = to;
    m_index = 1 - m_index;
}

void MoveContext::done()
{
    for (int i = 0; i < m_delaySize; i++) {
        if (m_reg[m_index] != 0) {
            if ((m_op[m_index] | SLJIT_32) == SLJIT_MOV_F32) {
                sljit_emit_fop1(m_compiler, m_op[m_index], m_target[m_index].arg,
                                m_target[m_index].argw, m_reg[m_index], 0);
            } else {
                sljit_emit_op1(m_compiler, m_op[m_index], m_target[m_index].arg,
                               m_target[m_index].argw, m_reg[m_index], 0);
            }
        }

        m_index = 1 - m_index;
    }

    reset();
}

void MoveContext::reset()
{
    m_index = 0;
    m_reg[0] = 0;
    m_reg[1] = 0;
}

static void emitCall(sljit_compiler* compiler, CallInstruction* call_instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Operand* operand = call_instr->operands();
    Operand* operandEnd = operand + call_instr->paramCount();
    MoveContext moveContext(compiler);
    JITArg from, to;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    JITArgPair arg64;
#endif /* SLJIT_32BIT_ARCHITECTURE */

    LocalsAllocator localsAllocator(call_instr->paramStart());

    while (operand < operandEnd) {
        localsAllocator.allocate(operand->location.valueInfo);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (!(operand->location.valueInfo & LocationInfo::kFloat) && (operand->location.valueInfo & LocationInfo::kSizeMask) == 2) {
            operandToArgPair(operand, arg64);

            from.arg = arg64.arg1;
            from.argw = arg64.arg1w;
            to.arg = SLJIT_MEM1(kFrameReg);
            to.argw = static_cast<sljit_sw>(localsAllocator.last().value);

            if (from.arg != to.arg || from.argw != to.argw) {
                moveContext.move(operand->location.valueInfo, from, to);
            }

            from.arg = arg64.arg2;
            from.argw = arg64.arg2w;
            to.argw += sizeof(sljit_sw);

            if (from.arg != to.arg || from.argw != to.argw) {
                moveContext.move(operand->location.valueInfo, from, to);
            }

            operand++;
            continue;
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        operandToArg(operand, from);
        to.arg = SLJIT_MEM1(kFrameReg);
        to.argw = static_cast<sljit_sw>(localsAllocator.last().value);

        if (from.arg != to.arg || from.argw != to.argw) {
            moveContext.move(operand->location.valueInfo, from, to);
        }

        operand++;
    }

    moveContext.done();

    if (!(call_instr->info() & CallInstruction::kIndirect)) {
        sljit_jump* jump = sljit_emit_call(compiler, SLJIT_CALL_REG_ARG, SLJIT_ARGS0(VOID));
        Index funcIndex = call_instr->value().funcIndex;

        context->compiler->getFunctionEntry(funcIndex)->jumpFrom(jump);
    }

    StackAllocator stackAllocator;
    operandEnd = operand + call_instr->resultCount();

    while (operand < operandEnd) {
        stackAllocator.push(operand->location.valueInfo);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (!(operand->location.valueInfo & LocationInfo::kFloat) && (operand->location.valueInfo & LocationInfo::kSizeMask) == 2) {
            operandToArgPair(operand, arg64);

            from.arg = SLJIT_MEM1(kFrameReg);
            from.argw = static_cast<sljit_sw>(stackAllocator.last().value);
            to.arg = arg64.arg1;
            to.argw = arg64.arg1w;

            if (from.arg != to.arg || from.argw != to.argw) {
                moveContext.move(operand->location.valueInfo, from, to);
            }

            from.argw += sizeof(sljit_sw);
            to.arg = arg64.arg2;
            to.argw = arg64.arg2w;

            if (from.arg != to.arg || from.argw != to.argw) {
                moveContext.move(operand->location.valueInfo, from, to);
            }

            operand++;
            continue;
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        operandToArg(operand, to);
        from.arg = SLJIT_MEM1(kFrameReg);
        from.argw = static_cast<sljit_sw>(stackAllocator.last().value);

        if (from.arg != to.arg || from.argw != to.argw) {
            moveContext.move(operand->location.valueInfo, from, to);
        }

        operand++;
    }

    moveContext.done();
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

struct LabelData {
    LabelData(sljit_label* label)
        : label(label)
    {
    }

    sljit_label* label;
};

void Label::jumpFrom(sljit_jump* jump)
{
    if (info() & Label::kHasLabelData) {
        sljit_set_label(jump, m_labelData->label);
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
    assert(!(info() & Label::kHasLabelData));

    sljit_label* label = sljit_emit_label(compiler);

    if (info() & Label::kHasJumpList) {
        for (auto it : m_jumpList->jumpList) {
            sljit_set_label(it, label);
        }

        delete m_jumpList;
        setInfo(info() ^ Label::kHasJumpList);
    }

    m_labelData = new LabelData(label);
    addInfo(Label::kHasLabelData);
}

JITModule* JITCompiler::compile()
{
    CompileContext compileContext(this);
    m_compiler = sljit_create_compiler(reinterpret_cast<void*>(&compileContext), nullptr);

    // Follows the declaration of FunctionDescriptor::ExternalDecl().
    // Context stored in SLJIT_S0 (kContextReg)
    // Frame stored in SLJIT_S1 (kFrameReg)
    sljit_emit_enter(m_compiler, 0, SLJIT_ARGS3(VOID, P, P, P_R), 3, 2, 0, 0, 0);
    sljit_emit_icall(m_compiler, SLJIT_CALL_REG_ARG, SLJIT_ARGS0(VOID), SLJIT_R2,
                     0);
    sljit_label* returnToLabel = sljit_emit_label(m_compiler);
    sljit_emit_return_void(m_compiler);

    compileContext.trapBlocks.push_back(
        TrapBlock(sljit_emit_label(m_compiler), returnToLabel));

    size_t currentFunction = 0;

    for (InstructionListItem* item = m_functionListFirst; item != nullptr;
         item = item->next()) {
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
                compileContext.frameSize = m_functionList[currentFunction].jitFunc->frameSize();
            }
            continue;
        }

        switch (item->group()) {
        case Instruction::Immediate: {
            emitImmediate(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::LocalMove: {
            emitLocalMove(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::DirectBranch: {
            emitDirectBranch(m_compiler, item->asInstruction());
            break;
        }
        case Instruction::Call: {
            emitCall(m_compiler, item->asInstruction()->asCall());
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
        case Instruction::Convert: {
            emitConvert(m_compiler, item->asInstruction());
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
            case UnreachableOpcode: {
                sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                               ExecutionContext::UnreachableError);
                sljit_set_label(sljit_emit_jump(m_compiler, SLJIT_JUMP),
                                compileContext.trapLabel);
                break;
            }
            default: {
                WABT_UNREACHABLE;
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

            assert((label->info() & Label::kNewFunction) || label->branches().size() > 0);

            if (label->info() & Label::kHasJumpList) {
                assert(!(label->info() & Label::kHasLabelData));
                delete label->m_jumpList;
            } else if (label->info() & Label::kHasLabelData) {
                delete label->m_labelData;
            }
        }

        InstructionListItem* next = item->next();

        assert(next == nullptr || next->m_prev == item);

        delete item;
        item = next;
    }

    m_functionList.clear();
}

void JITCompiler::emitProlog(size_t index, CompileContext& context)
{
    FunctionList& func = m_functionList[index];
    sljit_s32 savedRegCount = 4;

    sljit_set_context(m_compiler, SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2),
                      SLJIT_ARGS0(VOID), SLJIT_NUMBER_OF_SCRATCH_REGISTERS,
                      savedRegCount, SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, 0,
                      sizeof(ExecutionContext::CallFrame));

    context.trapLabel = sljit_emit_label(m_compiler);
    sljit_emit_op1(m_compiler, SLJIT_MOV_U32, SLJIT_MEM1(kContextReg),
                   OffsetOfContextField(error), SLJIT_R2, 0);

    context.returnToLabel = sljit_emit_label(m_compiler);

    sljit_emit_op_dst(m_compiler, SLJIT_GET_RETURN_ADDRESS, SLJIT_R1, 0);
    sljit_emit_op2(m_compiler, SLJIT_SUB, SLJIT_R0, 0, kContextReg, 0, SLJIT_IMM,
                   sizeof(ExecutionContext));
    sljit_emit_icall(m_compiler, SLJIT_CALL, SLJIT_ARGS2(W, W, W), SLJIT_IMM,
                     GET_FUNC_ADDR(sljit_sw, getTrapHandler));
    sljit_emit_return_to(m_compiler, SLJIT_R0, 0);

    if (func.isExported) {
        func.exportEntryLabel = sljit_emit_label(m_compiler);
    }

    func.entryLabel->emit(m_compiler);

    sljit_emit_enter(m_compiler, SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2),
                     SLJIT_ARGS0(VOID), SLJIT_NUMBER_OF_SCRATCH_REGISTERS,
                     savedRegCount, SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, 0,
                     sizeof(ExecutionContext::CallFrame));

    // Setup new frame.
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg),
                   OffsetOfContextField(lastFrame));

    if (func.jitFunc->frameSize() > 0) {
        sljit_emit_op2(m_compiler, SLJIT_SUB, kFrameReg, 0, kFrameReg, 0, SLJIT_IMM,
                       static_cast<sljit_sw>(func.jitFunc->frameSize()));

        sljit_emit_op1(m_compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                       ExecutionContext::OutOfStackError);
        sljit_jump* cmp = sljit_emit_cmp(m_compiler, SLJIT_LESS, kFrameReg, 0, kContextReg, 0);
        sljit_set_label(cmp, context.trapLabel);
    }

    sljit_get_local_base(m_compiler, SLJIT_R1, 0, 0);
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(kContextReg),
                   OffsetOfContextField(lastFrame), SLJIT_R1, 0);
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP),
                   offsetof(ExecutionContext::CallFrame, frameStart), kFrameReg,
                   0);
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP),
                   offsetof(ExecutionContext::CallFrame, prevFrame), SLJIT_R0,
                   0);
}

sljit_label* JITCompiler::emitEpilog(size_t index, CompileContext& context)
{
    FunctionList& func = m_functionList[index];

    // Restore previous frame.
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_SP),
                   offsetof(ExecutionContext::CallFrame, prevFrame));
    if (func.jitFunc->frameSize() > 0) {
        sljit_emit_op2(m_compiler, SLJIT_ADD, kFrameReg, 0, kFrameReg, 0, SLJIT_IMM,
                       static_cast<sljit_sw>(func.jitFunc->frameSize()));
    }
    sljit_emit_op1(m_compiler, SLJIT_MOV_P, SLJIT_MEM1(kContextReg),
                   OffsetOfContextField(lastFrame), SLJIT_R0, 0);

    sljit_emit_return_void(m_compiler);

    context.emitSlowCases(m_compiler);

    sljit_label* endLabel = sljit_emit_label(m_compiler);
    context.trapBlocks.push_back(TrapBlock(endLabel, context.returnToLabel));
    return endLabel;
}

} // namespace Walrus
