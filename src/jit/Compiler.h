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
#include "jit/SljitLir.h"
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
        // Unary Conditional SIMD opcodes (e.g. I8x16AllTrue)
        UnaryCondSIMD,
        // Load lane SIMD opcodes (e.g. V128Load16Lane)
        LoadLaneSIMD,
        // Bitmask SIMD opcodes (e.g. I16X8Bitmask)
        BitMaskSIMD,
        // Shift SIMD opcodes (e.g. I8X16SHL)
        ShiftSIMD,
        // Special type for initializing values from the stack
        StackInit,
#if defined(ENABLE_EXTENDED_FEATURES)
        // Atomic memory operations (e.g. I32AtomicRmwAdd, I64AtomicRmw16OrU)
        Atomic,
#endif /* ENABLE_EXTENDED_FEATURES */
    };

    virtual ~InstructionListItem() {}

    InstructionListItem* next() { return m_next; }

    size_t id() { return m_id; }
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
    explicit InstructionListItem(Group group)
        : m_next(nullptr)
        , m_id(0)
        , m_group(group)
        , m_resultCount(0)
        , m_info(0)
    {
    }

    uint8_t internalResultCount() { return m_resultCount; }

private:
    InstructionListItem* m_next;
    size_t m_id;
    uint8_t m_group;
    uint8_t m_resultCount;
    uint16_t m_info;
};

typedef uintptr_t VariableRef;
typedef VariableRef Operand;

// These macros provide VariableRefs related operations.
#define VARIABLE_SET(v, type) ((static_cast<VariableRef>(v) << 2) | (type))
#define VARIABLE_SET_PTR(v) (reinterpret_cast<VariableRef>(v))
#define VARIABLE_TYPE(v) ((v)&0x3)
#define VARIABLE_GET_REF(v) ((v) >> 2)
#define VARIABLE_GET_OFFSET(v) ((v) & ~(VariableRef)0x3)
#define VARIABLE_GET_IMM(v) (reinterpret_cast<InstructionListItem*>(v)->asInstruction())

class Instruction : public InstructionListItem {
    friend class JITCompiler;

public:
    enum OperandDescriptor : uint8_t {
        // Int64LowOperand is a special destination operand type for 32 bit
        // systems. It represents that a single register is enough to allocate
        // when the operand is memory (must be combined with TmpRequired).
        Int64LowOperand = 0,
        Int32Operand = 1,
        Int64Operand = 2,
        V128Operand = 4,
        // Int32Operand | Float32Operand == Float32Operand
        Float32Operand = 5,
        // Int64Operand | Float64Operand == Float64Operand
        Float64Operand = 6,
        // Helper constants for managing type info.
        FloatOperandMarker = V128Operand,
        TypeMask = 0x7,
        // A temporary register must be allocated for the source or destination
        // operand. In case of source operands, the register is not modified.
        TmpRequired = (1 << 3),
        // Flags for source operands.
        TmpNotAllowed = (1 << 4),
        // Similar to Int64LowOperand, except it applies to source operands
        LowerHalfNeeded = (1 << 5),
        // Flags for destination and temporary operands.
        // Registers assigned to these operands can take the role of source operands
        Src0Allowed = (1 << 4),
        Src1Allowed = (1 << 5),
        Src2Allowed = (1 << 6),
    };

    enum OperandType : Operand {
        // ConstPtr type must be 0, because they are ByteCode pointers.
        ConstPtr = 0,
        Register = 1,
        Offset = 2,
    };

    static const uint32_t TemporaryTypeShift = 4;
    static const uint32_t TemporaryTypeMask = 0xf;

