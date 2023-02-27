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
#include "runtime/JITExec.h"
#include "runtime/ObjectType.h"

#include <set>

namespace Walrus {

class DependencyGenContext {
public:
    struct Dependency {
        Dependency(InstructionListItem* item, size_t index)
            : item(item)
            , index(index)
        {
        }

        bool operator<(const Dependency& other) const
        {
            uintptr_t left = reinterpret_cast<uintptr_t>(item);
            uintptr_t right = reinterpret_cast<uintptr_t>(other.item);

            return (left < right) || (left == right && index < other.index);
        }

        InstructionListItem* item;
        Index index;
    };

    struct StackItem {
        std::vector<Dependency> instDeps;
        std::vector<Dependency> labelDeps;
    };

    DependencyGenContext(Label* label)
        : m_preservedCount(label->preservedCount())
        , m_startIndex(0)
    {
        m_stack.resize(label->stackSize());
    }

    size_t startIndex() { return m_startIndex; }
    void setStartIndex(size_t value) { m_startIndex = value; }
    std::vector<StackItem>* stack() { return &m_stack; }
    StackItem& stackItem(size_t index) { return m_stack[index]; }

    void update(std::vector<Operand>& deps);

private:
    static void append(std::vector<Dependency>& list, InstructionListItem* item, Index index);

