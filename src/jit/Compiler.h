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

#include "interpreter/ByteCode.h"
#include "runtime/Module.h"

// Backend compiler structures.
struct sljit_compiler;
struct sljit_jump;
struct sljit_label;

namespace Walrus {

#define STACK_OFFSET(v) ((v) >> 2)

class JITFunction;
class Instruction;
class ExtendedInstruction;
class BrTableInstruction;
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
        // Covert to/from float operation. (e.g. I32TruncSatF32U, F64ConvertI32S)
        ConvertFloat,
        // Load operation. (e.g. Load32, I64Load16S)
        Load,
        // Store operation. (e.g. Store64, F32Store)
        Store,
        // Table operation. (e.g. TableInit, TableGet)
        Table,
        // Memory management operation. (e.g. MemorySize, MemoryInit)
        Memory,
        // Move operation. (e.g. Move32, Move64)
        Move,
        // Extract lane SIMD opcodes (e.g. I8X16ExtractLaneS, F64X2ExtractLane)
        ExtractLaneSIMD,
        // Replace lane SIMD opcodes (e.g. I8X16ReplaceLane, F64X2ReplaceLane)
        ReplaceLaneSIMD,
        // Splat to all lanes operation (e.g. I8X16Splat, F32X4Splat)
        SplatSIMD,
        // Binary SIMD opcodes (e.g. F64X2Min)
        BinarySIMD,
        // Unary SIMD opcodes (e.g. F64X2Abs)
        UnarySIMD,
        // Select SIMD operation (e.g. V128BitSelect)
        SelectSIMD,
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

    uint8_t internalResultCount() { return m_resultCount; }

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

    ByteCode::Opcode opcode() { return m_opcode; }

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
        ASSERT(opcode() == ByteCode::BrTableOpcode);
        return reinterpret_cast<BrTableInstruction*>(this);
    }

protected:
    explicit Instruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, Operand* operands, InstructionListItem* prev)
        : InstructionListItem(group, prev)
        , m_byteCode(byteCode)
        , m_opcode(opcode)
        , m_paramCount(paramCount)
        , m_operands(operands)
    {
    }

    explicit Instruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, InstructionListItem* prev)
        : InstructionListItem(group, prev)
        , m_byteCode(byteCode)
        , m_opcode(opcode)
        , m_paramCount(0)
        , m_operands(nullptr)
    {
    }

private:
    ByteCode* m_byteCode;
    ByteCode::Opcode m_opcode;
    uint32_t m_paramCount;
    Operand* m_operands;
};

union InstructionValue {
    // For direct branches.
    Label* targetLabel;
    // For calls.
    uint32_t resultCount;
};

class ExtendedInstruction : public Instruction {
    friend class JITCompiler;

public:
    InstructionValue& value() { return m_value; }

protected:
    explicit ExtendedInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, Operand* operands, InstructionListItem* prev)
        : Instruction(byteCode, group, opcode, paramCount, operands, prev)
    {
        ASSERT(group == Instruction::DirectBranch || group == Instruction::Call);
    }

private:
    InstructionValue m_value;
};

template <int n>
class SimpleInstruction : public Instruction {
    friend class JITCompiler;

protected:
    explicit SimpleInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, InstructionListItem* prev)
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
    explicit SimpleExtendedInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, InstructionListItem* prev)
        : ExtendedInstruction(byteCode, group, opcode, paramCount, m_inlineOperands, prev)
    {
        ASSERT(paramCount <= n);
    }

private:
    Operand m_inlineOperands[n];
};

class ComplexInstruction : public Instruction {
    friend class JITCompiler;

public:
    ~ComplexInstruction() override;

protected:
    explicit ComplexInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t operandCount, InstructionListItem* prev)
        : Instruction(byteCode, group, opcode, paramCount, new Operand[operandCount], prev)
    {
        assert(operandCount >= paramCount && operandCount > 4);
        assert(opcode == ByteCode::EndOpcode);
    }
};

