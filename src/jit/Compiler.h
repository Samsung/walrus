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

#ifndef __WalrusJITCompiler__
#define __WalrusJITCompiler__

#include "wabt/common.h"
#include "jit/Allocator.h"
#include "interpreter/Opcode.h"

// Backend compiler structures.
struct sljit_compiler;
struct sljit_jump;
struct sljit_label;

namespace Walrus {

class JITFunction;
class Instruction;
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
        // LocalGet or LocalSet.
        LocalMove,
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
        // Covert operation. (e.g. I64ExtendI32S, I32WrapI64)
        Convert,
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
        assert(isInstruction());
        return reinterpret_cast<Instruction*>(this);
    }

    bool isLabel() { return group() == Group::CodeLabel; }

    Label* asLabel()
    {
        assert(isLabel());
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
    enum LocationType : uint8_t {
        Stack,
        Register,
        // Represents a value that is not passed on the stack.
        // Usually happens when instructions are merged.
        Unused,
        // Cannot be a result type.
        Immediate,
        // Not allowed after computeOperandLocations() call.
        CallArg,
        LocalSet,
        LocalGet,
    };

    static const uint16_t kIsOffset = 1 << 0;
    static const uint16_t kIsLocal = 1 << 1;
    static const uint16_t kIsRegister = 1 << 2;

    struct Location {
        uint8_t type;
        ValueInfo valueInfo;
    };

    union {
        // Dependency / immedate tracking.
        InstructionListItem* item;
        // For removed local.set instructions.
        Index localIndex;
        // After the computeStackAllocation call, the value member
        // follows the same rules as the value in LocationInfo
        // except for constants where the item pointer is kept.
        Index value;
    };

    union {
        Index index;
        Location location;
    };
};

union InstructionValue {
    // For immediates.
    uint32_t value32;
    uint64_t value64;
    // For local management.
    Index localIndex;
    Index value;
    // For direct calls.
    Index funcIndex;
    // For direct branches.
    Label* targetLabel;
    // For BrTable instruction.
    size_t targetLabelCount;
    // For immediate and local groups.
    Instruction* parent;
};

#define WABT_JIT_INVALID_INSTRUCTION reinterpret_cast<Instruction*>(1)

class Instruction : public InstructionListItem {
    friend class JITCompiler;

public:
    // Various info bits. Depends on type.
    static const uint16_t kKeepInstruction = 1 << 0;
    static const uint16_t kHasParent = 1 << 1;
    // Bits for Select operation
    static const uint16_t kHasResultValueInfo = 1 << 0;

    OpcodeKind opcode() { return m_opcode; }

    bool isConditionalBranch()
    {
        return opcode() == BrIfOpcode || opcode() == InterpBrUnlessOpcode;
    }

    InstructionValue& value() { return m_value; }

    // Params and results are stored in the same operands
    // array, where params come first followed by results.
    Operand* operands() { return m_operands; }
    Index paramCount() { return m_paramCount; }
    Operand* getParam(size_t i) { return m_operands + i; }
    Operand* params() { return m_operands; }
    Index resultCount();
    Operand* getResult(size_t i)
    {
        assert(hasResult());
        return m_operands + m_paramCount + i;
    }
    Operand* results()
    {
        assert(hasResult());
        return m_operands + m_paramCount;
    }

    BrTableInstruction* asBrTable()
    {
        assert(opcode() == BrTableOpcode);
        return reinterpret_cast<BrTableInstruction*>(this);
    }

    CallInstruction* asCall()
    {
        assert(group() == Call);
        return reinterpret_cast<CallInstruction*>(this);
    }

protected:
    explicit Instruction(Group group, OpcodeKind opcode, Index paramCount, Operand* operands, InstructionListItem* prev)
        : InstructionListItem(group, prev)
        , m_opcode(opcode)
        , m_paramCount(paramCount)
        , m_operands(operands)
    {
    }

    explicit Instruction(Group group, OpcodeKind opcode, InstructionListItem* prev)
        : InstructionListItem(group, prev)
        , m_opcode(opcode)
        , m_paramCount(0)
        , m_operands(nullptr)
    {
    }

private:
    OpcodeKind m_opcode;
    Index m_paramCount;
    InstructionValue m_value;
    Operand* m_operands;
};

class BrTableInstruction : public Instruction {
    friend class JITCompiler;

public:
    ~BrTableInstruction() override;

