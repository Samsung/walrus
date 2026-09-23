/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "jit/Compiler.h"
#include "runtime/GCArray.h"

#include <algorithm>
#include <set>

namespace Walrus {

struct DependencyGenContext {
    enum Type : VariableRef {
        // Label must be 0, since labels are pointers.
        Label = Instruction::ConstPtr,
        Variable = Instruction::Register,
    };

    // Also uses: VariableList::kConstraints.
    static const uint8_t kOptReferenced = 1 << 0;
    static const uint8_t kOptDependencyComputed = 1 << 1;
    static const uint8_t kOptNeeded = 1 << 2;

    static const VariableRef kNoRef = ~(VariableRef)0;

    class DependencyList {
    public:
        DependencyList()
            : m_inline{ 0, 0 }
        {
        }

        // DependencyList is not available for copy, since it may double freed.
        DependencyList(const DependencyList&) = delete;
        DependencyList& operator=(const DependencyList&) = delete;

        DependencyList(DependencyList&& other) noexcept
            : m_inline{ other.m_inline[0], other.m_inline[1] }
        {
            other.m_inline[0] = 0;
            other.m_inline[1] = 0;
        }

        ~DependencyList()
        {
            if (isSpilled()) {
                delete spilled();
            }
        }

        size_t size() const
        {
            if (isSpilled()) {
                return spilled()->size();
            }
            return m_inline[0] == 0 ? 0 : (m_inline[1] == 0 ? 1 : 2);
        }

        class const_iterator {
        public:
            const_iterator(const VariableRef* current)
                : m_isSpilled(false)
                , m_current(current)
            {
            }

            const_iterator(std::set<VariableRef>::const_iterator current)
                : m_isSpilled(true)
                , m_current(nullptr)
                , m_spilledCurrent(current)
            {
            }

            VariableRef operator*() const { return m_isSpilled ? *m_spilledCurrent : *m_current; }

            const_iterator& operator++()
            {
                if (m_isSpilled) {
                    ++m_spilledCurrent;
                } else {
                    ++m_current;
                }
                return *this;
            }

            bool operator!=(const const_iterator& other) const
            {
                return m_isSpilled ? m_spilledCurrent != other.m_spilledCurrent : m_current != other.m_current;
            }

        private:
            bool m_isSpilled;
            const VariableRef* m_current;
            std::set<VariableRef>::const_iterator m_spilledCurrent;
        };

        const_iterator begin() const
        {
            return isSpilled() ? const_iterator(spilled()->begin()) : const_iterator(m_inline);
        }

        const_iterator end() const
        {
            return isSpilled() ? const_iterator(spilled()->end()) : const_iterator(m_inline + size());
        }

        void clear()
        {
            if (isSpilled()) {
                delete spilled();
            }
            m_inline[0] = 0;
            m_inline[1] = 0;
        }

        // Returns true when ref was not present yet.
        bool insert(VariableRef ref);

    private:
        // Label refs are pointers (low bits 0) and variable refs end in 1,
        // so 2 is free to mark a spilled set. A ref is never 0.
        static const VariableRef kSpilledTag = 2;

        bool isSpilled() const { return (m_inline[0] & 0x3) == kSpilledTag; }
        std::set<VariableRef>* spilled() const
        {
            return reinterpret_cast<std::set<VariableRef>*>(m_inline[0] & ~static_cast<VariableRef>(0x3));
        }

        VariableRef m_inline[2];
    };

    DependencyGenContext(size_t dependencySize, size_t requiredStackSize)
    {
        currentDependencies.resize(requiredStackSize);
        currentOptions.resize(requiredStackSize);

        if (dependencySize == 0) {
            // No labels in the code.
            return;
        }

        ASSERT((dependencySize % requiredStackSize) == 0);

        dependencies.resize(dependencySize);
        options.resize(dependencySize);
        maxDistance.resize(dependencySize / requiredStackSize);
    }

    void update(size_t dependencyStart, size_t id);
    void update(size_t dependencyStart, size_t id, size_t excludeStart, const TypeVector* param, VariableList* variableList, bool pushExnRef);
    void updateWithGetter(VariableList* variableList, VariableRef ref, Instruction* getter);
    void assignReference(VariableRef ref, size_t offset, uint32_t typeInfo);

    std::vector<DependencyList> dependencies;
    std::vector<uint8_t> options;
    std::vector<size_t> maxDistance;
    std::vector<VariableRef> currentDependencies;
    std::vector<uint8_t> currentOptions;
};

struct SlotData {
    static const uint8_t kUnknown = 0;
    static const uint8_t kConstant = 1;
    static const uint8_t kVariable = 2;

    SlotData()
        : value(DependencyGenContext::kNoRef)
        , rangeStart(VariableList::kRangeMax)
        , rangeEnd(0)
        , state(kUnknown)
        , constraints(0)
    {
    }

