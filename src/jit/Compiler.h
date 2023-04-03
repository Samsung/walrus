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

#ifndef __WalrusJITCompiler__
#define __WalrusJITCompiler__

#include "interpreter/Opcode.h"
#include "interpreter/ByteCode.h"

// Backend compiler structures.
struct sljit_compiler;
struct sljit_jump;
struct sljit_label;

namespace Walrus {

class JITFunction;
class Instruction;
class ExtendedInstruction;
class BrTableInstruction;
class CallInstruction;
class Label;
class JITModule;
struct CompileContext;

// Defined in ObjectType.h.
class FunctionType;

class InstructionListItem {
    friend class JITCompiler;

public:
    enum Group : uint8_t {
        // Not in a group.
        CodeLabel,
        // Generic instruction, also start of the instructions.
        Any,
        // I32Const, I64Const, F32Const, F64Const, V128Const.
        Immediate,
        // Br, BrIf, InterpBrUnless.
        DirectBranch,
        // BrTable.
        BrTable,
        // Return.
        Return,
        // Call, CallIndirect, ReturnCall, ReturnCallIndirect.
        Call,
        // Binary operation (e.g. I32Add, I64Sub).
        Binary,
        // Binary float operation
        BinaryFloat,
        // Unary operation (e.g. I32Ctz, U64Clz).
        Unary,
        // Unary float operation
        UnaryFloat,
        // Compare operation. (e.g. I32Eqz, I64LtU)
        Compare,
        // Compare operation. (e.g. F32Eq, F64Lt)
        CompareFloat,
        // Covert operation. (e.g. I64ExtendI32S, I32WrapI64)
        Convert,
        // Move operation. (e.g. Move32, Move64)
        Move,
    };

    virtual ~InstructionListItem() {}

    InstructionListItem* next() { return m_next; }
    InstructionListItem* prev() { return m_prev; }

    uint16_t info() { return m_info; }
    void setInfo(uint16_t value) { m_info = value; }
    void addInfo(uint16_t value) { m_info |= value; }
    void clearInfo(uint16_t value) { m_info &= static_cast<uint16_t>(~value); }
    Group group() { return static_cast<Group>(m_group); }
    bool hasResult() { return m_resultCount != 0; }

    bool isInstruction() { return group() >= Group::Any; }

    Instruction* asInstruction()
    {
        ASSERT(isInstruction());
        return reinterpret_cast<Instruction*>(this);
    }

    bool isLabel() { return group() == Group::CodeLabel; }

    Label* asLabel()
    {
        ASSERT(isLabel());
        return reinterpret_cast<Label*>(this);
    }

protected:
    explicit InstructionListItem(Group group, InstructionListItem* prev)
        : m_next(nullptr)
        , m_prev(prev)
        , m_group(group)
        , m_resultCount(0)
        , m_info(0)
    {
    }

private:
    InstructionListItem* m_next;
    InstructionListItem* m_prev;
    uint8_t m_group;
    uint8_t m_resultCount;
    uint16_t m_info;
};

struct Operand {
    // Dependency / immedate tracking.
    InstructionListItem* item;
    uint32_t offset;
};

class Instruction : public InstructionListItem {
    friend class JITCompiler;

public:
    // Various info bits. Depends on type.
    static const uint16_t kIs32Bit = 1 << 0;
    static const uint16_t kKeepInstruction = 1 << 1;
    static const uint16_t kEarlyReturn = 1 << 2;

    OpcodeKind opcode() { return m_opcode; }

    bool isConditionalBranch()
    {
        return opcode() == BrIfOpcode || opcode() == InterpBrUnlessOpcode;
    }

    ByteCode* byteCode() { return m_byteCode; }

    // Params and results are stored in the same operands
    // array, where params come first followed by results.
    Operand* operands() { return m_operands; }
    uint32_t paramCount() { return m_paramCount; }
    Operand* getParam(size_t i) { return m_operands + i; }
    Operand* params() { return m_operands; }
    uint32_t resultCount();
    Operand* getResult(size_t i)
    {
        ASSERT(hasResult());
        return m_operands + m_paramCount + i;
    }
    Operand* results()
    {
        ASSERT(hasResult());
        return m_operands + m_paramCount;
    }

    ExtendedInstruction* asExtended()
    {
        ASSERT(group() == Instruction::DirectBranch);
        return reinterpret_cast<ExtendedInstruction*>(this);
    }

    BrTableInstruction* asBrTable()
    {
        ASSERT(opcode() == BrTableOpcode);
        return reinterpret_cast<BrTableInstruction*>(this);
    }