    // Various info bits. Depends on type.
    static const uint16_t kIs32Bit = 1 << 0;
    static const uint16_t kIsCallback = 1 << 1;
    static const uint16_t kDestroysR0R1 = 1 << 2;
    static const uint16_t kHasFloatOperand = 1 << 3;
    static const uint16_t kIsShift = 1 << 4;
    static const uint16_t kIsMergeCompare = 1 << 5;
    static const uint16_t kFreeUnusedEarly = 1 << 6;
    static const uint16_t kKeepInstruction = 1 << 7;
    static const uint16_t kEarlyReturn = kKeepInstruction;

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
        ASSERT(group() == Instruction::DirectBranch || group() == Instruction::StackInit);
        return reinterpret_cast<ExtendedInstruction*>(this);
    }

    BrTableInstruction* asBrTable()
    {
        ASSERT(opcode() == ByteCode::BrTableOpcode);
        return reinterpret_cast<BrTableInstruction*>(this);
    }

    void setRequiredRegsDescriptor(uint32_t requiredRegsDescriptor)
    {
        u.m_requiredRegsDescriptor = requiredRegsDescriptor;
    }

    uint8_t requiredReg(size_t n)
    {
        ASSERT(n < sizeof(u.m_requiredRegs));
        return u.m_requiredRegs[n];
    }

    void setRequiredReg(size_t n, uint8_t value)
    {
        ASSERT(n < sizeof(u.m_requiredRegs));
        u.m_requiredRegs[n] = value;
    }

    inline const uint8_t* getOperandDescriptor()
    {
        return m_operandDescriptors + u.m_requiredRegsDescriptor;
    }

    static inline const uint8_t* getOperandDescriptorByOffset(uint32_t offset)
    {
        return m_operandDescriptors + offset;
    }

    static uint32_t valueTypeToOperandType(Value::Type type);

protected:
    explicit Instruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, Operand* operands)
        : InstructionListItem(group)
        , m_byteCode(byteCode)
        , m_operands(operands)
        , m_opcode(opcode)
        , m_paramCount(paramCount)
    {
        u.m_requiredRegsDescriptor = 0;
    }

    explicit Instruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode)
        : InstructionListItem(group)
        , m_byteCode(byteCode)
        , m_operands(nullptr)
        , m_opcode(opcode)
        , m_paramCount(0)
    {
        u.m_requiredRegsDescriptor = 0;
    }

private:
    static const uint8_t m_operandDescriptors[];

    ByteCode* m_byteCode;
    Operand* m_operands;
    ByteCode::Opcode m_opcode;
    uint32_t m_paramCount;
    union {
        uint32_t m_requiredRegsDescriptor;
        uint8_t m_requiredRegs[4];
    } u;
};

union InstructionValue {
    // For direct branches.
    Label* targetLabel;
    // For calls.
    uint32_t resultCount;
    // For StackInit group.
    VariableRef offset;
};

class ExtendedInstruction : public Instruction {
    friend class JITCompiler;

public:
    InstructionValue& value() { return m_value; }

protected:
    explicit ExtendedInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, Operand* operands)
        : Instruction(byteCode, group, opcode, paramCount, operands)
    {
        ASSERT(group == Instruction::DirectBranch || group == Instruction::Call || group == Instruction::StackInit);
    }

private:
    InstructionValue m_value;
};

template <int n>
class SimpleInstruction : public Instruction {
    friend class JITCompiler;

protected:
    explicit SimpleInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount)
        : Instruction(byteCode, group, opcode, paramCount, m_inlineOperands)
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
    explicit SimpleExtendedInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount)
        : ExtendedInstruction(byteCode, group, opcode, paramCount, m_inlineOperands)
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
    explicit ComplexInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t operandCount)
        : Instruction(byteCode, group, opcode, paramCount, new Operand[operandCount])
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
    explicit ComplexExtendedInstruction(ByteCode* byteCode, Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t operandCount)
        : ExtendedInstruction(byteCode, group, opcode, paramCount, new Operand[operandCount])
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
    BrTableInstruction(ByteCode* byteCode, size_t targetLabelCount);

private:
    Operand m_inlineParam;
    size_t m_targetLabelCount;
    Label** m_targetLabels;
};

struct LabelJumpList;
struct LabelData;

class Label : public InstructionListItem {
    friend class JITCompiler;

public:
    // Various info bits.
    static const uint16_t kHasJumpList = 1 << 0;
    static const uint16_t kHasLabelData = 1 << 1;
    static const uint16_t kHasTryInfo = 1 << 2;
    static const uint16_t kHasCatchInfo = 1 << 3;

    explicit Label()
        : InstructionListItem(CodeLabel)
    {
    }

    const std::vector<Instruction*>& branches() { return m_branches; }

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
    std::vector<Instruction*> m_branches;

    // Contexts used by different compiling stages.
    union {
        size_t m_dependencyStart;
        LabelJumpList* m_jumpList;
        sljit_label* m_label;
    };
};

struct TryBlock {
    struct CatchBlock {
        CatchBlock(Label* handler, size_t stackSizeToBe, uint32_t tagIndex)
            : stackSizeToBe(stackSizeToBe)
            , tagIndex(tagIndex)
        {
            u.handler = handler;
        }