    VariableRef value;
    size_t rangeStart;
    size_t rangeEnd;
    uint8_t state;
    uint8_t constraints;
};

bool DependencyGenContext::DependencyList::insert(VariableRef ref)
{
    ASSERT(ref != 0 && (ref & 0x3) != kSpilledTag);

    if (isSpilled()) {
        return spilled()->insert(ref).second;
    }

    if (m_inline[0] == 0) {
        m_inline[0] = ref;
        return true;
    }

    if (m_inline[0] == ref) {
        return false;
    }

    if (m_inline[1] == 0) {
        if (ref < m_inline[0]) {
            m_inline[1] = m_inline[0];
            m_inline[0] = ref;
        } else {
            m_inline[1] = ref;
        }
        return true;
    }

    if (m_inline[1] == ref) {
        return false;
    }

    std::set<VariableRef>* set = new std::set<VariableRef>();

    set->insert(m_inline[0]);
    set->insert(m_inline[1]);
    set->insert(ref);

    m_inline[0] = reinterpret_cast<VariableRef>(set) | kSpilledTag;
    m_inline[1] = 0;
    return true;
}

void DependencyGenContext::update(size_t dependencyStart, size_t id)
{
    size_t size = currentDependencies.size();

    ASSERT(dependencyStart + size <= dependencies.size()
           && (dependencyStart % size) == 0
           && maxDistance[dependencyStart / size] <= id);

    maxDistance[dependencyStart / size] = id;

    for (size_t i = 0; i < size; i++) {
        VariableRef ref = currentDependencies[i];

        if (ref != 0) {
            dependencies[dependencyStart + i].insert(currentDependencies[i]);
            options[dependencyStart + i] |= currentOptions[i] & VariableList::kConstraints;
        }
    }
}

void DependencyGenContext::update(size_t dependencyStart, size_t id, size_t excludeStart, const TypeVector* param, VariableList* variableList, bool pushExnRef)
{
    size_t size = currentDependencies.size();
    size_t offset = excludeStart;

    ASSERT(dependencyStart + size <= dependencies.size()
           && (dependencyStart % size) == 0
           && maxDistance[dependencyStart / size] <= id);

    if (param != nullptr) {
        for (auto it : param->types()) {
            if (variableList != nullptr) {
                // Construct new variables.
                VariableRef ref = variableList->variables.size();

                dependencies[dependencyStart + offset].insert(VARIABLE_SET(ref, Variable));
                variableList->variables.push_back(VariableList::Variable(VARIABLE_SET(offset, Instruction::Offset), 0, id));
            }

            offset += STACK_OFFSET(valueStackAllocatedSize(it));
        }
    }

    if (pushExnRef) {
        if (variableList != nullptr) {
            // Construct new variables.
            VariableRef ref = variableList->variables.size();

            dependencies[dependencyStart + offset].insert(VARIABLE_SET(ref, Variable));
            variableList->variables.push_back(VariableList::Variable(VARIABLE_SET(offset, Instruction::Offset), 0, id));
        }

        offset += STACK_OFFSET(valueStackAllocatedSize(Value::ExnRef));
    }

    for (size_t i = 0; i < size; i++) {
        if (i >= excludeStart && i < offset) {
            continue;
        }

        VariableRef ref = currentDependencies[i];

        if (ref != 0) {
            dependencies[dependencyStart + i].insert(currentDependencies[i]);
            options[dependencyStart + i] |= currentOptions[i] & VariableList::kConstraints;
        }
    }
}

void DependencyGenContext::updateWithGetter(VariableList* variableList, VariableRef ref, Instruction* getter)
{
    VariableList::Variable& variable = variableList->variables[ref];

    if (variable.info & VariableList::kIsImmediate) {
        return;
    }

    variable.info |= currentOptions[VARIABLE_GET_REF(variable.value)] & VariableList::kConstraints;

    if (getter->id() < variable.u.rangeStart) {
        variable.u.rangeStart = getter->id();
    } else if (variable.rangeEnd < getter->id()) {
        variable.rangeEnd = getter->id();
    }
}

void DependencyGenContext::assignReference(VariableRef ref, size_t offset, uint32_t typeInfo)
{
    switch (typeInfo) {
    case Instruction::Int64Operand:
    case Instruction::Float64Operand:
        currentDependencies[offset + 1] = 0;
        currentOptions[offset + 1] = 0;
        break;

    case Instruction::V128Operand:
        currentDependencies[offset + 1] = 0;
        currentOptions[offset + 1] = 0;
        currentDependencies[offset + 2] = 0;
        currentOptions[offset + 2] = 0;
        currentDependencies[offset + 3] = 0;
        currentOptions[offset + 3] = 0;
        break;
    }

    currentDependencies[offset] = ref;
    currentOptions[offset] = 0;
}

static bool sameImmediateValue(VariableList* variableList, VariableRef first, VariableRef second)
{
    VariableList::Variable& firstVariable = variableList->variables[first];
    VariableList::Variable& secondVariable = variableList->variables[second];

    if (!(firstVariable.info & VariableList::kIsImmediate) || !(secondVariable.info & VariableList::kIsImmediate)) {
        return false;
    }

    Instruction* firstInstr = firstVariable.u.immediate;
    Instruction* secondInstr = secondVariable.u.immediate;

    if (firstInstr->opcode() != secondInstr->opcode()) {
        return false;
    }

    switch (firstInstr->opcode()) {
    case ByteCode::Const32Opcode:
        return reinterpret_cast<Const32*>(firstInstr->byteCode())->value() == reinterpret_cast<Const32*>(secondInstr->byteCode())->value();
    case ByteCode::Const64Opcode:
        return reinterpret_cast<Const64*>(firstInstr->byteCode())->value() == reinterpret_cast<Const64*>(secondInstr->byteCode())->value();
    default:
        ASSERT(firstInstr->opcode() == ByteCode::Const128Opcode);
        return memcmp(reinterpret_cast<Const128*>(firstInstr->byteCode())->value(),
                      reinterpret_cast<Const128*>(secondInstr->byteCode())->value(), 16)
            == 0;
    }
}

static VariableRef mergeVariables(VariableList* variableList, VariableRef head, VariableRef other)
{
    ASSERT(!(variableList->variables[head].info & VariableList::kIsMerged));

    other = variableList->getMergeHead(other);

    if (head == other) {
        return head;
    }

    if (UNLIKELY(head > other)) {
        VariableRef tmp = head;
        head = other;
        other = tmp;
    }

    VariableList::Variable& variableHead = variableList->variables[head];
    VariableList::Variable& variableOther = variableList->variables[other];

    if (variableHead.info & VariableList::kIsImmediate) {
        variableHead.info -= VariableList::kIsImmediate;
        variableHead.u.rangeStart = variableHead.rangeEnd;
    }

    if (variableOther.info & VariableList::kIsImmediate) {
        variableOther.info -= VariableList::kIsImmediate;
        variableOther.u.rangeStart = variableOther.rangeEnd;
    }

    if (variableOther.u.rangeStart < variableHead.u.rangeStart) {
        variableHead.u.rangeStart = variableOther.u.rangeStart;
    }

    if (variableHead.rangeEnd < variableOther.rangeEnd) {
        variableHead.rangeEnd = variableOther.rangeEnd;
    }

    variableHead.info |= variableOther.info & (VariableList::kConstraints | Instruction::TypeMask);

    variableOther.u.parent = head;
    variableOther.info |= VariableList::kIsMerged;
    return head;
}

#define VARIABLE_GET_LABEL(v) (reinterpret_cast<InstructionListItem*>(v)->asLabel())

void JITCompiler::buildVariables(uint32_t requiredStackSize)
{
    ASSERT_STATIC(Instruction::Int32Operand < Instruction::Float32Operand
                      && (Instruction::Int32Operand | Instruction::Float32Operand) == Instruction::Float32Operand,
                  "Coverting Int32Operand to Float32Operand should be possible");
    ASSERT_STATIC(Instruction::Int64Operand < Instruction::Float64Operand
                      && (Instruction::Int64Operand | Instruction::Float64Operand) == Instruction::Float64Operand,
                  "Coverting Int64Operand to Float64Operand should be possible");

    size_t variableCount = requiredStackSize;
    size_t dependencySize = 0;
    size_t nextId = 0;
    size_t nextTryBlock = m_tryBlockStart;
    size_t currentTryBlock = Label::kNoTryBlock;
    std::vector<size_t> tryBlockStack;

    // Create variables for each result or external values.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        item->m_id = ++nextId;

        if (item->isLabel()) {
            Label* label = item->asLabel();

            label->m_dependencyStart = dependencySize;
            dependencySize += requiredStackSize;

            ASSERT((label->info() & (Label::kHasTryInfo | Label::kHasCatchInfo)) != (Label::kHasTryInfo | Label::kHasCatchInfo));

            if (label->info() & Label::kHasCatchInfo) {
                ASSERT(tryBlocks()[currentTryBlock].catchBlocks[0].u.handler == label);

                label->m_handlerOfTryBlock = currentTryBlock;
                currentTryBlock = tryBlockStack.back();
                tryBlockStack.pop_back();
            }

            if (label->info() & Label::kHasTryInfo) {
                ASSERT(tryBlocks()[nextTryBlock].start == label);

                do {
                    for (auto it : tryBlocks()[nextTryBlock].catchBlocks) {
                        if (it.tagIndex != std::numeric_limits<uint32_t>::max()) {
                            TagType* tagType = module()->tagType(it.tagIndex);
                            variableCount += tagType->functionType()->param().size();
                        }
                        if (it.pushExnRef) {
                            variableCount++;
                        }
                    }

                    tryBlocks()[nextTryBlock].parent = currentTryBlock;
                    tryBlockStack.push_back(currentTryBlock);
                    currentTryBlock = nextTryBlock++;
                } while (nextTryBlock < tryBlocks().size()
                         && tryBlocks()[nextTryBlock].start == label);
            }

            label->m_tryBlock = currentTryBlock;
        } else {
            variableCount += item->asInstruction()->resultCount();
        }
    }