    CallInstruction* asCall()
    {
        ASSERT(group() == Call);
        return reinterpret_cast<CallInstruction*>(this);
    }

protected:
    explicit Instruction(ByteCode* byteCode, Group group, OpcodeKind opcode, uint32_t paramCount, Operand* operands, InstructionListItem* prev)
        : InstructionListItem(group, prev)
        , m_byteCode(byteCode)
        , m_opcode(opcode)
        , m_paramCount(paramCount)
        , m_operands(operands)
    {
    }

    explicit Instruction(ByteCode* byteCode, Group group, OpcodeKind opcode, InstructionListItem* prev)
        : InstructionListItem(group, prev)
        , m_byteCode(byteCode)
        , m_opcode(opcode)
        , m_paramCount(0)
        , m_operands(nullptr)
    {
    }

private:
    ByteCode* m_byteCode;
    OpcodeKind m_opcode;
    uint32_t m_paramCount;
    Operand* m_operands;
};

union InstructionValue {
    // For direct branches.
    Label* targetLabel;
};

class ExtendedInstruction : public Instruction {
    friend class JITCompiler;

public:
    InstructionValue& value() { return m_value; }

protected:
    explicit ExtendedInstruction(ByteCode* byteCode, Group group, OpcodeKind opcode, uint32_t paramCount, Operand* operands, InstructionListItem* prev)
        : Instruction(byteCode, group, opcode, paramCount, operands, prev)
    {
        ASSERT(group == Instruction::DirectBranch);
    }

private:
    InstructionValue m_value;
};

template <int n>
class SimpleInstruction : public Instruction {
    friend class JITCompiler;

protected:
    explicit SimpleInstruction(ByteCode* byteCode, Group group, OpcodeKind opcode, uint32_t paramCount, InstructionListItem* prev)
        : Instruction(byteCode, group, opcode, paramCount, m_inlineOperands, prev)
    {
        ASSERT(paramCount == n || paramCount + 1 == n);
    }

private:
    Operand m_inlineOperands[n];
};

template <int n>
class SimpleExtendedInstruction : public ExtendedInstruction {
    friend class JITCompiler;

protected:
    explicit SimpleExtendedInstruction(ByteCode* byteCode, Group group, OpcodeKind opcode, uint32_t paramCount, InstructionListItem* prev)
        : ExtendedInstruction(byteCode, group, opcode, paramCount, m_inlineOperands, prev)
    {
        ASSERT(paramCount == n || paramCount + 1 == n);
    }

private:
    Operand m_inlineOperands[n];
};

class ComplexInstruction : public Instruction {
    friend class JITCompiler;

public:
    ~ComplexInstruction() override;

protected:
    explicit ComplexInstruction(ByteCode* byteCode, Group group, OpcodeKind opcode, uint32_t paramCount, uint32_t operandCount, InstructionListItem* prev)
        : Instruction(byteCode, group, opcode, paramCount, new Operand[operandCount], prev)
    {
        assert(operandCount >= paramCount && operandCount > 4);
        assert(opcode == EndOpcode);
    }
};

class BrTableInstruction : public Instruction {
    friend class JITCompiler;

public:
    ~BrTableInstruction() override;

    Label** targetLabels() { return m_targetLabels; }
    size_t targetLabelCount() { return m_targetLabelCount; }

protected:
    BrTableInstruction(ByteCode* byteCode, size_t targetLabelCount, InstructionListItem* prev);

private:
    Operand m_inlineParam;
    size_t m_targetLabelCount;
    Label** m_targetLabels;
};

class CallInstruction : public Instruction {
    friend class JITCompiler;

public:
    static const uint16_t kIndirect = 1 << 0;
    static const uint16_t kReturn = 1 << 1;

    FunctionType* functionType() { return m_functionType; }
    uint32_t frameSize() { return m_frameSize; }
    uint32_t paramStart() { return m_paramStart; }

protected:
    explicit CallInstruction(ByteCode* byteCode, OpcodeKind opcode, uint32_t paramCount, FunctionType* functionType, Operand* operands, InstructionListItem* prev)
        : Instruction(byteCode, Instruction::Call, opcode, paramCount, operands, prev)
        , m_functionType(functionType)
    {
        ASSERT(opcode == CallOpcode || opcode == CallIndirectOpcode || opcode == ReturnCallOpcode);
    }

    FunctionType* m_functionType;
    uint32_t m_frameSize;
    uint32_t m_paramStart;
};

template <int n>
class SimpleCallInstruction : public CallInstruction {
    friend class JITCompiler;

private:
    explicit SimpleCallInstruction(ByteCode* byteCode, OpcodeKind opcode, FunctionType* functionType, InstructionListItem* prev);