        union {
            Label* handler;
            sljit_label* handlerLabel;
        } u;
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

class JITCompiler;
class SlowCase;

struct TrapBlock {
    TrapBlock(sljit_label* endLabel, size_t tryBlockId)
        : endLabel(endLabel)
    {
        u.tryBlockId = tryBlockId;
    }

    TrapBlock(sljit_label* endLabel, sljit_label* handlerLabel)
        : endLabel(endLabel)
    {
        u.handlerLabel = handlerLabel;
    }

    sljit_label* endLabel;

    union {
        sljit_label* handlerLabel;
        size_t tryBlockId;
    } u;
};

struct TrapJump {
    TrapJump(uint32_t jumpType, sljit_jump* jump)
        : jumpType(jumpType)
        , jump(jump)
    {
    }

    bool operator<(const TrapJump& other) const
    {
        return jumpType < other.jumpType;
    }

    uint32_t jumpType;
    sljit_jump* jump;
};

struct CompileContext {
    CompileContext(Module* module, JITCompiler* compiler);

    static CompileContext* get(sljit_compiler* compiler);

    void add(SlowCase* slowCase) { slowCases.push_back(slowCase); }
    void appendTrapJump(uint32_t jumpType, sljit_jump* jump) { trapJumps.push_back(TrapJump(jumpType, jump)); }
    void emitSlowCases(sljit_compiler* compiler);

    JITCompiler* compiler;
    uintptr_t branchTableOffset;
#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
    uintptr_t shuffleOffset;
#endif /* SLJIT_CONFIG_X86 */
    size_t globalsStart;
    size_t tableStart;
    size_t functionsStart;
    size_t nextTryBlock;
    size_t currentTryBlock;
    size_t trapBlocksStart;
    uint64_t initialMemorySize;
    uint64_t maximumMemorySize;
    std::vector<TrapBlock> trapBlocks;
    std::vector<size_t> tryBlockStack;
    std::vector<SlowCase*> slowCases;
    std::vector<sljit_jump*> earlyReturns;
    std::vector<TrapJump> trapJumps;
};

struct VariableList {
    // Used by DependencyGenContext
    static const uint32_t kIsCallback = 1 << 4;
    static const uint32_t kDestroysR0R1 = 1 << 5;
    static const uint32_t kIsMerged = 1 << 6;
    static const uint32_t kIsImmediate = 1 << 7;

    static const uint32_t kConstraints = kIsCallback | kDestroysR0R1;
    static const size_t kRangeMax = ~(VariableRef)0;
    static const uint8_t kUnusedReg = 0xff;

    struct Variable {
        Variable(VariableRef value, uint32_t typeInfo, size_t id)
            : value(value)
            , info(typeInfo)
            , reg1(kUnusedReg)
            , reg2(kUnusedReg)
            , rangeEnd(id)
        {
            u.rangeStart = id;
        }

        Variable(VariableRef value, uint32_t typeInfo, Instruction* instr)
            : value(value)
            , info(typeInfo)
            , reg1(kUnusedReg)
            , reg2(kUnusedReg)
            , rangeEnd(instr->id())
        {
            if (instr->group() == Instruction::Immediate) {
                info |= kIsImmediate;
                u.immediate = instr;
            } else {
                u.rangeStart = rangeEnd;
            }
        }

        VariableRef value;
        uint16_t info;
        uint8_t reg1;
        uint8_t reg2;
        union {
            size_t rangeStart;
            size_t parent;
            Instruction* immediate;
        } u;
        size_t rangeEnd;
    };

    struct CatchUpdate {
        CatchUpdate(Label* handler, size_t variableListStart, size_t variableListSize)
            : handler(handler)
            , variableListStart(variableListStart)
            , variableListSize(variableListSize)
        {
        }

        Label* handler;
        size_t variableListStart;
        size_t variableListSize;
    };

    VariableList(size_t variableCount, size_t paramCount)
        : paramCount(paramCount)
    {
        variables.reserve(variableCount);
    }

    VariableRef getMergeHeadSlowCase(VariableRef ref);

    VariableRef getMergeHead(VariableRef ref)
    {
        if (!(variables[ref].info & kIsMerged)) {
            return ref;
        }

        return getMergeHeadSlowCase(ref);
    }

    void pushCatchUpdate(Label* handler, size_t variableListSize)
    {
        catchUpdates.push_back(CatchUpdate(handler, variables.size(), variableListSize));
    }