    Index m_preservedCount;
    size_t m_startIndex;
    std::vector<StackItem> m_stack;
};

void DependencyGenContext::update(std::vector<Operand>& deps)
{
    assert(deps.size() >= m_stack.size());

    size_t size = m_stack.size();
    size_t current = 0;

    for (size_t i = 0; i < size; i++, current++) {
        if (current == m_preservedCount) {
            // These items are ignored when the branch is executed.
            current += deps.size() - m_stack.size();
        }

        Operand& dep = deps[current];

        if (dep.item->isInstruction()) {
            append(m_stack[i].instDeps, dep.item, dep.index);
        } else {
            append(m_stack[i].labelDeps, dep.item, dep.index);
        }
    }

    assert(current == deps.size() || size == m_preservedCount);
}

void DependencyGenContext::append(std::vector<Dependency>& list, InstructionListItem* item, Index index)
{
    for (auto it : list) {
        if (it.item == item && it.index == index) {
            return;
        }
    }

    list.push_back(Dependency(item, index));
}

static void findSelectResult(Instruction* selectInstr)
{
    /* Must depend on an instruction which is placed before. */
    assert(!(selectInstr->info() & Instruction::kHasResultValueInfo));

    ValueInfo valueInfo = LocationInfo::kNone;
    Operand* operand = selectInstr->getParam(0);

    if (operand->item->isLabel()) {
        const Label::DependencyList& deps = operand->item->asLabel()->dependencies(operand->index);

        for (auto it : deps) {
            if (it.instr->opcode() != SelectOpcode || (it.instr->info() & Instruction::kHasResultValueInfo)) {
                valueInfo = it.instr->getResult(it.index)->location.valueInfo;
                break;
            }
        }

        assert(valueInfo != LocationInfo::kNone);
    } else {
        Instruction* instr = operand->item->asInstruction();

        assert(instr->opcode() != SelectOpcode || (instr->info() & Instruction::kHasResultValueInfo));

        valueInfo = instr->getResult(operand->index)->location.valueInfo;
        assert(valueInfo != LocationInfo::kNone);
    }

    selectInstr->getResult(0)->location.valueInfo = valueInfo;
    selectInstr->addInfo(Instruction::kHasResultValueInfo);
}

void JITCompiler::buildParamDependencies()
{
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();
            label->m_dependencyCtx = new DependencyGenContext(label);
        }
    }

    bool updateDeps = true;
    size_t startIndex = 0;
    std::vector<Operand> currentDeps;

    // Phase 1: the direct dependencies are computed for instructions
    // and labels (only labels can have multiple dependencies).
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            if (startIndex > 0) {
                assert(currentDeps[0].item->isLabel());
                Label* label = currentDeps[0].item->asLabel();
                label->m_dependencyCtx->setStartIndex(startIndex);
            }

            // Build a dependency list which refers to the last label.
            Label* label = item->asLabel();

            if (updateDeps) {
                label->m_dependencyCtx->update(currentDeps);
                currentDeps.clear();
            }

            startIndex = label->m_dependencyCtx->stack()->size();

            currentDeps.resize(startIndex);

            for (size_t i = 0; i < startIndex; ++i) {
                Operand& dep = currentDeps[i];
                dep.item = label;
                dep.index = i;
            }

            updateDeps = true;
            continue;
        }

        Instruction* instr = item->asInstruction();

        // Pop params from the stack first.
        Index end = instr->paramCount();
        Operand* param = instr->params() + end;

        for (Index i = end; i > 0; --i) {
            *(--param) = currentDeps.back();
            currentDeps.pop_back();

            if (param->item->isLabel()) {
                // Values are consumed top to bottom.
                assert(startIndex == param->index + 1);
                startIndex = param->index;
            }
        }

        // Push results next.
        if (instr->hasResult()) {
            Operand dep;
            dep.item = instr;

            end = instr->resultCount();
            for (Index i = 0; i < end; ++i) {
                dep.index = i;
                currentDeps.push_back(dep);
            }
        }

        if (instr->group() == Instruction::DirectBranch) {
            Label* label = instr->value().targetLabel;
            label->m_dependencyCtx->update(currentDeps);

            if (instr->opcode() == BrOpcode) {
                updateDeps = false;
            }
        } else if (instr->group() == Instruction::BrTable) {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;
            std::set<Label*> updatedLabels;

            while (label < end) {
                if (updatedLabels.insert(*label).second) {
                    (*label)->m_dependencyCtx->update(currentDeps);
                }
                label++;
            }
            updateDeps = false;
        }
    }

    // Phase 2: the indirect instruction
    // references are computed for labels.
    std::vector<DependencyGenContext::StackItem>* lastStack = nullptr;
    Index lastStartIndex = 0;

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (!item->isLabel()) {
            Instruction* instr = item->asInstruction();
            Operand* param = instr->params();

            for (Index i = instr->paramCount(); i > 0; --i) {
                if (param->item->isLabel()) {
                    assert(param->item->asLabel()->m_dependencyCtx->stack() == lastStack);
                    assert(param->index >= lastStartIndex);

                    std::vector<DependencyGenContext::Dependency> instDeps = (*lastStack)[param->index].instDeps;

                    if (instDeps.size() == 1) {
                        // A single reference is copied into the instruction.
                        param->item = instDeps[0].item;
                        param->index = instDeps[0].index;
                    } else {
                        // References below lastStartIndex are deleted.
                        param->index -= lastStartIndex;
                    }
                }
                param++;
            }
            continue;
        }

        DependencyGenContext* context = item->asLabel()->m_dependencyCtx;

        lastStack = context->stack();
        lastStartIndex = static_cast<Index>(context->startIndex());
        size_t end = lastStack->size();

        for (size_t i = lastStartIndex; i < end; ++i) {
            std::vector<DependencyGenContext::Dependency> unprocessedLabels;
            std::set<DependencyGenContext::Dependency> processedDeps;
            std::vector<DependencyGenContext::Dependency>& instDeps = (*lastStack)[i].instDeps;

            for (auto it : instDeps) {
                processedDeps.insert(it);
            }

            for (auto it : (*lastStack)[i].labelDeps) {
                processedDeps.insert(it);
                unprocessedLabels.push_back(it);
            }

            (*lastStack)[i].labelDeps.clear();

            while (!unprocessedLabels.empty()) {
                DependencyGenContext::Dependency& dep = unprocessedLabels.back();
                DependencyGenContext::StackItem& item = dep.item->asLabel()->m_dependencyCtx->stackItem(dep.index);

                unprocessedLabels.pop_back();

                for (auto it : item.instDeps) {
                    if (processedDeps.insert(it).second) {
                        instDeps.push_back(it);
                    }
                }

                for (auto it : item.labelDeps) {
                    if (processedDeps.insert(it).second) {
                        unprocessedLabels.push_back(it);
                    }
                }
            }

            // At least one instruction dependency
            // must be present if the input is valid.
            assert(instDeps.size() > 0);
        }
    }

    // Phase 3: The indirect references for labels are
    // collected, and all temporary structures are deleted.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (!item->isLabel()) {
            if (item->group() == Instruction::Any && item->asInstruction()->opcode() == SelectOpcode) {
                // Label data has been computed.
                findSelectResult(item->asInstruction());
            }
            continue;
        }

        // Copy the final dependency data into
        // the dependency list of the label.
        Label* label = item->asLabel();
        DependencyGenContext* context = label->m_dependencyCtx;
        std::vector<DependencyGenContext::StackItem>* stack = context->stack();
        size_t startIndex = context->startIndex();
        size_t end = stack->size();

        // Single item dependencies are
        // moved into the corresponding oeprand data.
        label->m_dependencies.resize(end - startIndex);

        for (size_t i = startIndex; i < end; ++i) {
            assert((*stack)[i].labelDeps.empty());

            Label::DependencyList& list = label->m_dependencies[i - startIndex];

            size_t size = (*stack)[i].instDeps.size();

            if (size > 1) {
                list.reserve(size);

                for (auto it : (*stack)[i].instDeps) {
                    list.push_back(Label::Dependency(it.item->asInstruction(), it.index));
                }
            }
        }

        delete context;
    }
}