    Operand m_inlineOperands[n];
};

class ComplexCallInstruction : public CallInstruction {
    friend class JITCompiler;

public:
    ~ComplexCallInstruction() override;

private:
    explicit ComplexCallInstruction(ByteCode* byteCode, OpcodeKind opcode, FunctionType* functionType, InstructionListItem* prev);
};

class DependencyGenContext;
struct LabelJumpList;
struct LabelData;

class Label : public InstructionListItem {
    friend class JITCompiler;

public:
    // Various info bits.
    static const uint16_t kNewFunction = 1 << 0;
    static const uint16_t kHasJumpList = 1 << 1;
    static const uint16_t kHasLabelData = 1 << 2;

    typedef std::vector<Instruction*> DependencyList;

    explicit Label()
        : InstructionListItem(CodeLabel, nullptr)
    {
    }

    const std::vector<Instruction*>& branches() { return m_branches; }
    size_t dependencyCount() { return m_dependencies.size(); }
    const DependencyList& dependencies(size_t i) { return m_dependencies[i]; }

    void append(Instruction* instr);
    // Should be called before removing the other instruction.
    void merge(Label* other);

    void jumpFrom(sljit_jump* jump);
    void emit(sljit_compiler* compiler);

private:
    explicit Label(InstructionListItem* prev)
        : InstructionListItem(CodeLabel, prev)
    {
    }

    std::vector<Instruction*> m_branches;
    std::vector<DependencyList> m_dependencies;

    // Contexts used by different compiling stages.
    union {
        DependencyGenContext* m_dependencyCtx;
        LabelJumpList* m_jumpList;
        sljit_label* m_label;
    };
};

class JITCompiler {
public:
    static const uint32_t kHasCondMov = 1 << 0;

    JITCompiler(int verboseLevel)
        : m_first(nullptr)
        , m_last(nullptr)
        , m_functionListFirst(nullptr)
        , m_functionListLast(nullptr)
        , m_compiler(nullptr)
        , m_branchTableSize(0)
        , m_verboseLevel(verboseLevel)
        , m_options(0)
    {
        computeOptions();
    }

    ~JITCompiler()
    {
        clear();
        releaseFunctionList();
    }

    int verboseLevel() { return m_verboseLevel; }
    uint32_t options() { return m_options; }
    InstructionListItem* first() { return m_first; }
    InstructionListItem* last() { return m_last; }

    void clear();
    void computeOptions();

    Instruction* append(ByteCode* byteCode, Instruction::Group group, OpcodeKind opcode, uint32_t paramCount, uint32_t resultCount);
    void appendBranch(ByteCode* byteCode, OpcodeKind opcode, Label* label, uint32_t offset);
    BrTableInstruction* appendBrTable(ByteCode* byteCode, uint32_t numTargets, uint32_t offset);

    void appendLabel(Label* label)
    {
        ASSERT(label->m_prev == nullptr);
        label->m_prev = m_last;
        append(label);
    }

    void increaseBranchTableSize(size_t value)
    {
        m_branchTableSize += value;
    }

    void appendFunction(JITFunction* jitFunc, bool isExternal);
    void dump();

    void buildParamDependencies(uint32_t requiredStackSize);
    JITModule* compile();

    Label* getFunctionEntry(size_t i) { return m_functionList[i].entryLabel; }

private:
    struct FunctionList {
        FunctionList(JITFunction* jitFunc, Label* entryLabel, bool isExported, size_t branchTableSize)
            : jitFunc(jitFunc)
            , entryLabel(entryLabel)
            , exportEntryLabel(nullptr)
            , isExported(isExported)
            , branchTableSize(branchTableSize)
        {
        }

        JITFunction* jitFunc;
        Label* entryLabel;
        sljit_label* exportEntryLabel;
        bool isExported;
        size_t branchTableSize;
    };

    void append(InstructionListItem* item);
    InstructionListItem* remove(InstructionListItem* item);
    void replace(InstructionListItem* item, InstructionListItem* newItem);

    // Backend operations.
    void releaseFunctionList();
    void emitProlog(size_t index, CompileContext& context);
    sljit_label* emitEpilog(size_t index, CompileContext& context);

    InstructionListItem* m_first;
    InstructionListItem* m_last;

    // List of functions. Multiple functions are compiled
    // in one step to allow direct calls between them.
    InstructionListItem* m_functionListFirst;
    InstructionListItem* m_functionListLast;

    sljit_compiler* m_compiler;
    size_t m_branchTableSize;
    int m_verboseLevel;
    uint32_t m_options;

    std::vector<FunctionList> m_functionList;
};

} // namespace Walrus

#endif // __WalrusJITCompiler__
