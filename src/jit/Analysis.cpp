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

#include "Walrus.h"

#include "jit/Compiler.h"

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

    static const VariableRef kNoRef = ~(VariableRef)0;

    typedef std::set<VariableRef> DependencyList;

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
    void update(size_t dependencyStart, size_t id, size_t excludeStart, const ValueTypeVector& param, VariableList* variableList);
    void updateWithGetter(VariableList* variableList, VariableRef ref, Instruction* getter);
    void assignReference(VariableRef ref, size_t offset, uint32_t typeInfo);

    std::vector<DependencyList> dependencies;
    std::vector<uint8_t> options;
    std::vector<size_t> maxDistance;
    std::vector<VariableRef> currentDependencies;
    std::vector<uint8_t> currentOptions;
};

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

void DependencyGenContext::update(size_t dependencyStart, size_t id, size_t excludeStart, const ValueTypeVector& param, VariableList* variableList)
{
    size_t size = currentDependencies.size();
    size_t offset = excludeStart;

    ASSERT(dependencyStart + size <= dependencies.size()
           && (dependencyStart % size) == 0
           && maxDistance[dependencyStart / size] <= id);

    for (auto it : param) {
        if (variableList != nullptr) {
            // Construct new variables.
            VariableRef ref = variableList->variables.size();

            dependencies[dependencyStart + offset].insert(VARIABLE_SET(ref, Variable));
            variableList->variables.push_back(VariableList::Variable(VARIABLE_SET(offset, Instruction::Offset), 0, id));
        }

        offset += STACK_OFFSET(valueStackAllocatedSize(it));
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

static bool checkSameConst(VariableList* variableList, std::set<VariableRef>& dependencies)
{
    VariableRef constRef = 0;
    Instruction* constInstr = nullptr;
    uint32_t value32 = 0;
    uint32_t value64 = 0;
    const uint8_t* value128 = nullptr;

    for (auto it : dependencies) {
        if (VARIABLE_TYPE(it) == DependencyGenContext::Label) {
            continue;
        }

        ASSERT(VARIABLE_TYPE(it) == DependencyGenContext::Variable);

        VariableList::Variable& variable = variableList->variables[VARIABLE_GET_REF(it)];

        if (!(variable.info & VariableList::kIsImmediate)) {
            return false;
        }

        Instruction* instr = variable.u.immediate;

        if (constInstr == nullptr) {
            constRef = it;
            constInstr = instr;

            switch (constInstr->opcode()) {
            case ByteCode::Const32Opcode:
                value32 = reinterpret_cast<Const32*>(constInstr->byteCode())->value();
                break;
            case ByteCode::Const64Opcode:
                value64 = reinterpret_cast<Const64*>(constInstr->byteCode())->value();
                break;
            default:
                ASSERT(constInstr->opcode() == ByteCode::Const128Opcode);
                value128 = reinterpret_cast<Const128*>(constInstr->byteCode())->value();
                break;
            }
            continue;
        }

        ASSERT(instr->opcode() == constInstr->opcode());

        switch (constInstr->opcode()) {
        case ByteCode::Const32Opcode:
            if (reinterpret_cast<Const32*>(instr->byteCode())->value() != value32) {
                return false;
            }
            break;
        case ByteCode::Const64Opcode:
            if (reinterpret_cast<Const64*>(instr->byteCode())->value() != value64) {
                return false;
            }
            break;
        default:
            ASSERT(constInstr->opcode() == ByteCode::Const128Opcode);
            if (memcmp(reinterpret_cast<Const128*>(instr->byteCode())->value(), value128, 16) != 0) {
                return false;
            }
            break;
        }
    }

    dependencies.clear();
    dependencies.insert(constRef);
    return true;
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

    // Create variables for each result or external values.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        item->m_id = ++nextId;

        if (item->isLabel()) {
            Label* label = item->asLabel();

            label->m_dependencyStart = dependencySize;
            dependencySize += requiredStackSize;

            if (label->info() & Label::kHasTryInfo) {
                ASSERT(tryBlocks()[nextTryBlock].start == label);

                do {
                    for (auto it : tryBlocks()[nextTryBlock].catchBlocks) {
                        if (it.tagIndex != std::numeric_limits<uint32_t>::max()) {
                            TagType* tagType = module()->tagType(it.tagIndex);
                            variableCount += module()->functionType(tagType->sigIndex())->param().size();
                        }
                    }

                    nextTryBlock++;
                } while (nextTryBlock < tryBlocks().size()
                         && tryBlocks()[nextTryBlock].start == label);
            }
        } else {
            variableCount += item->asInstruction()->resultCount();
        }
    }

    if (requiredStackSize == 0) {
        return;
    }

    DependencyGenContext dependencyCtx(dependencySize, requiredStackSize);
    bool updateDeps = true;
    std::vector<size_t> activeTryBlocks;

    m_variableList = new VariableList(variableCount, requiredStackSize);
    nextTryBlock = m_tryBlockStart;

    for (uint32_t i = 0; i < requiredStackSize; i++) {
        m_variableList->variables.push_back(VariableList::Variable(VARIABLE_SET(i, Instruction::Offset), 0, static_cast<size_t>(0)));
        dependencyCtx.currentDependencies[i] = VARIABLE_SET(i, DependencyGenContext::Variable);
        dependencyCtx.currentOptions[i] = 0;
    }

    // Phase 1: the direct dependencies are computed for instructions
    // and labels (only labels can have multiple dependencies).
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            // Build a dependency list which refers to the last label.
            Label* label = item->asLabel();

            if (updateDeps) {
                dependencyCtx.update(label->m_dependencyStart, label->id());
            } else {
                dependencyCtx.maxDistance[label->m_dependencyStart / requiredStackSize] = label->id();
            }

            if (label->info() & Label::kHasTryInfo) {
                ASSERT(tryBlocks()[nextTryBlock].start == label);

                do {
                    for (auto it : tryBlocks()[nextTryBlock].catchBlocks) {
                        // Forward jump.
                        if (it.tagIndex == std::numeric_limits<uint32_t>::max()) {
                            dependencyCtx.update(it.u.handler->m_dependencyStart, label->id());
                        } else {
                            TagType* tagType = module()->tagType(it.tagIndex);
                            const ValueTypeVector& param = module()->functionType(tagType->sigIndex())->param();
                            Label* catchLabel = it.u.handler;

                            m_variableList->pushCatchUpdate(catchLabel, param.size());

                            dependencyCtx.update(catchLabel->m_dependencyStart, catchLabel->id(),
                                                 STACK_OFFSET(it.stackSizeToBe), param, m_variableList);
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

            updateDeps = true;
            continue;
        }

        Instruction* instr = item->asInstruction();
        Operand* operand = instr->operands();
        Operand* end = operand + instr->paramCount();

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
                updateDeps = false;
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
            updateDeps = false;
            continue;
        }

        if (activeTryBlocks.size() > 0 && (instr->group() == Instruction::Call || instr->opcode() == ByteCode::ThrowOpcode)) {
            // Every call or throw may jump to any active catch block. Future
            // optimizations could reduce these (e.g. a throw can be converted
            // to a jump if its target catch block is in the same function).
            for (auto blockIt : activeTryBlocks) {
                for (auto it : tryBlocks()[blockIt].catchBlocks) {
                    if (it.tagIndex == std::numeric_limits<uint32_t>::max()) {
                        dependencyCtx.update(it.u.handler->m_dependencyStart, instr->id());
                    } else {
                        TagType* tagType = module()->tagType(it.tagIndex);
                        const ValueTypeVector& param = module()->functionType(tagType->sigIndex())->param();
                        Label* catchLabel = it.u.handler;

                        dependencyCtx.update(catchLabel->m_dependencyStart, catchLabel->id(),
                                             STACK_OFFSET(it.stackSizeToBe), param, nullptr);
                    }
                }
            }
        }

        if (instr->opcode() == ByteCode::ThrowOpcode || instr->opcode() == ByteCode::UnreachableOpcode
            || instr->opcode() == ByteCode::EndOpcode) {
            updateDeps = false;
            continue;
        }

        if (instr->info() & Instruction::kIsCallback) {
            for (size_t i = 0; i < requiredStackSize; i++) {
                dependencyCtx.currentOptions[i] |= VariableList::kIsCallback;
            }
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

        if (instr->opcode() == ByteCode::CallOpcode) {
            Call* call = reinterpret_cast<Call*>(instr->byteCode());
            functionType = module()->function(call->index())->functionType();
        } else {
            CallIndirect* callIndirect = reinterpret_cast<CallIndirect*>(instr->byteCode());
            functionType = callIndirect->functionType();
        }

        ASSERT(functionType->result().size() == resultCount);

        size_t id = instr->id();

        for (auto it : functionType->result()) {
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

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (!item->isLabel()) {
            continue;
        }

        Label* currentLabel = item->asLabel();
        size_t dependencyStart = currentLabel->m_dependencyStart;

        // Compute the required size first
        size_t end = dependencyStart + requiredStackSize;

        for (uint32_t i = dependencyStart; i < end; ++i) {
            ASSERT(!(dependencyCtx.options[i] & DependencyGenContext::kOptDependencyComputed));

            if (!(dependencyCtx.options[i] & DependencyGenContext::kOptReferenced)) {
                continue;
            }

            std::vector<Label*> unprocessedLabels;
            std::set<VariableRef>& dependencies = dependencyCtx.dependencies[i];

            for (auto it : dependencies) {
                if (VARIABLE_TYPE(it) == DependencyGenContext::Label) {
                    unprocessedLabels.push_back(VARIABLE_GET_LABEL(it));
                }
            }

            while (!unprocessedLabels.empty()) {
                Label* label = unprocessedLabels.back();
                std::set<VariableRef>& list = dependencyCtx.dependencies[i - dependencyStart + label->m_dependencyStart];

                unprocessedLabels.pop_back();

                for (auto it : list) {
                    if (dependencies.insert(it).second) {
                        if (VARIABLE_TYPE(it) == DependencyGenContext::Label) {
                            unprocessedLabels.push_back(VARIABLE_GET_LABEL(it));
                        }
                    }
                }
            }

            dependencyCtx.options[i] |= DependencyGenContext::kOptDependencyComputed;

            // Compute variable dependencies.
            if (checkSameConst(m_variableList, dependencies)) {
                continue;
            }

            VariableRef headRef = DependencyGenContext::kNoRef;
            uint8_t options = 0;
            size_t rangeStart = VariableList::kRangeMax;
            size_t rangeEnd = dependencyCtx.maxDistance[currentLabel->m_dependencyStart / requiredStackSize];

            for (auto it : dependencies) {
                if (VARIABLE_TYPE(it) == DependencyGenContext::Label) {
                    Label* label = VARIABLE_GET_LABEL(it);

                    if (label == currentLabel) {
                        continue;
                    }

                    options |= dependencyCtx.options[label->m_dependencyStart + (i - dependencyStart)] & VariableList::kConstraints;

                    if (rangeStart > label->id()) {
                        rangeStart = label->id();
                    }

                    size_t end = dependencyCtx.maxDistance[label->m_dependencyStart / requiredStackSize];
                    if (rangeEnd < end) {
                        rangeEnd = end;
                    }
                    continue;
                }

                VariableRef ref = VARIABLE_GET_REF(it);

                if (headRef == DependencyGenContext::kNoRef) {
                    headRef = m_variableList->getMergeHead(ref);
                } else {
                    headRef = mergeVariables(m_variableList, headRef, ref);
                }
            }

            ASSERT(headRef != DependencyGenContext::kNoRef);

            VariableList::Variable& variable = m_variableList->variables[headRef];
            variable.info |= (options | dependencyCtx.options[i]) & VariableList::kConstraints;

            if (variable.u.rangeStart > rangeStart) {
                variable.u.rangeStart = rangeStart;
            }

            if (variable.rangeEnd < rangeEnd) {
                variable.rangeEnd = rangeEnd;
            }

            dependencies.clear();
            dependencies.insert(VARIABLE_SET(headRef, DependencyGenContext::Variable));
        }
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
                const ValueTypeVector* types = nullptr;

                switch (instr->opcode()) {
                case ByteCode::CallOpcode: {
                    Call* call = reinterpret_cast<Call*>(instr->byteCode());
                    types = &module()->function(call->index())->functionType()->param();
                    break;
                }
                case ByteCode::CallIndirectOpcode: {
                    CallIndirect* callIndirect = reinterpret_cast<CallIndirect*>(instr->byteCode());
                    types = &callIndirect->functionType()->param();
                    break;
                }
                case ByteCode::ThrowOpcode: {
                    Throw* throwTag = reinterpret_cast<Throw*>(instr->byteCode());
                    TagType* tagType = module()->tagType(throwTag->tagIndex());
                    types = &module()->functionType(tagType->sigIndex())->param();
                    break;
                }
                default: {
                    ASSERT(instr->opcode() == ByteCode::EndOpcode);
                    types = &moduleFunction()->functionType()->result();
                    break;
                }
                }

                for (auto it : *types) {
                    VariableList::Variable& variable = m_variableList->variables[*param++];
                    variable.info |= Instruction::valueTypeToOperandType(it);
                }

                if (instr->opcode() == ByteCode::CallIndirectOpcode) {
                    VariableList::Variable& variable = m_variableList->variables[*param];
                    variable.info |= Instruction::Int32Operand;
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

        ASSERT(instr->next() != nullptr);

        VariableRef ref = *instr->getResult(0);
        VariableList::Variable& variable = m_variableList->variables[ref];

        if (variable.u.rangeStart == instr->id() && variable.rangeEnd == instr->id() + 1) {
            ASSERT(instr->next()->isInstruction());
            Instruction* nextInstr = instr->next()->asInstruction();

            switch (nextInstr->opcode()) {
            case ByteCode::JumpIfTrueOpcode:
            case ByteCode::JumpIfFalseOpcode:
                // These instructions has only one argument.
                ASSERT(*nextInstr->getParam(0) == VARIABLE_SET(ref, DependencyGenContext::Variable));
                variable.info |= VariableList::kIsImmediate;
                variable.value = VARIABLE_SET_PTR(nullptr);
                continue;
            case ByteCode::SelectOpcode:
                if (*nextInstr->getParam(2) == VARIABLE_SET(ref, DependencyGenContext::Variable)) {
                    variable.info |= VariableList::kIsImmediate;
                    variable.value = VARIABLE_SET_PTR(nullptr);
                    continue;
                }
                break;
            default:
                break;
            }
        }

        instr->clearInfo(Instruction::kIsMergeCompare);
    }
}

} // namespace Walrus