void JITCompiler::optimizeBlocks()
{
    std::vector<Label::Dependency> stack;

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            stack.clear();
            continue;
        }

        switch (item->group()) {
        case Instruction::Call: {
            CallInstruction* callInstr = reinterpret_cast<CallInstruction*>(item);

            LocalsAllocator localsAllocator(callInstr->paramStart());

            for (auto it : callInstr->functionType()->param()) {
                localsAllocator.allocate(LocationInfo::typeToValueInfo(it));
            }

            Index end = callInstr->paramCount();
            size_t size = stack.size();

            for (Index i = 0; i < end; ++i) {
                if (size + i >= end) {
                    Label::Dependency& item = stack[size + i - end];

                    Operand* operand = item.instr->getResult(item.index);

                    if (operand->location.type == Operand::Stack) {
                        operand->location.type = Operand::CallArg;
                        operand->value = localsAllocator.get(i).value;
                    }
                }
            }

            stack.clear();
            continue;
        }
        case Instruction::Compare: {
            assert(item->next() != nullptr);

            if (item->next()->group() == Instruction::DirectBranch || item->next()->group() == Instruction::Any) {
                Instruction* next = item->next()->asInstruction();

                if (next->opcode() == BrIfOpcode || next->opcode() == InterpBrUnlessOpcode || next->opcode() == SelectOpcode) {
                    item->asInstruction()->getResult(0)->location.type = Operand::Unused;
                }
            }
            break;
        }
        default: {
            break;
        }
        }

        Instruction* instr = item->asInstruction();
        Index end = instr->paramCount();

        for (Index i = end; i > 0; --i) {
            if (stack.empty()) {
                break;
            }

            stack.pop_back();
        }

        end = instr->resultCount();

        for (Index i = 0; i < end; i++) {
            stack.push_back(Label::Dependency(instr, i));
        }
    }
}

static StackAllocator* cloneAllocator(Label* label, StackAllocator* other)
{
    assert(other->values().size() >= label->stackSize());

    if (other->values().size() == label->stackSize()) {
        return new StackAllocator(*other);
    }

    StackAllocator* allocator = new StackAllocator(other, label->preservedCount());

    std::vector<LocationInfo>& otherValues = other->values();
    Index end = label->resultCount();
    Index current = otherValues.size() - end;

    for (Index i = 0; i < end; i++, current++) {
        LocationInfo& info = otherValues[current];

        if (info.status & LocationInfo::kIsOffset) {
            allocator->push(info.valueInfo);
        } else {
            allocator->values().push_back(info);
        }
    }

    return allocator;
}