    ASSERT(tryBlockStack.empty() && currentTryBlock == Label::kNoTryBlock);

    if (requiredStackSize == 0) {
        return;
    }

    DependencyGenContext dependencyCtx(dependencySize, requiredStackSize);
    InstructionListItem* fallThrough = nullptr;
    std::vector<size_t> activeTryBlocks;

    m_variableList = new VariableList(variableCount, requiredStackSize);
    nextTryBlock = m_tryBlockStart;

    for (uint32_t i = 0; i < requiredStackSize; i++) {
        m_variableList->variables.push_back(VariableList::Variable(VARIABLE_SET(i, Instruction::Offset), 0, static_cast<size_t>(0)));
        dependencyCtx.currentDependencies[i] = VARIABLE_SET(i, DependencyGenContext::Variable);
        dependencyCtx.currentOptions[i] = 0;
    }

    const TypeVector::Types& paramTypeInfo = moduleFunction()->functionType()->param().types();
    size_t argc = paramTypeInfo.size();
    size_t offsetIndex = 0;
    for (size_t i = 0; i < argc; i++) {
        m_variableList->variables[offsetIndex].info |= Instruction::valueTypeToOperandType(paramTypeInfo[i]);
        offsetIndex += STACK_OFFSET(valueStackAllocatedSize(paramTypeInfo[i]));
    }