    const uint8_t* getOperandDescriptor(Instruction* instr);

    size_t paramCount;
    std::vector<Variable> variables;
    std::vector<CatchUpdate> catchUpdates;
};

class JITCompiler {
public:
#ifdef WALRUS_JITPERF
    struct DebugEntry {
        DebugEntry()
            : line(0)
        {
            u.label = nullptr;
        }

        DebugEntry(sljit_label* label, uint32_t line)
            : line(line)
        {
            u.label = label;
        }

        union {
            sljit_label* label;
            uintptr_t address;
        } u;
        uint32_t line;
    };
#endif

    static const uint32_t kHasCondMov = 1 << 0;

    JITCompiler(Module* module, uint32_t JITFlags);

    ~JITCompiler()
    {
        clear();
        m_functionList.clear();
    }

    Module* module() { return m_module; }
    ModuleFunction* moduleFunction() { return m_moduleFunction; }
    CompileContext& context() { return m_context; }
    uint32_t JITFlags() { return m_JITFlags; }
    uint32_t options() { return m_options; }
    InstructionListItem* first() { return m_first; }
    InstructionListItem* last() { return m_last; }

    void clear();

    Instruction* append(ByteCode* byteCode, Instruction::Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t resultCount);
    ExtendedInstruction* appendExtended(ByteCode* byteCode, Instruction::Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t resultCount);
    Instruction* appendBranch(ByteCode* byteCode, ByteCode::Opcode opcode, Label* label, uint32_t offset);
    BrTableInstruction* appendBrTable(ByteCode* byteCode, uint32_t numTargets, uint32_t offset);
    InstructionListItem* insertStackInit(InstructionListItem* prev, VariableList::Variable& variable, VariableRef ref);
    void insertStackInitList(InstructionListItem* prev, size_t variableListStart, size_t variableListSize);

    void appendLabel(Label* label)
    {
        append(label);
    }

    void increaseBranchTableSize(size_t value)
    {
        m_branchTableSize += value;
    }

    void setModuleFunction(ModuleFunction* moduleFunction)
    {
        m_moduleFunction = moduleFunction;
    }

    void buildVariables(uint32_t requiredStackSize);
    void allocateRegistersSimple();
    void allocateRegisters();
    void freeVariables();

    void compileFunction(JITFunction* jitFunc, bool isExternal);
    void generateCode();

    std::vector<TryBlock>& tryBlocks() { return m_tryBlocks; }
    void initTryBlockStart() { m_tryBlockStart = m_tryBlocks.size(); }
    size_t tryBlockOffset() { return m_tryBlockOffset; }

#if !defined(NDEBUG)
    static const char** byteCodeNames()
    {
        return m_byteCodeNames;
    }
    void dump();
#endif /* !NDEBUG */

private:
    struct FunctionList {
        FunctionList(JITFunction* jitFunc, bool isExported, size_t branchTableSize)
            : jitFunc(jitFunc)
            , exportEntryLabel(nullptr)
            , isExported(isExported)
            , branchTableSize(branchTableSize)
        {
        }

        JITFunction* jitFunc;
        sljit_label* exportEntryLabel;
        bool isExported;
        size_t branchTableSize;
    };

    void append(InstructionListItem* item);

    // Backend operations.
    void emitProlog();
    void emitEpilog();

#if !defined(NDEBUG)
    static const char* m_byteCodeNames[];
#endif /* !NDEBUG */

    InstructionListItem* m_first;
    InstructionListItem* m_last;

    sljit_compiler* m_compiler;
    CompileContext m_context;
    Module* m_module;
    ModuleFunction* m_moduleFunction;
    VariableList* m_variableList;
    size_t m_branchTableSize;
    // Start inside the m_tryBlocks vector.
    size_t m_tryBlockStart;
    // Start inside the instance const data.
    size_t m_tryBlockOffset;
    uint32_t m_JITFlags;
    uint32_t m_options;
    uint8_t m_savedIntegerRegCount;
    uint8_t m_savedFloatRegCount;

    std::vector<TryBlock> m_tryBlocks;
    std::vector<FunctionList> m_functionList;
#if defined(WALRUS_JITPERF) && !defined(NDEBUG)
    std::vector<DebugEntry> m_debugEntries;
#endif /* WALRUS_JITPERF && !NDEBUG */
};

} // namespace Walrus

#endif // __WalrusJITCompiler__