    Label** targetLabels() { return m_targetLabels; }

protected:
    BrTableInstruction(size_t targetLabelCount, InstructionListItem* prev);

private:
    Operand m_inlineParam;
    Label** m_targetLabels;
};

template <int n>
class SimpleInstruction : public Instruction {
    friend class JITCompiler;

protected:
    explicit SimpleInstruction(Group group, OpcodeKind opcode, Index paramCount, InstructionListItem* prev)
        : Instruction(group, opcode, paramCount, m_inlineOperands, prev)
    {
        assert(paramCount == n || paramCount + 1 == n);
    }

    explicit SimpleInstruction(Group group, OpcodeKind opcode, InstructionListItem* prev)
        : Instruction(group, opcode, n, m_inlineOperands, prev)
    {
    }

private:
    Operand m_inlineOperands[n];
};

class ComplexInstruction : public Instruction {
    friend class JITCompiler;

public:
    ~ComplexInstruction() override;

protected:
    explicit ComplexInstruction(Group group, OpcodeKind opcode, Index paramCount, Index operandCount, InstructionListItem* prev)
        : Instruction(group, opcode, paramCount, new Operand[operandCount], prev)
    {
        assert(operandCount >= paramCount && operandCount > 4);
        assert(opcode == ReturnOpcode);
    }
};

class CallInstruction : public Instruction {
    friend class JITCompiler;

public:
    static const uint16_t kIndirect = 1 << 0;
    static const uint16_t kReturn = 1 << 1;

    FunctionType* functionType() { return m_functionType; }
    Index frameSize() { return m_frameSize; }
    Index paramStart() { return m_paramStart; }

protected:
    explicit CallInstruction(OpcodeKind opcode, Index paramCount, FunctionType* functionType, Operand* operands, InstructionListItem* prev)
        : Instruction(Instruction::Call, opcode, paramCount, operands, prev)
        , m_functionType(functionType)
    {
        assert(opcode == CallOpcode || opcode == CallIndirectOpcode || opcode == ReturnCallOpcode);
    }

    FunctionType* m_functionType;
    Index m_frameSize;
    Index m_paramStart;
};

template <int n>
class SimpleCallInstruction : public CallInstruction {
    friend class JITCompiler;

private:
    explicit SimpleCallInstruction(OpcodeKind opcode, FunctionType* functionType, InstructionListItem* prev);

    Operand m_inlineOperands[n];
};

class ComplexCallInstruction : public CallInstruction {
    friend class JITCompiler;

public:
    ~ComplexCallInstruction() override;

private:
    explicit ComplexCallInstruction(OpcodeKind opcode, FunctionType* functionType, InstructionListItem* prev);
};

class DependencyGenContext;
class SetLocalContext;
class ReduceMovesContext;
struct LabelJumpList;
struct LabelData;

class Label : public InstructionListItem {
    friend class JITCompiler;

public:
    // Various info bits.
    static const uint16_t kNewFunction = 1 << 0;
    static const uint16_t kAfterUncondBranch = 1 << 1;
    static const uint16_t kCanHaveElse = 1 << 2;
    static const uint16_t kHasJumpList = 1 << 3;
    static const uint16_t kHasLabelData = 1 << 4;

    // Info bits for checkLocals.
    static const uint16_t kCheckLocalsReached = 1 << 5;
    static const uint16_t kCheckLocalsHasContext = 1 << 6;

    struct Dependency {
        Dependency(Instruction* instr, Index index)
            : instr(instr)
            , index(index)
        {
        }

        Instruction* instr;
        Index index;
    };

    typedef std::vector<Dependency> DependencyList;

    Index resultCount() { return m_resultCount; }
    Index preservedCount() { return m_preservedCount; }
    Index stackSize() { return m_preservedCount + m_resultCount; }

    const std::vector<Instruction*>& branches() { return m_branches; }
    size_t dependencyCount() { return m_dependencies.size(); }
    const DependencyList& dependencies(size_t i) { return m_dependencies[i]; }

    void append(Instruction* instr);
    // Should be called before removing the other instruction.
    void merge(Label* other);

    void jumpFrom(sljit_jump* jump);
    void emit(sljit_compiler* compiler);

private:
    explicit Label(Index resultCount, Index preservedCount, InstructionListItem* prev)
        : InstructionListItem(CodeLabel, prev)
        , m_resultCount(resultCount)
        , m_preservedCount(preservedCount)
    {
    }