    // Phase 1: the direct dependencies are computed for instructions
    // and labels (only labels can have multiple dependencies).
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            // Build a dependency list which refers to the last label.
            Label* label = item->asLabel();

            if (fallThrough != nullptr) {
                ExtendedInstruction* jump = ExtendedInstruction::create(nullptr, Instruction::DirectBranch, ByteCode::JumpOpcode, 0, 0);

                jump->m_id = label->id();
                jump->value().targetLabel = label;
                label->m_branches.push_back(jump);

                jump->m_next = item;
                fallThrough->m_next = jump;
            }

            if (fallThrough != nullptr || item == m_first) {
                dependencyCtx.update(label->m_dependencyStart, label->id());
            } else {
                dependencyCtx.maxDistance[label->m_dependencyStart / requiredStackSize] = label->id();
            }

            if (label->info() & Label::kHasTryInfo) {
                ASSERT(tryBlocks()[nextTryBlock].start == label);

                do {
                    for (auto it : tryBlocks()[nextTryBlock].catchBlocks) {
                        // Forward jump.
                        Label* catchLabel = it.u.handler;
                        if (it.tagIndex == std::numeric_limits<uint32_t>::max()) {
                            dependencyCtx.update(catchLabel->m_dependencyStart, catchLabel->id(),
                                                 STACK_OFFSET(it.stackSizeToBe), nullptr, m_variableList, it.pushExnRef);
                        } else {
                            TagType* tagType = module()->tagType(it.tagIndex);
                            const TypeVector& param = tagType->functionType()->param();

                            m_variableList->pushCatchUpdate(catchLabel, param.size() + (it.pushExnRef ? 1 : 0));
                            dependencyCtx.update(catchLabel->m_dependencyStart, catchLabel->id(),
                                                 STACK_OFFSET(it.stackSizeToBe), &param, m_variableList, it.pushExnRef);
                        }
                    }

                    activeTryBlocks.push_back(nextTryBlock);
                    nextTryBlock++;
                } while (nextTryBlock < tryBlocks().size()
                         && tryBlocks()[nextTryBlock].start == label);
            }

            if (label->info() & Label::kHasCatchInfo) {
                activeTryBlocks.pop_back();
            }

            for (size_t i = 0; i < requiredStackSize; ++i) {
                dependencyCtx.currentDependencies[i] = VARIABLE_SET_PTR(label);
                dependencyCtx.currentOptions[i] = 0;
            }

