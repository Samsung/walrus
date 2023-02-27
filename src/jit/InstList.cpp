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

#include "jit/Compiler.h"
#include "runtime/ObjectType.h"

#include <map>

namespace Walrus {

Index Instruction::resultCount()
{
    if (!hasResult()) {
        return 0;
    }

    if (group() == Call) {
        CallInstruction* callInstr = reinterpret_cast<CallInstruction*>(this);
        return callInstr->functionType()->result().size();
    }

    return 1;
}

BrTableInstruction::~BrTableInstruction()
{
    delete[] m_targetLabels;
}

ComplexInstruction::~ComplexInstruction()
{
    delete[] params();
}

template <int n>
SimpleCallInstruction<n>::SimpleCallInstruction(OpcodeKind opcode, FunctionType* functionType, InstructionListItem* prev)
    : CallInstruction(opcode, functionType->param().size(), functionType, m_inlineOperands, prev)
{
    assert(functionType->param().size() + functionType->result().size() == n);
}

ComplexCallInstruction::ComplexCallInstruction(OpcodeKind opcode, FunctionType* functionType, InstructionListItem* prev)
    : CallInstruction(opcode, functionType->param().size(), functionType, new Operand[functionType->param().size() + functionType->result().size()], prev)
{
    assert(functionType->param().size() + functionType->result().size() > 4);
}

ComplexCallInstruction::~ComplexCallInstruction()
{
    delete[] params();
}

BrTableInstruction::BrTableInstruction(size_t targetLabelCount, InstructionListItem* prev)
    : Instruction(Instruction::BrTable, BrTableOpcode, 1, &m_inlineParam, prev)
{
    m_targetLabels = new Label*[targetLabelCount];
    value().targetLabelCount = targetLabelCount;
}

void Label::append(Instruction* instr)
{
    for (auto it : m_branches) {
        if (it == instr) {
            return;
        }
    }

    m_branches.push_back(instr);
}

void Label::merge(Label* other)
{
    assert(this != other);
    assert(stackSize() == other->stackSize());

    for (auto it : other->m_branches) {
        if (it->group() != Instruction::BrTable) {
            assert(it->group() == Instruction::DirectBranch);
            it->value().targetLabel = this;
            m_branches.push_back(it);
            continue;
        }

        BrTableInstruction* instr = it->asBrTable();

        Label** label = instr->targetLabels();
        Label** end = label + instr->value().targetLabelCount;
        bool found = false;

        do {
            if (*label == other) {
                *label = this;
            } else if (*label == this) {
                found = true;
            }
        } while (++label < end);

        if (!found) {
            m_branches.push_back(it);
        }
    }
}

void JITCompiler::clear()
{
    for (auto it : m_labelStack) {
        if (it != nullptr && it->m_prev == nullptr) {
            delete it;
        }
    }

    m_stackDepth = 0;
    m_labelStack.clear();
    m_elseBlocks.clear();
    m_locals.clear();

    InstructionListItem* item = m_first;

    m_first = nullptr;
    m_last = nullptr;

    while (item != nullptr) {
        InstructionListItem* next = item->next();

        assert(next == nullptr || next->m_prev == item);
        assert(!item->isLabel() || item->asLabel()->branches().size() > 0);

        delete item;
        item = next;
    }
}

Instruction* JITCompiler::append(Instruction::Group group, OpcodeKind op, Index paramCount)
{
    return appendInternal(group, op, paramCount, paramCount, LocationInfo::kNone);
}

Instruction* JITCompiler::append(Instruction::Group group, OpcodeKind op, Index paramCount, ValueInfo result)
{
    assert(result != LocationInfo::kNone);
    return appendInternal(group, op, paramCount, paramCount + 1, result);
}

CallInstruction* JITCompiler::appendCall(OpcodeKind opcode, FunctionType* functionType)
{
    if (m_stackDepth == wabt::kInvalidIndex) {
        return nullptr;
    }

    Index paramCount = functionType->param().size();
    Index resultCount = functionType->result().size();

    m_stackDepth -= paramCount;
    m_stackDepth += resultCount;

    CallInstruction* callInstr;
    switch (paramCount + resultCount) {
    case 0:
        callInstr = new CallInstruction(opcode, 0, functionType, nullptr, nullptr);
        break;
    case 1:
        callInstr = new SimpleCallInstruction<1>(opcode, functionType, m_last);
        break;
    case 2:
        callInstr = new SimpleCallInstruction<2>(opcode, functionType, m_last);
        break;
    case 3:
        callInstr = new SimpleCallInstruction<3>(opcode, functionType, m_last);
        break;
    case 4:
        callInstr = new SimpleCallInstruction<4>(opcode, functionType, m_last);
        break;
    default:
        callInstr = new ComplexCallInstruction(opcode, functionType, m_last);
        break;
    }

    callInstr->m_resultCount = static_cast<uint8_t>((resultCount > 255) ? 255 : resultCount);

    StackAllocator stackAllocator;

    if (resultCount > 0) {
        Operand* results = callInstr->results();

        for (Index i = 0; i < resultCount; i++) {
            ValueInfo valueInfo = LocationInfo::typeToValueInfo(functionType->result()[i]);

            results[i].location.type = Operand::Stack;
            results[i].location.valueInfo = valueInfo;
            stackAllocator.push(valueInfo);
        }
    }

    callInstr->m_paramStart = stackAllocator.size();

    LocalsAllocator localsAllocator(stackAllocator.size());

    for (auto it : functionType->param()) {
        localsAllocator.allocate(LocationInfo::typeToValueInfo(it));
    }

    callInstr->m_frameSize = StackAllocator::alignedSize(localsAllocator.size());

    append(callInstr);
    return callInstr;
}

void JITCompiler::appendBranch(OpcodeKind opcode, Index depth)
{
    if (m_stackDepth == wabt::kInvalidIndex) {
        return;
    }

    assert(depth < m_labelStack.size());
    assert(opcode == BrOpcode || opcode == BrIfOpcode);

    Label* label = m_labelStack[m_labelStack.size() - (depth + 1)];
    Instruction* branch;

    if (opcode == BrOpcode) {
        m_stackDepth = wabt::kInvalidIndex;
        branch = new Instruction(Instruction::DirectBranch, BrOpcode, m_last);
    } else {
        m_stackDepth--;
        branch = new SimpleInstruction<1>(Instruction::DirectBranch, opcode, m_last);
    }

    branch->value().targetLabel = label;
    label->m_branches.push_back(branch);
    append(branch);
}

void JITCompiler::appendElseLabel()
{
    Label* label = m_labelStack[m_labelStack.size() - 1];

    if (label == nullptr) {
        assert(m_stackDepth == wabt::kInvalidIndex);
        return;
    }

    assert(label->info() & Label::kCanHaveElse);
    label->clearInfo(Label::kCanHaveElse);

    if (m_stackDepth != wabt::kInvalidIndex) {
        Instruction* branch = new Instruction(Instruction::DirectBranch, BrOpcode, m_last);
        branch->value().targetLabel = label;
        append(branch);
        label->m_branches.push_back(branch);
    }

    ElseBlock& elseBlock = m_elseBlocks.back();

    label = new Label(elseBlock.resultCount, elseBlock.preservedCount, m_last);
    label->addInfo(Label::kAfterUncondBranch);
    append(label);
    m_stackDepth = elseBlock.preservedCount + elseBlock.resultCount;

    Instruction* branch = elseBlock.branch;
    assert(branch->opcode() == InterpBrUnlessOpcode);

    branch->value().targetLabel = label;
    label->m_branches.push_back(branch);
    m_elseBlocks.pop_back();
}

void JITCompiler::appendBrTable(Index numTargets, Index* targetDepths, Index defaultTargetDepth)
{
    if (m_stackDepth == wabt::kInvalidIndex) {
        return;
    }

    BrTableInstruction* branch = new BrTableInstruction(numTargets + 1, m_last);
    append(branch);
    m_stackDepth = wabt::kInvalidIndex;

    Label* label;
    Label** labels = branch->targetLabels();
    size_t labelStackSize = m_labelStack.size();

    for (Index i = 0; i < numTargets; i++) {
        label = m_labelStack[labelStackSize - (*targetDepths + 1)];

        targetDepths++;
        *labels++ = label;
        label->append(branch);
    }

    label = m_labelStack[labelStackSize - (defaultTargetDepth + 1)];
    *labels = label;
    label->append(branch);
}

void JITCompiler::pushLabel(OpcodeKind opcode, Index paramCount, Index resultCount)
{
    InstructionListItem* prev = nullptr;

    if (m_stackDepth == wabt::kInvalidIndex) {
        m_labelStack.push_back(nullptr);
        return;
    }

    if (opcode == LoopOpcode) {
        if (m_last != nullptr && m_last->isLabel()) {
            m_labelStack.push_back(m_last->asLabel());
            return;
        }

        resultCount = paramCount;
        prev = m_last;
    } else if (opcode == IfOpcode) {
        m_stackDepth--;
    }

    Label* label = new Label(resultCount, m_stackDepth - paramCount, prev);
    m_labelStack.push_back(label);

    if (opcode == LoopOpcode) {
        append(label);
    } else if (opcode == IfOpcode) {
        label->addInfo(Label::kCanHaveElse);

        Instruction* branch = new SimpleInstruction<1>(Instruction::DirectBranch, InterpBrUnlessOpcode, m_last);
        m_elseBlocks.push_back(ElseBlock(branch, paramCount, m_stackDepth - paramCount));
        append(branch);
    }
}

void JITCompiler::popLabel()
{
    Label* label = m_labelStack.back();
    m_labelStack.pop_back();

    if (label == nullptr) {
        assert(m_stackDepth == wabt::kInvalidIndex);
        return;
    }

    if (label->m_prev != nullptr || m_first == label) {
        // Loop instruction.
        if (label->branches().size() == 0) {
            remove(label);
        }
        return;
    }

    if (label->info() & Label::kCanHaveElse) {
        Instruction* branch = m_elseBlocks.back().branch;

        branch->value().targetLabel = label;
        label->m_branches.push_back(branch);
        label->clearInfo(Label::kCanHaveElse);

        m_elseBlocks.pop_back();
    }

    if (label->branches().size() == 0) {
        delete label;
        return;
    }

    if (m_last != nullptr && m_last->isLabel()) {
        assert(m_last->asLabel()->branches().size() > 0 && m_stackDepth == m_last->asLabel()->stackSize());
        m_last->asLabel()->merge(label);
        delete label;
        return;
    }

    append(label);
    if (m_stackDepth == wabt::kInvalidIndex) {
        label->addInfo(Label::kAfterUncondBranch);
    }

    m_stackDepth = label->preservedCount() + label->resultCount();
}

void JITCompiler::appendFunction(JITFunction* jitFunc, bool isExternal)
{
    assert(m_first != nullptr && m_last != nullptr);

    Label* entryLabel = new Label(0, 0, m_functionListLast);
    entryLabel->addInfo(Label::kNewFunction);
    entryLabel->m_next = m_first;
    m_first->m_prev = entryLabel;

    if (m_functionListFirst == nullptr) {
        assert(m_functionListLast == nullptr);
        m_functionListFirst = entryLabel;
    } else {
        assert(m_functionListLast != nullptr);
        m_functionListLast->m_next = entryLabel;
    }

    m_functionListLast = m_last;
    m_first = nullptr;
    m_last = nullptr;
    m_functionList.push_back(FunctionList(jitFunc, entryLabel, isExternal));
}

static const char* flagsToType(ValueInfo valueInfo)
{
    ValueInfo size = valueInfo & LocationInfo::kSizeMask;

    if (valueInfo & LocationInfo::kReference) {
        return (size == LocationInfo::kEightByteSize) ? "ref8" : "ref4";
    }

    if (valueInfo & LocationInfo::kFloat) {
        return (size == LocationInfo::kEightByteSize) ? "f64" : "f32";
    }

    if (size == LocationInfo::kSixteenByteSize) {
        return "v128";
    }

    return (size == LocationInfo::kEightByteSize) ? "i64" : "i32";
}

void JITCompiler::dump(bool afterStackComputation)
{
    std::map<InstructionListItem*, int> instrIndex;
    bool enableColors = (verboseLevel() >= 2);
    int counter = 0;

    for (InstructionListItem* item = first(); item != nullptr; item = item->next()) {
        instrIndex[item] = counter++;
    }

    const char* defaultText = enableColors ? "\033[0m" : "";
    const char* instrText = enableColors ? "\033[1;35m" : "";
    const char* labelText = enableColors ? "\033[1;36m" : "";
    const char* unusedText = enableColors ? "\033[1;33m" : "";

    for (InstructionListItem* item = first(); item != nullptr; item = item->next()) {
        if (item->isInstruction()) {
            Instruction* instr = item->asInstruction();
            printf("%s%d%s: ", instrText, instrIndex[item], defaultText);

            if (enableColors) {
                printf("(%p) ", item);
            }

            printf("Opcode: %s\n", g_byteCodeInfo[instr->opcode()].m_name);

            switch (instr->group()) {
            case Instruction::DirectBranch: {
                printf("  Jump to: %s%d%s\n", labelText, instrIndex[instr->value().targetLabel], defaultText);
                break;
            }
            case Instruction::LocalMove: {
                if (afterStackComputation) {
                    printf("  Addr: sp[%d]\n", instr->value().localIndex);
                } else {
                    printf("  Index: %d\n", instr->value().localIndex);
                }
                break;
            }
            case Instruction::Call: {
                printf("  Frame size: %d, param start: %d\n", instr->asCall()->frameSize(), instr->asCall()->paramStart());
                break;
            }
            case Instruction::BrTable: {
                size_t targetLabelCount = instr->value().targetLabelCount;
                Label** targetLabels = instr->asBrTable()->targetLabels();

                for (size_t i = 0; i < targetLabelCount; i++) {
                    printf("  Jump to: %s%d%s\n", labelText, instrIndex[*targetLabels], defaultText);
                    targetLabels++;
                }
                break;
            }
            default: {
                break;
            }
            }

            Index paramCount = instr->paramCount();
            Index size = paramCount + instr->resultCount();
            Operand* param = instr->params();

            for (Index i = 0; i < size; ++i) {
                if (i < paramCount) {
                    printf("  Param[%d]: ", static_cast<int>(i));
                } else {
                    printf("  Result[%d]: ", static_cast<int>(i - paramCount));
                }

                if (afterStackComputation) {
                    printf("%s ", flagsToType(param->location.valueInfo));

                    switch (param->location.type) {
                    case Operand::Stack: {
                        printf("sp[%d]\n", static_cast<int>(param->value));
                        break;
                    }
                    case Operand::Register: {
                        printf("reg:%d\n", static_cast<int>(param->value));
                        break;
                    }
                    case Operand::Immediate: {
                        const char* color = param->item->isLabel() ? labelText : instrText;
                        printf("const:%s%d%s\n", color, instrIndex[param->item], defaultText);
                        break;
                    }
                    case Operand::Unused: {
                        printf("%sunused%s\n", unusedText, defaultText);
                        break;
                    }
                    default: {
                        printf("%s[UNKNOWN]%s\n", unusedText, defaultText);
                        break;
                    }
                    }
                } else if (i < paramCount) {
                    const char* color = param->item->isLabel() ? labelText : instrText;
                    printf("ref:%s%d%s(%d)\n", color, instrIndex[param->item], defaultText, static_cast<int>(param->index));
                } else {
                    printf("%s\n", flagsToType(param->location.valueInfo));
                }
                param++;
            }
        } else {
            printf("%s%d%s: ", labelText, instrIndex[item], defaultText);

            if (enableColors) {
                printf("(%p) ", item);
            }

            Label* label = item->asLabel();

            printf("Label: resultCount: %d preservedCount: %d\n", static_cast<int>(label->resultCount()), static_cast<int>(label->preservedCount()));

            for (auto it : label->branches()) {
                printf("  Jump from: %s%d%s\n", instrText, instrIndex[it], defaultText);
            }

            size_t size = label->dependencyCount();

            for (size_t i = 0; i < size; ++i) {
                printf("  Param[%d]\n", static_cast<int>(i));
                for (auto dependencyIt : label->dependencies(i)) {
                    printf("    %s%d%s(%d) instruction\n", instrText, instrIndex[dependencyIt.instr], defaultText, static_cast<int>(dependencyIt.index));
                }
            }
        }
    }
}

void JITCompiler::append(InstructionListItem* item)
{
    if (m_last != nullptr) {
        m_last->m_next = item;
    } else {
        m_first = item;
    }

    item->m_prev = m_last;
    m_last = item;
}

InstructionListItem* JITCompiler::remove(InstructionListItem* item)
{
    InstructionListItem* prev = item->m_prev;
    InstructionListItem* next = item->m_next;

    if (prev == nullptr) {
        assert(m_first == item);
        m_first = next;
    } else {
        assert(m_first != item);
        prev->m_next = next;
    }

    if (next == nullptr) {
        assert(m_last == item);
        m_last = prev;
    } else {
        assert(m_last != item);
        next->m_prev = prev;
    }

    delete item;
    return next;
}

void JITCompiler::replace(InstructionListItem* item, InstructionListItem* newItem)
{
    assert(item != newItem);

    InstructionListItem* prev = item->m_prev;
    InstructionListItem* next = item->m_next;

    newItem->m_prev = prev;
    newItem->m_next = next;

    if (prev == nullptr) {
        assert(m_first == item);
        m_first = newItem;
    } else {
        assert(m_first != item);
        prev->m_next = newItem;
    }

    if (next == nullptr) {
        assert(m_last == item);
        m_last = newItem;
    } else {
        assert(m_last != item);
        next->m_prev = newItem;
    }

    delete item;
}

Instruction* JITCompiler::appendInternal(Instruction::Group group, OpcodeKind op, Index paramCount, Index operandCount, ValueInfo result)
{
    if (m_stackDepth == wabt::kInvalidIndex) {
        return nullptr;
    }

    m_stackDepth -= paramCount;

    Instruction* instr;
    switch (operandCount) {
    case 0:
        instr = new Instruction(group, op, 0, nullptr, m_last);
        break;
    case 1:
        instr = new SimpleInstruction<1>(group, op, paramCount, m_last);
        break;
    case 2:
        instr = new SimpleInstruction<2>(group, op, paramCount, m_last);
        break;
    case 3:
        instr = new SimpleInstruction<3>(group, op, paramCount, m_last);
        break;
    case 4:
        instr = new SimpleInstruction<4>(group, op, paramCount, m_last);
        break;
    default:
        instr = new ComplexInstruction(group, op, paramCount, operandCount, m_last);
        break;
    }

    if (result != LocationInfo::kNone) {
        instr->m_resultCount = 1;
        m_stackDepth++;

        Operand* defaultResult = instr->getResult(0);
        defaultResult->location.type = Operand::Stack;
        defaultResult->location.valueInfo = result;
    }

    append(instr);
    return instr;
}

} // namespace Walrus