    // The number of items on the stack should
    // be the sum of result and preserved count.
    Index m_resultCount;
    Index m_preservedCount;

    std::vector<Instruction*> m_branches;
    std::vector<DependencyList> m_dependencies;

    // Contexts used by different compiling stages.
    union {
        SetLocalContext* m_setLocalCtx;
        DependencyGenContext* m_dependencyCtx;
        ReduceMovesContext* m_reduceMovesCtx;
        StackAllocator* m_stackAllocator;
        LabelJumpList* m_jumpList;
        LabelData* m_labelData;
        size_t m_instrIndex;
    };
};

class JITCompiler {
public:
    JITCompiler(int verboseLevel)
        : m_verboseLevel(verboseLevel)
    {
    }

    ~JITCompiler()
    {
        clear();
        releaseFunctionList();
    }

    int verboseLevel() { return m_verboseLevel; }
    InstructionListItem* first() { return m_first; }
    InstructionListItem* last() { return m_last; }

    void clear();

    Instruction* append(Instruction::Group group, OpcodeKind opcode, Index paramCount);
    Instruction* append(Instruction::Group group, OpcodeKind opcode, Index paramCount, ValueInfo result);
    CallInstruction* appendCall(OpcodeKind opcode, FunctionType* functionType);
    void appendBranch(OpcodeKind opcode, Index depth);
    void appendElseLabel();
    void appendBrTable(Index numTargets, Index* targetDepths, Index defaultTargetDepth);
    void appendUnreachable()
    {
        append(Instruction::Any, UnreachableOpcode, 0);
        m_stackDepth = wabt::kInvalidIndex;
    }

    void pushLabel(OpcodeKind opcode, Index paramCount, Index resultCount);
    void popLabel();
    size_t labelCount() { return m_labelStack.size(); }
    ValueInfo local(size_t i) { return m_locals[i]; }
    std::vector<ValueInfo>& locals() { return m_locals; }

    void appendFunction(JITFunction* jitFunc, bool isExternal);
    void dump(bool afterStackComputation);

    void checkLocals(size_t params_size);
    void buildParamDependencies();
    void reduceLocalAndConstantMoves();
    void optimizeBlocks();
    void computeOperandLocations(JITFunction* jit_func,
                                 ValueTypeVector& results);
    JITModule* compile();

    Label* getFunctionEntry(size_t i) { return m_functionList[i].entryLabel; }

private:
    struct FunctionList {
        FunctionList(JITFunction* jitFunc, Label* entryLabel, bool isExported)
            : jitFunc(jitFunc)
            , entryLabel(entryLabel)
            , exportEntryLabel(nullptr)
            , isExported(isExported)
        {
        }

        JITFunction* jitFunc;
        Label* entryLabel;
        sljit_label* exportEntryLabel;
        bool isExported;
    };

    struct ElseBlock {
        ElseBlock(Instruction* branch, Index resultCount, Index preservedCount)
            : branch(branch)
            , resultCount(resultCount)
            , preservedCount(preservedCount)
        {
        }

        Instruction* branch;
        Index resultCount;
        Index preservedCount;
    };

    void append(InstructionListItem* item);
    InstructionListItem* remove(InstructionListItem* item);
    void replace(InstructionListItem* item, InstructionListItem* newItem);

    Instruction* appendInternal(Instruction::Group group, OpcodeKind opcode, Index paramCount, Index operandCount, ValueInfo result);

    // Backend operations.
    void releaseFunctionList();
    void emitProlog(size_t index, CompileContext& context);
    sljit_label* emitEpilog(size_t index, CompileContext& context);

    InstructionListItem* m_first = nullptr;
    InstructionListItem* m_last = nullptr;

    // List of functions. Multiple functions are compiled
    // in one step to allow direct calls between them.
    InstructionListItem* m_functionListFirst = nullptr;
    InstructionListItem* m_functionListLast = nullptr;

    sljit_compiler* m_compiler = nullptr;
    Index m_stackDepth = 0;
    int m_verboseLevel;

    std::vector<Label*> m_labelStack;
    std::vector<ElseBlock> m_elseBlocks;
    std::vector<ValueInfo> m_locals;
    std::vector<FunctionList> m_functionList;
};

} // namespace Walrus

#endif // __WalrusJITCompiler__