            fallThrough = label;
            continue;
        }

        Instruction* instr = item->asInstruction();
        Operand* operand = instr->operands();
        Operand* end = operand + instr->paramCount();

        fallThrough = instr;

        while (operand < end) {
            VariableRef ref = dependencyCtx.currentDependencies[*operand];

            if (VARIABLE_TYPE(ref) == DependencyGenContext::Label) {
                size_t offset = VARIABLE_GET_LABEL(ref)->m_dependencyStart + *operand;
                dependencyCtx.options[offset] |= DependencyGenContext::kOptReferenced | (dependencyCtx.currentOptions[*operand] & VariableList::kConstraints);

                ref = VARIABLE_SET(*operand, DependencyGenContext::Label);
            } else {
                ASSERT(VARIABLE_TYPE(ref) == DependencyGenContext::Variable);
                dependencyCtx.updateWithGetter(m_variableList, VARIABLE_GET_REF(ref), instr);
            }

            *operand++ = ref;
        }

        if (instr->group() == Instruction::DirectBranch) {
            Label* label = instr->asExtended()->value().targetLabel;
            dependencyCtx.update(label->m_dependencyStart, instr->id());

            if (instr->opcode() == ByteCode::JumpOpcode) {
                fallThrough = nullptr;
            }
            continue;
        }

        if (instr->group() == Instruction::BrTable) {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->asBrTable()->targetLabelCount();
            std::set<Label*> updatedLabels;

            while (label < end) {
                if (updatedLabels.insert(*label).second) {
                    dependencyCtx.update((*label)->m_dependencyStart, instr->id());
                }
                label++;
            }
            fallThrough = nullptr;
            continue;
        }

        if (instr->info() & Instruction::kIsCallback) {
            for (size_t i = 0; i < requiredStackSize; i++) {
                dependencyCtx.currentOptions[i] |= VariableList::kIsCallback;
            }
        }

        if (activeTryBlocks.size() > 0
            && (instr->group() == Instruction::Call || instr->opcode() == ByteCode::ThrowOpcode || instr->opcode() == ByteCode::ThrowRefOpcode)) {
            // Every call or throw may jump to any active catch block. Future
            // optimizations could reduce these (e.g. a throw can be converted
            // to a jump if its target catch block is in the same function).
            for (auto blockIt : activeTryBlocks) {
                for (auto it : tryBlocks()[blockIt].catchBlocks) {
                    Label* catchLabel = it.u.handler;
                    if (it.tagIndex == std::numeric_limits<uint32_t>::max()) {
                        dependencyCtx.update(catchLabel->m_dependencyStart, catchLabel->id(),
                                             STACK_OFFSET(it.stackSizeToBe), nullptr, nullptr, it.pushExnRef);
                    } else {
                        TagType* tagType = module()->tagType(it.tagIndex);
                        const TypeVector& param = tagType->functionType()->param();

                        dependencyCtx.update(catchLabel->m_dependencyStart, catchLabel->id(),
                                             STACK_OFFSET(it.stackSizeToBe), &param, nullptr, it.pushExnRef);
                    }
                }
            }
        }

        if (instr->opcode() == ByteCode::ThrowOpcode || instr->opcode() == ByteCode::ThrowRefOpcode
            || instr->opcode() == ByteCode::UnreachableOpcode || instr->opcode() == ByteCode::EndOpcode) {
            fallThrough = nullptr;
            continue;
        }

        if (instr->info() & Instruction::kDestroysR0R1) {
            for (size_t i = 0; i < requiredStackSize; i++) {
                dependencyCtx.currentOptions[i] |= VariableList::kDestroysR0R1;
            }
        }

        uint32_t resultCount = instr->resultCount();

        if (resultCount == 0) {
            continue;
        }

        if (instr->group() != Instruction::Call) {
            ASSERT(resultCount == 1);

            const uint8_t* list = instr->getOperandDescriptor();
            ASSERT(list != 0);

            VariableRef ref = VARIABLE_SET(m_variableList->variables.size(), DependencyGenContext::Variable);
            uint32_t typeInfo = list[instr->paramCount()] & Instruction::TypeMask;

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            if (typeInfo == Instruction::Int64LowOperand) {
                typeInfo = Instruction::Int64Operand;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */

            VariableRef value = VARIABLE_SET(*operand, Instruction::Offset);
            m_variableList->variables.push_back(VariableList::Variable(value, typeInfo, instr));
            dependencyCtx.assignReference(ref, *operand, typeInfo);

            *operand = ref;
            continue;
        }

        FunctionType* functionType;
        switch (instr->opcode()) {
        case ByteCode::CallOpcode: {
            Call* call = reinterpret_cast<Call*>(instr->byteCode());
            functionType = module()->function(call->index())->functionType();
            break;
        }
        case ByteCode::ReturnCallOpcode: {
            ReturnCall* call = reinterpret_cast<ReturnCall*>(instr->byteCode());
            functionType = module()->function(call->index())->functionType();
            fallThrough = nullptr;
            break;
        }
        case ByteCode::ReturnCallIndirectOpcode:
        case ByteCode::ReturnCallIndirectM64Opcode:
            fallThrough = nullptr;
            FALLTHROUGH;
        case ByteCode::CallIndirectOpcode:
        case ByteCode::CallIndirectM64Opcode: {
            CallTable* callTable = reinterpret_cast<CallTable*>(instr->byteCode());
            functionType = callTable->functionType();
            break;
        }
        case ByteCode::CallRefOpcode: {
            CallRef* callRef = reinterpret_cast<CallRef*>(instr->byteCode());
            functionType = callRef->functionType();
            break;
        }
        default: {
            ASSERT(instr->opcode() == ByteCode::ReturnCallRefOpcode);
            ReturnCallRef* callRef = reinterpret_cast<ReturnCallRef*>(instr->byteCode());
            functionType = callRef->functionType();
            fallThrough = nullptr;
            break;
        }
        }

        ASSERT(functionType->result().size() == resultCount);

        size_t id = instr->id();

        for (auto it : functionType->result().types()) {
            VariableRef ref = VARIABLE_SET(m_variableList->variables.size(), DependencyGenContext::Variable);
            uint32_t typeInfo = Instruction::valueTypeToOperandType(it);

            m_variableList->variables.push_back(VariableList::Variable(VARIABLE_SET(*operand, Instruction::Offset), typeInfo, id));
            dependencyCtx.assignReference(ref, *operand, typeInfo);
            *operand++ = ref;
        }
    }

    ASSERT(variableCount == m_variableList->variables.size());
    ASSERT(activeTryBlocks.size() == 0);

    // Phase 2: the indirect instruction
    // references are computed for labels.

    std::vector<SlotData> slotData(dependencySize);
    std::vector<size_t> pending;

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (!item->isLabel()) {
            continue;
        }

        size_t dependencyStart = item->asLabel()->m_dependencyStart;
        size_t end = dependencyStart + requiredStackSize;

        for (size_t i = dependencyStart; i < end; ++i) {
            if (dependencyCtx.options[i] & DependencyGenContext::kOptReferenced) {
                dependencyCtx.options[i] |= DependencyGenContext::kOptNeeded;
                pending.push_back(i);
            }
        }
    }

    std::vector<size_t> needed;

    while (!pending.empty()) {
        size_t index = pending.back();
        size_t slot = index % requiredStackSize;

        pending.pop_back();
        needed.push_back(index);

        for (auto it : dependencyCtx.dependencies[index]) {
            if (VARIABLE_TYPE(it) != DependencyGenContext::Label) {
                continue;
            }

            size_t source = VARIABLE_GET_LABEL(it)->m_dependencyStart + slot;

            if (!(dependencyCtx.options[source] & DependencyGenContext::kOptNeeded)) {
                dependencyCtx.options[source] |= DependencyGenContext::kOptNeeded;
                pending.push_back(source);
            }
        }
    }

    std::sort(needed.begin(), needed.end());

    bool changed;

    do {
        changed = false;

        for (auto index : needed) {
            SlotData& data = slotData[index];

            if (data.state == SlotData::kVariable) {
                continue;
            }

            size_t slot = index % requiredStackSize;
            uint8_t state = SlotData::kUnknown;
            VariableRef value = DependencyGenContext::kNoRef;

            for (auto it : dependencyCtx.dependencies[index]) {
                VariableRef candidate;

                if (VARIABLE_TYPE(it) == DependencyGenContext::Label) {
                    SlotData& source = slotData[VARIABLE_GET_LABEL(it)->m_dependencyStart + slot];

                    if (source.state == SlotData::kUnknown) {
                        continue;
                    }

                    if (source.state == SlotData::kVariable) {
                        state = SlotData::kVariable;
                        break;
                    }

                    candidate = source.value;
                } else {
                    candidate = VARIABLE_GET_REF(it);

                    if (!(m_variableList->variables[candidate].info & VariableList::kIsImmediate)) {
                        state = SlotData::kVariable;
                        break;
                    }
                }

                if (state == SlotData::kUnknown) {
                    state = SlotData::kConstant;
                    value = candidate;
                } else if (!sameImmediateValue(m_variableList, value, candidate)) {
                    state = SlotData::kVariable;
                    break;
                }
            }

            if (state != data.state) {
                data.state = state;
                data.value = state == SlotData::kConstant ? value : DependencyGenContext::kNoRef;
                changed = true;
            }
        }

        for (auto index : needed) {
            if (slotData[index].state != SlotData::kVariable) {
                continue;
            }

            size_t slot = index % requiredStackSize;

            for (auto it : dependencyCtx.dependencies[index]) {
                if (VARIABLE_TYPE(it) != DependencyGenContext::Label) {
                    continue;
                }

                SlotData& source = slotData[VARIABLE_GET_LABEL(it)->m_dependencyStart + slot];

                if (source.state != SlotData::kVariable) {
                    source.state = SlotData::kVariable;
                    source.value = DependencyGenContext::kNoRef;
                    changed = true;
                }
            }
        }
    } while (changed);

    do {
        changed = false;

        for (auto index : needed) {
            SlotData& data = slotData[index];

            if (data.state != SlotData::kVariable) {
                continue;
            }

            size_t slot = index % requiredStackSize;
            VariableRef head = data.value;
            size_t rangeStart = data.rangeStart;
            size_t rangeEnd = data.rangeEnd;
            uint8_t constraints = data.constraints;

            if (head != DependencyGenContext::kNoRef) {
                head = m_variableList->getMergeHead(head);
            }

            for (auto it : dependencyCtx.dependencies[index]) {
                VariableRef candidate;

                if (VARIABLE_TYPE(it) == DependencyGenContext::Label) {
                    Label* label = VARIABLE_GET_LABEL(it);
                    size_t sourceIndex = label->m_dependencyStart + slot;
                    SlotData& source = slotData[sourceIndex];
                    size_t sourceEnd = dependencyCtx.maxDistance[label->m_dependencyStart / requiredStackSize];

                    if (rangeStart > label->id()) {
                        rangeStart = label->id();
                    }

                    if (rangeEnd < sourceEnd) {
                        rangeEnd = sourceEnd;
                    }

                    constraints |= dependencyCtx.options[sourceIndex] & VariableList::kConstraints;

                    if (rangeStart > source.rangeStart) {
                        rangeStart = source.rangeStart;
                    }

                    if (rangeEnd < source.rangeEnd) {
                        rangeEnd = source.rangeEnd;
                    }

                    constraints |= source.constraints;
                    candidate = source.value;

                    if (candidate == DependencyGenContext::kNoRef) {
                        continue;
                    }
                } else {
                    candidate = VARIABLE_GET_REF(it);
                }

                head = (head == DependencyGenContext::kNoRef)
                    ? m_variableList->getMergeHead(candidate)
                    : mergeVariables(m_variableList, head, candidate);
            }

            if (head == DependencyGenContext::kNoRef) {
                continue;
            }

            if (data.value != head || data.rangeStart != rangeStart
                || data.rangeEnd != rangeEnd || data.constraints != constraints) {
                changed = true;
            }

            data.value = head;
            data.rangeStart = rangeStart;
            data.rangeEnd = rangeEnd;
            data.constraints = constraints;

            size_t ownEnd = dependencyCtx.maxDistance[index / requiredStackSize];

            if (rangeEnd < ownEnd) {
                rangeEnd = ownEnd;
            }

            VariableList::Variable& variable = m_variableList->variables[head];

            if (variable.info & VariableList::kIsImmediate) {
                variable.info -= VariableList::kIsImmediate;
                variable.u.rangeStart = variable.rangeEnd;
            }

            variable.info |= (constraints | dependencyCtx.options[index]) & VariableList::kConstraints;

            if (variable.u.rangeStart > rangeStart) {
                variable.u.rangeStart = rangeStart;
            }

            if (variable.rangeEnd < rangeEnd) {
                variable.rangeEnd = rangeEnd;
            }
        }
    } while (changed);

    for (auto index : needed) {
        if (!(dependencyCtx.options[index] & DependencyGenContext::kOptReferenced)) {
            continue;
        }

        SlotData& data = slotData[index];

        ASSERT(data.value != DependencyGenContext::kNoRef);

        DependencyGenContext::DependencyList& dependencies = dependencyCtx.dependencies[index];

        dependencies.clear();
        dependencies.insert(VARIABLE_SET(data.value, DependencyGenContext::Variable));
        dependencyCtx.options[index] |= DependencyGenContext::kOptDependencyComputed;
    }

    // Cleanup
    size_t lastDependencyStart = 0;

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            lastDependencyStart = item->asLabel()->m_dependencyStart;
            continue;
        }

        Instruction* instr = item->asInstruction();
        Operand* param = instr->params();
        Operand* end = param + instr->paramCount() + instr->resultCount();

        while (param < end) {
            VariableRef ref = *param;

            if (VARIABLE_TYPE(ref) == DependencyGenContext::Label) {
                size_t offset = lastDependencyStart + VARIABLE_GET_REF(ref);
                DependencyGenContext::DependencyList& list = dependencyCtx.dependencies[offset];

                ASSERT((dependencyCtx.options[offset] & DependencyGenContext::kOptDependencyComputed) && list.size() == 1);

                ref = *list.begin();

                ASSERT(VARIABLE_TYPE(ref) == DependencyGenContext::Variable);

                ref = m_variableList->getMergeHead(VARIABLE_GET_REF(ref));
                VariableList::Variable& variable = m_variableList->variables[ref];

                if (!(variable.info & VariableList::kIsImmediate)) {
                    ASSERT(variable.u.rangeStart < instr->id());

                    if (variable.rangeEnd < instr->id()) {
                        variable.rangeEnd = instr->id();
                    }
                }

                *param = ref;
            } else {
                ASSERT(VARIABLE_TYPE(ref) == DependencyGenContext::Variable);
                *param = m_variableList->getMergeHead(VARIABLE_GET_REF(ref));
            }

            param++;
        }

        if (instr->paramCount() > 0) {
            // Force float type for variables which input needs to be float.
            const uint8_t* list = instr->getOperandDescriptor();
            param = instr->params();

            if (*list != 0) {
                end = param + instr->paramCount();

                do {
                    VariableList::Variable& variable = m_variableList->variables[*param++];
                    variable.info |= (*list & Instruction::TypeMask);
                    list++;
                } while (param < end);
            } else {
                switch (instr->opcode()) {
                case ByteCode::StructNewOpcode: {
                    for (auto it : reinterpret_cast<StructNew*>(instr->byteCode())->typeInfo()->fields().types()) {
                        VariableList::Variable& variable = m_variableList->variables[*param++];
                        variable.info |= Instruction::valueTypeToOperandType(Value::unpackType(it.type()));
                    }
                    break;
                }
                case ByteCode::ArrayNewFixedOpcode: {
                    Value::Type type = reinterpret_cast<ArrayNewFixed*>(instr->byteCode())->typeInfo()->field().type();
                    uint32_t info = Instruction::valueTypeToOperandType(Value::unpackType(type));
                    end = param + instr->paramCount();

                    while (param < end) {
                        VariableList::Variable& variable = m_variableList->variables[*param++];
                        variable.info |= info;
                    }
                    break;
                }
                case ByteCode::ArrayCopyOpcode: {
                    ASSERT(instr->paramCount() == 5);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                    uint8_t refSize = Instruction::Int32Operand;
#else /* !SLJIT_32BIT_ARCHITECTURE */
                    uint8_t refSize = Instruction::Int64Operand;
#endif /* SLJIT_32BIT_ARCHITECTURE */
                    m_variableList->variables[param[0]].info |= refSize;
                    m_variableList->variables[param[1]].info |= Instruction::Int32Operand;
                    m_variableList->variables[param[2]].info |= refSize;
                    m_variableList->variables[param[3]].info |= Instruction::Int32Operand;
                    m_variableList->variables[param[4]].info |= Instruction::Int32Operand;
                    break;
                }
                case ByteCode::ArrayFillOpcode: {
                    ASSERT(instr->paramCount() == 4);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                    m_variableList->variables[param[0]].info |= Instruction::Int32Operand;
#else /* !SLJIT_32BIT_ARCHITECTURE */
                    m_variableList->variables[param[0]].info |= Instruction::Int64Operand;
#endif /* SLJIT_32BIT_ARCHITECTURE */
                    m_variableList->variables[param[1]].info |= Instruction::Int32Operand;
                    m_variableList->variables[param[2]].info |= Instruction::valueTypeToOperandType(Value::unpackType(reinterpret_cast<ArrayFill*>(instr->byteCode())->type()));
                    m_variableList->variables[param[3]].info |= Instruction::Int32Operand;
                    break;
                }
                case ByteCode::ArrayInitDataOpcode:
                case ByteCode::ArrayInitElemOpcode: {
                    ASSERT(instr->paramCount() == 4);
                    VariableList::Variable& variable = m_variableList->variables[*param++];
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                    variable.info |= Instruction::Int32Operand;
#else /* !SLJIT_32BIT_ARCHITECTURE */
                    variable.info |= Instruction::Int64Operand;
#endif /* SLJIT_32BIT_ARCHITECTURE */
                    for (int i = 1; i < 4; i++) {
                        VariableList::Variable& variable = m_variableList->variables[*param++];
                        variable.info |= Instruction::Int32Operand;
                    }
                    break;
                }
                default: {
                    const TypeVector* types = nullptr;
                    uint16_t calleeInfo = 0;

                    switch (instr->opcode()) {
                    case ByteCode::CallOpcode: {
                        Call* call = reinterpret_cast<Call*>(instr->byteCode());
                        types = &module()->function(call->index())->functionType()->param();
                        break;
                    }
                    case ByteCode::ReturnCallOpcode: {
                        ReturnCall* call = reinterpret_cast<ReturnCall*>(instr->byteCode());
                        types = &module()->function(call->index())->functionType()->param();
                        break;
                    }
                    case ByteCode::CallIndirectOpcode:
                    case ByteCode::ReturnCallIndirectOpcode:
                        calleeInfo = Instruction::Int32Operand;
                        FALLTHROUGH;
                    case ByteCode::CallIndirectM64Opcode:
                    case ByteCode::ReturnCallIndirectM64Opcode: {
                        CallTable* callTable = reinterpret_cast<CallTable*>(instr->byteCode());
                        types = &callTable->functionType()->param();
                        if (calleeInfo == 0) {
                            calleeInfo = Instruction::Int64Operand;
                        }
                        break;
                    }
                    case ByteCode::CallRefOpcode: {
                        CallRef* callRef = reinterpret_cast<CallRef*>(instr->byteCode());
                        types = &callRef->functionType()->param();
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
                        calleeInfo = Instruction::Int64Operand;
#else /* !SLJIT_64BIT_ARCHITECTURE */
                        calleeInfo = Instruction::Int32Operand;
#endif /* SLJIT_64BIT_ARCHITECTURE */
                        break;
                    }
                    case ByteCode::ReturnCallRefOpcode: {
                        ReturnCallRef* callRef = reinterpret_cast<ReturnCallRef*>(instr->byteCode());
                        types = &callRef->functionType()->param();
                        break;
                    }
                    case ByteCode::ThrowOpcode: {
                        Throw* throwTag = reinterpret_cast<Throw*>(instr->byteCode());
                        TagType* tagType = module()->tagType(throwTag->tagIndex());
                        types = &tagType->functionType()->param();
                        break;
                    }
                    default: {
                        ASSERT(instr->opcode() == ByteCode::EndOpcode);
                        types = &moduleFunction()->functionType()->result();
                        break;
                    }
                    }

                    for (auto it : types->types()) {
                        VariableList::Variable& variable = m_variableList->variables[*param++];
                        variable.info |= Instruction::valueTypeToOperandType(it);
                    }

                    if (calleeInfo != 0) {
                        m_variableList->variables[*param].info |= calleeInfo;
                    }
                }
                }
            }
        }

        if (instr->group() == Instruction::Immediate) {
            VariableList::Variable& variable = m_variableList->variables[*instr->getResult(0)];

            if (variable.info & VariableList::kIsImmediate) {
                ASSERT(!(variable.info & VariableList::kIsMerged) && variable.u.immediate == instr);
                variable.value = VARIABLE_SET_PTR(instr);
            } else {
                instr->addInfo(Instruction::kKeepInstruction);
            }
        }

        if (!(instr->info() & Instruction::kIsMergeCompare)) {
            continue;
        }

        if (!instr->next()->isInstruction()) {
            instr->clearInfo(Instruction::kIsMergeCompare);
            continue;
        }

        VariableRef ref = *instr->getResult(0);
        VariableList::Variable& variable = m_variableList->variables[ref];
        Instruction* nextInstr = instr->next()->asInstruction();
        bool dropVariable;
        Operand refAsVariable = VARIABLE_SET(ref, DependencyGenContext::Variable);

        ASSERT((variable.info & Instruction::TypeMask) == Instruction::Int32Operand);

        switch (nextInstr->opcode()) {
        case ByteCode::JumpIfTrueOpcode:
        case ByteCode::JumpIfFalseOpcode:
            // These instructions have only one argument.
            if (*nextInstr->getParam(0) == refAsVariable) {
                dropVariable = (variable.u.rangeStart == instr->id() && variable.rangeEnd == instr->id() + 1);
                break;
            }

            instr->clearInfo(Instruction::kIsMergeCompare);
            continue;
        case ByteCode::SelectOpcode:
            if (*nextInstr->getParam(2) == refAsVariable) {
                dropVariable = (variable.u.rangeStart == instr->id()
                                && variable.rangeEnd == instr->id() + 1
                                && *nextInstr->getParam(0) != refAsVariable
                                && *nextInstr->getParam(1) != refAsVariable);
                break;
            }
            FALLTHROUGH;
        default:
            instr->clearInfo(Instruction::kIsMergeCompare);
            continue;
        }

        if (dropVariable) {
            variable.info |= VariableList::kIsImmediate;
            variable.value = VARIABLE_SET_PTR(nullptr);
        }

        if (instr->group() == Instruction::Binary) {
            instr->convertBinaryToCompare();
        }
    }
}

} // namespace Walrus

#endif // WALRUS_ENABLE_JIT