void JITCompiler::computeOperandLocations(JITFunction* jitFunc, ValueTypeVector& results)
{
    // Build space for results first.
    StackAllocator* m_stackAllocator = new StackAllocator();

    for (auto it : results) {
        m_stackAllocator->push(LocationInfo::typeToValueInfo(it));
    }

    Index localsStart = m_stackAllocator->size();
    LocalsAllocator localsAllocator(localsStart);

    size_t size = m_locals.size();

    for (size_t i = 0; i < size; i++) {
        localsAllocator.allocate(m_locals[i]);
    }

    Index maxMStackSize = localsAllocator.size();
    Index argsSize = StackAllocator::alignedSize(maxMStackSize);
    Index totalFrameSize = argsSize;
    Index maxCallFrameSize = 0;

    m_stackAllocator->skipRange(localsStart, maxMStackSize);

    // Compute stack allocation.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();
            label->m_stackAllocator = nullptr;
        }
    }

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();

            maxCallFrameSize += StackAllocator::alignedSize(maxMStackSize);
            if (maxCallFrameSize > totalFrameSize) {
                totalFrameSize = maxCallFrameSize;
            }

            maxCallFrameSize = 0;
            maxMStackSize = 0;

            if (label->m_stackAllocator == nullptr) {
                // Avoid cloning the allocator when a block
                // can be executed after the previous block.
                assert(!(label->info() & Label::kAfterUncondBranch));
                label->m_stackAllocator = m_stackAllocator;
                continue;
            }

            assert(label->info() & Label::kAfterUncondBranch);
            delete m_stackAllocator;
            m_stackAllocator = item->asLabel()->m_stackAllocator;
            continue;
        }

        Instruction* instr = item->asInstruction();

        // Pop params from the stack first.
        Index end = instr->paramCount();
        Operand* operand = instr->params() + end;

        for (Index i = end; i > 0; --i) {
            const LocationInfo& location = m_stackAllocator->values().back();

            --operand;

            if (location.status & LocationInfo::kIsOffset) {
                operand->value = location.value;
                operand->location.type = Operand::Stack;
            } else if (location.status & LocationInfo::kIsLocal) {
                operand->value = location.value;
                operand->location.type = Operand::Stack;
            } else if (location.status & LocationInfo::kIsRegister) {
                operand->value = location.value;
                operand->location.type = Operand::Register;
            } else if (location.status & LocationInfo::kIsCallArg) {
                operand->value = location.value;
                operand->location.type = Operand::CallArg;
            } else if (location.status & LocationInfo::kIsImmediate) {
                assert(location.value == 0);
                if (operand->item->isLabel()) {
                    // Since operand->index is unavaliable after this point,
                    // the item must point to the constant instruction.
                    const Label::DependencyList& deps = operand->item->asLabel()->dependencies(operand->index);
                    operand->item = deps[0].instr;
                }
                operand->location.type = Operand::Immediate;
            } else {
                assert((location.status & LocationInfo::kIsUnused) && location.value == 0);
                operand->location.type = Operand::Unused;
            }

            operand->location.valueInfo = location.valueInfo;
            m_stackAllocator->pop();
        }

        // Push results next.
        if (instr->hasResult()) {
            end = instr->resultCount();
            operand = instr->results();

            for (Index i = 0; i < end; ++i) {
                switch (operand->location.type) {
                case Operand::Stack: {
                    m_stackAllocator->push(operand->location.valueInfo);
                    operand->value = m_stackAllocator->values().back().value;
                    break;
                }
                case Operand::Register: {
                    m_stackAllocator->pushReg(operand->value, operand->location.valueInfo);
                    break;
                }
                case Operand::CallArg: {
                    operand->location.type = Operand::CallArg;
                    m_stackAllocator->pushCallArg(operand->value, operand->location.valueInfo);
                    break;
                }
                case Operand::Immediate: {
                    assert(instr->group() == Instruction::Immediate);
                    m_stackAllocator->pushImmediate(operand->location.valueInfo);
                    operand->location.type = Operand::Unused;
                    operand->value = 0;
                    break;
                }
                case Operand::LocalSet: {
                    operand->location.type = Operand::Stack;
                    operand->value = localsAllocator.get(operand->localIndex).value;
                    m_stackAllocator->pushUnused(operand->location.valueInfo);
                    break;
                }
                case Operand::LocalGet: {
                    assert(instr->opcode() == LocalGetOpcode);
                    operand->location.type = Operand::Unused;
                    Index offset = localsAllocator.get(instr->value().localIndex).value;
                    m_stackAllocator->pushLocal(offset, operand->location.valueInfo);
                    break;
                }
                default: {
                    assert(operand->location.type == Operand::Unused);
                    m_stackAllocator->pushUnused(operand->location.valueInfo);
                    break;
                }
                }

                operand++;
            }

            if (maxMStackSize < m_stackAllocator->size()) {
                maxMStackSize = m_stackAllocator->size();
            }
        }

        switch (instr->group()) {
        case Instruction::DirectBranch: {
            Label* label = instr->value().targetLabel;
            if ((label->info() & Label::kAfterUncondBranch) && label->m_stackAllocator == nullptr) {
                label->m_stackAllocator = cloneAllocator(label, m_stackAllocator);
            }
            break;
        }
        case Instruction::BrTable: {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;

            while (label < end) {
                if (((*label)->info() & Label::kAfterUncondBranch) && (*label)->m_stackAllocator == nullptr) {
                    (*label)->m_stackAllocator = cloneAllocator(*label, m_stackAllocator);
                }
                label++;
            }
            break;
        }
        case Instruction::Call: {
            Index frame_size = instr->asCall()->frameSize();

            if (maxCallFrameSize < frame_size) {
                maxCallFrameSize = frame_size;
            }
            break;
        }
        default: {
            break;
        }
        }
    }

    assert(m_stackAllocator->empty() || m_last->group() != Instruction::Return);
    delete m_stackAllocator;

    maxCallFrameSize += StackAllocator::alignedSize(maxMStackSize);
    if (maxCallFrameSize > totalFrameSize) {
        totalFrameSize = maxCallFrameSize;
    }

    assert(totalFrameSize >= argsSize && StackAllocator::alignedSize(totalFrameSize) == totalFrameSize);
    jitFunc->initSizes(argsSize, totalFrameSize - argsSize);

    Index argsStart = totalFrameSize - argsSize;

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            continue;
        }

        Instruction* instr = item->asInstruction();

        // Pop params from the stack first.
        Operand* operand = instr->operands();
        Operand* operandEnd = operand + instr->paramCount() + instr->resultCount();

        if (instr->group() == Instruction::LocalMove) {
            assert(operand + 1 == operandEnd);

            Index value = localsAllocator.get(instr->value().localIndex).value;

            if (value < argsSize) {
                value += argsStart;
            } else {
                Index length = LocationInfo::length(operand->location.valueInfo);

                assert(totalFrameSize >= value + length);
                value = totalFrameSize - value - length;
            }

            instr->value().value = value;
        }

        while (operand < operandEnd) {
            if (operand->location.type == Operand::CallArg) {
                operand->location.type = Operand::Stack;
            } else if (operand->location.type == Operand::Stack) {
                if (operand->value < argsSize) {
                    operand->value += argsStart;
                } else {
                    Index length = LocationInfo::length(operand->location.valueInfo);

                    assert(operand->value >= operand->value + length);
                    operand->value = totalFrameSize - operand->value - length;
                }
            }

            operand++;
        }
    }
}

} // namespace Walrus