class ComplexExtendedInstruction : public ExtendedInstruction {
    friend class JITCompiler;

public:
    ~ComplexExtendedInstruction() override;

protected:
    explicit ComplexExtendedInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t operandCount, InstructionListItem* prev)
        : ExtendedInstruction(byteCode, group, opcode, paramCount, new Operand[operandCount], prev)
    {
        assert(operandCount >= paramCount && operandCount > 4);
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

class DependencyGenContext;
struct LabelJumpList;
struct LabelData;

class Label : public InstructionListItem {
    friend class JITCompiler;

public:
    // Various info bits.
    static const uint16_t kHasJumpList = 1 << 0;
    static const uint16_t kHasLabelData = 1 << 1;
    static const uint16_t kNewFunction = 1 << 2;
    static const uint16_t kHasTryInfo = 1 << 3;
    static const uint16_t kHasCatchInfo = 1 << 4;

    typedef std::vector<Instruction*> DependencyList;

    explicit Label()
        : InstructionListItem(CodeLabel, nullptr)
    {
    }

    const std::vector<Instruction*>& branches() { return m_branches; }
    size_t dependencyCount() { return m_dependencies.size(); }
    const DependencyList& dependencies(size_t i) { return m_dependencies[i]; }

    sljit_label* label()
    {
        ASSERT(info() & Label::kHasLabelData);
        return m_label;
    }

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

struct TryBlock {
    struct CatchBlock {
        CatchBlock(Label* handler, size_t stackSizeToBe, uint32_t tagIndex)
            : handler(handler)
            , stackSizeToBe(stackSizeToBe)
            , tagIndex(tagIndex)
        {
        }

        Label* handler;
        size_t stackSizeToBe;
        uint32_t tagIndex;
    };

    TryBlock(Label* start, size_t size)
        : start(start)
        , parent(0)
        , returnToLabel(nullptr)
    {
        catchBlocks.reserve(size);
    }

    Label* start;
    size_t parent;
    sljit_label* findHandlerLabel;
    sljit_label* returnToLabel;
    std::vector<CatchBlock> catchBlocks;
    std::vector<sljit_jump*> throwJumps;
};

class JITCompiler {
public:
    static const uint32_t kHasCondMov = 1 << 0;

    JITCompiler(Module* module, int verboseLevel)
        : m_first(nullptr)
        , m_last(nullptr)
        , m_functionListFirst(nullptr)
        , m_functionListLast(nullptr)
        , m_compiler(nullptr)
        , m_module(module)
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

    Module* module() { return m_module; }
    int verboseLevel() { return m_verboseLevel; }
    uint32_t options() { return m_options; }
    InstructionListItem* first() { return m_first; }
    InstructionListItem* last() { return m_last; }

    void clear();
    void computeOptions();

    Instruction* append(ByteCode* byteCode, Instruction::Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t resultCount);
    ExtendedInstruction* appendExtended(ByteCode* byteCode, Instruction::Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t resultCount);
    void appendBranch(ByteCode* byteCode, ByteCode::Opcode opcode, Label* label, uint32_t offset);
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

    void buildParamDependencies(uint32_t requiredStackSize, size_t nextTryBlock);
    JITModule* compile();

    std::vector<TryBlock>& tryBlocks() { return m_tryBlocks; }
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
    void emitEpilog(size_t index, CompileContext& context);

    InstructionListItem* m_first;
    InstructionListItem* m_last;

    // List of functions. Multiple functions are compiled
    // in one step to allow direct calls between them.
    InstructionListItem* m_functionListFirst;
    InstructionListItem* m_functionListLast;

    sljit_compiler* m_compiler;
    Module* m_module;
    size_t m_branchTableSize;
    int m_verboseLevel;
    uint32_t m_options;

    std::vector<TryBlock> m_tryBlocks;
    std::vector<FunctionList> m_functionList;
};

} // namespace Walrus

#endif // __WalrusJITCompiler__
