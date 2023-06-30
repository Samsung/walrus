/*
 * Copyright 2020 WebAssembly Community Group participants
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

#include "interpreter/Interpreter.h"
#include "runtime/Instance.h"
#include "runtime/Function.h"
#include "runtime/Memory.h"
#include "runtime/Table.h"
#include "runtime/Global.h"
#include "runtime/Module.h"
#include "runtime/Trap.h"
#include "runtime/Tag.h"
#include "util/MathOperation.h"

#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
extern char FillByteCodeOpcodeTableAsmLbl[];
const void* FillByteCodeOpcodeAddress[] = { &FillByteCodeOpcodeTableAsmLbl[0] };
#endif

namespace Walrus {

ByteCodeTable g_byteCodeTable;


// SIMD Structures
template <typename T, uint8_t L>
struct SIMDValue {
    using LaneType = T;
    static constexpr uint8_t Lanes = L;

    T v[L];

    inline T& operator[](uint8_t idx)
    {
#if defined(WALRUS_BIG_ENDIAN)
        idx = (~idx) & (L - 1);
#endif
        ASSERT(idx < L);
        return v[idx];
    }
};

typedef SIMDValue<uint8_t, 16> U8x16;
typedef SIMDValue<uint16_t, 8> U16x8;
typedef SIMDValue<uint32_t, 4> U32x4;
typedef SIMDValue<uint64_t, 2> U64x2;

COMPILE_ASSERT(sizeof(U8x16) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(U16x8) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(U32x4) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(U64x2) == sizeof(Vec128), "");

template <typename T>
struct SIMDType;
template <>
struct SIMDType<uint8_t> {
    using Type = U8x16;
};
template <>
struct SIMDType<uint16_t> {
    using Type = U16x8;
};
template <>
struct SIMDType<uint32_t> {
    using Type = U32x4;
};
template <>
struct SIMDType<uint64_t> {
    using Type = U64x2;
};

ByteCodeTable::ByteCodeTable()
{
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    // Dummy bytecode execution to initialize the ByteCodeTable.
    ExecutionState dummyState;
    ByteCode b;
#if defined(WALRUS_COMPUTED_GOTO_INTERPRETER_INIT_WITH_NULL)
    b.m_opcodeInAddress = nullptr;
#else
    b.m_opcodeInAddress = const_cast<void*>(FillByteCodeOpcodeAddress[0]);
#endif
    size_t pc = reinterpret_cast<size_t>(&b);
    Interpreter::interpret(dummyState, pc, nullptr, nullptr, nullptr, nullptr, nullptr);
#endif
}

ByteCodeStackOffset* Interpreter::interpret(ExecutionState& state,
                                            uint8_t* bp)
{
    DefinedFunction* df = state.currentFunction()->asDefinedFunction();
    ModuleFunction* mf = df->moduleFunction();
    size_t programCounter = reinterpret_cast<size_t>(mf->byteCode());
    Instance* instance = df->instance();
    while (true) {
        try {
            return interpret(state, programCounter, bp, instance, instance->m_memories, instance->m_tables, instance->m_globals);
        } catch (std::unique_ptr<Exception>& e) {
            for (size_t i = e->m_programCounterInfo.size(); i > 0; i--) {
                if (e->m_programCounterInfo[i - 1].first == &state) {
                    programCounter = e->m_programCounterInfo[i - 1].second;
                    break;
                }
            }
            if (e->isUserException()) {
                bool isCatchSucessful = false;
                Tag* tag = e->tag().value();
                size_t offset = programCounter - reinterpret_cast<size_t>(mf->byteCode());
                for (const auto& item : mf->catchInfo()) {
                    if (item.m_tryStart <= offset && offset < item.m_tryEnd) {
                        if (item.m_tagIndex == std::numeric_limits<uint32_t>::max() || state.currentFunction()->asDefinedFunction()->instance()->tag(item.m_tagIndex) == tag) {
                            programCounter = item.m_catchStartPosition + reinterpret_cast<size_t>(mf->byteCode());
                            uint8_t* sp = bp + item.m_stackSizeToBe;
                            if (item.m_tagIndex != std::numeric_limits<uint32_t>::max() && tag->functionType()->paramStackSize()) {
                                memcpy(sp, e->userExceptionData().data(), tag->functionType()->paramStackSize());
                            }
                            isCatchSucessful = true;
                            break;
                        }
                    }
                }
                if (isCatchSucessful) {
                    continue;
                }
            }
            throw std::unique_ptr<Exception>(std::move(e));
        }
    }
}

template <typename T>
ALWAYS_INLINE void writeValue(uint8_t* bp, ByteCodeStackOffset offset, const T& v)
{
    *reinterpret_cast<T*>(bp + offset) = v;
}

template <typename T>
ALWAYS_INLINE T readValue(uint8_t* bp, ByteCodeStackOffset offset)
{
    return *reinterpret_cast<T*>(bp + offset);
}

template <typename T>
bool intEqz(T val) { return val == 0; }
template <typename T>
bool eq(ExecutionState& state, T lhs, T rhs) { return lhs == rhs; }
template <typename T>
bool ne(ExecutionState& state, T lhs, T rhs) { return lhs != rhs; }
template <typename T>
bool lt(ExecutionState& state, T lhs, T rhs) { return lhs < rhs; }
template <typename T>
bool le(ExecutionState& state, T lhs, T rhs) { return lhs <= rhs; }
template <typename T>
bool gt(ExecutionState& state, T lhs, T rhs) { return lhs > rhs; }
template <typename T>
bool ge(ExecutionState& state, T lhs, T rhs) { return lhs >= rhs; }
template <typename T>
T add(ExecutionState& state, T lhs, T rhs) { return canonNaN(lhs + rhs); }
template <typename T>
T sub(ExecutionState& state, T lhs, T rhs) { return canonNaN(lhs - rhs); }
template <typename T>
T xchg(ExecutionState& state, T lhs, T rhs) { return rhs; }
template <typename T>
T intAnd(ExecutionState& state, T lhs, T rhs) { return lhs & rhs; }
template <typename T>
T intOr(ExecutionState& state, T lhs, T rhs) { return lhs | rhs; }
template <typename T>
T intXor(ExecutionState& state, T lhs, T rhs) { return lhs ^ rhs; }
template <typename T>
T intShl(ExecutionState& state, T lhs, T rhs) { return lhs << shiftMask(rhs); }
template <typename T>
T intShr(ExecutionState& state, T lhs, T rhs) { return lhs >> shiftMask(rhs); }
template <typename T>
T intMin(ExecutionState& state, T lhs, T rhs) { return std::min(lhs, rhs); }
template <typename T>
T intMax(ExecutionState& state, T lhs, T rhs) { return std::max(lhs, rhs); }
template <typename T>
T intAndNot(ExecutionState& state, T lhs, T rhs) { return lhs & ~rhs; }
template <typename T>
T intClz(T val) { return clz(val); }
template <typename T>
T intCtz(T val) { return ctz(val); }
template <typename T>
T intPopcnt(T val) { return popCount(val); }
template <typename T>
T intNot(T val) { return ~val; }
template <typename T>
T intNeg(T val) { return ~val + 1; }
template <typename T>
T intAvgr(ExecutionState& state, T lhs, T rhs) { return (lhs + rhs + 1) / 2; }

template <typename T>
T intDiv(ExecutionState& state, T lhs, T rhs)
{
    if (UNLIKELY(rhs == 0)) {
        Trap::throwException(state, "integer divide by zero");
    }
    if (UNLIKELY(!isNormalDivRem(lhs, rhs))) {
        Trap::throwException(state, "integer overflow");
    }
    return lhs / rhs;
}

template <typename T>
T intRem(ExecutionState& state, T lhs, T rhs)
{
    if (UNLIKELY(rhs == 0)) {
        Trap::throwException(state, "integer divide by zero");
    }
    if (LIKELY(isNormalDivRem(lhs, rhs))) {
        return lhs % rhs;
    } else {
        return 0;
    }
}

template <typename R, typename T>
R doConvert(ExecutionState& state, T val)
{
    if (std::is_integral<R>::value && std::is_floating_point<T>::value) {
        // Don't use std::isnan here because T may be a non-floating-point type.
        if (UNLIKELY(isNaN(val))) {
            Trap::throwException(state, "invalid conversion to integer");
        }
    }
    if (UNLIKELY(!canConvert<R>(val))) {
        Trap::throwException(state, "integer overflow");
    }
    return convert<R>(val);
}

#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
static void initAddressToOpcodeTable()
{
#define REGISTER_TABLE(name, ...) \
    g_byteCodeTable.m_addressToOpcodeTable[g_byteCodeTable.m_addressTable[ByteCode::name##Opcode]] = ByteCode::name##Opcode;
    FOR_EACH_BYTECODE(REGISTER_TABLE)
#undef REGISTER_TABLE
}
#endif

ByteCodeStackOffset* Interpreter::interpret(ExecutionState& state,
                                            size_t programCounter,
                                            uint8_t* bp,
                                            Instance* instance,
                                            Memory** memories,
                                            Table** tables,
                                            Global** globals)
{
    state.m_programCounterPointer = &programCounter;

#define ADD_PROGRAM_COUNTER(codeName) programCounter += sizeof(codeName);

#define BINARY_OPERATION(name, op, paramType, returnType)                   \
    DEFINE_OPCODE(name)                                                     \
        :                                                                   \
    {                                                                       \
        name* code = (name*)programCounter;                                 \
        auto lhs = readValue<paramType>(bp, code->srcOffset()[0]);          \
        auto rhs = readValue<paramType>(bp, code->srcOffset()[1]);          \
        writeValue<returnType>(bp, code->dstOffset(), op(state, lhs, rhs)); \
        ADD_PROGRAM_COUNTER(name);                                          \
        NEXT_INSTRUCTION();                                                 \
    }

#define UNARY_OPERATION(name, op, type)                                                      \
    DEFINE_OPCODE(name)                                                                      \
        :                                                                                    \
    {                                                                                        \
        name* code = (name*)programCounter;                                                  \
        writeValue<type>(bp, code->dstOffset(), op(readValue<type>(bp, code->srcOffset()))); \
        ADD_PROGRAM_COUNTER(name);                                                           \
        NEXT_INSTRUCTION();                                                                  \
    }

#define UNARY_OPERATION_2(name, op, paramType, returnType, T1, T2)                                                     \
    DEFINE_OPCODE(name)                                                                                                \
        :                                                                                                              \
    {                                                                                                                  \
        name* code = (name*)programCounter;                                                                            \
        writeValue<returnType>(bp, code->dstOffset(), op<T1, T2>(state, readValue<paramType>(bp, code->srcOffset()))); \
        ADD_PROGRAM_COUNTER(name);                                                                                     \
        NEXT_INSTRUCTION();                                                                                            \
    }

#define SIMD_BINARY_OPERATION(name, op, paramType, returnType) \
    DEFINE_OPCODE(name)                                        \
        :                                                      \
    {                                                          \
        using Type = typename SIMDType<paramType>::Type;       \
        name* code = (name*)programCounter;                    \
        auto lhs = readValue<Type>(bp, code->srcOffset()[0]);  \
        auto rhs = readValue<Type>(bp, code->srcOffset()[1]);  \
        Type result;                                           \
        for (uint8_t i = 0; i < Type::Lanes; i++) {            \
            result[i] = op(state, lhs[i], rhs[i]);             \
        }                                                      \
        writeValue<Type>(bp, code->dstOffset(), result);       \
        ADD_PROGRAM_COUNTER(name);                             \
        NEXT_INSTRUCTION();                                    \
    }

#define SIMD_UNARY_OPERATION(name, op, type)                                                \
    DEFINE_OPCODE(name)                                                                     \
        :                                                                                   \
    {                                                                                       \
        using Type = typename SIMDType<type>::Type;                                         \
        name* code = (name*)programCounter;                                                 \
        auto val = readValue<Type>(bp, code->srcOffset());                                  \
        Type result;                                                                        \
        std::transform(std::begin(val.v), std::end(val.v), std::begin(result.v), op<type>); \
        writeValue<Type>(bp, code->dstOffset(), result);                                    \
        ADD_PROGRAM_COUNTER(name);                                                          \
        NEXT_INSTRUCTION();                                                                 \
    }

#define MEMORY_LOAD_OPERATION(opcodeName, readType, writeType)        \
    DEFINE_OPCODE(opcodeName)                                         \
        :                                                             \
    {                                                                 \
        MemoryLoad* code = (MemoryLoad*)programCounter;               \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset()); \
        readType value;                                               \
        memories[0]->load(state, offset, code->offset(), &value);     \
        writeValue<writeType>(bp, code->dstOffset(), value);          \
        ADD_PROGRAM_COUNTER(MemoryLoad);                              \
        NEXT_INSTRUCTION();                                           \
    }

#define MEMORY_STORE_OPERATION(opcodeName, readType, writeType)        \
    DEFINE_OPCODE(opcodeName)                                          \
        :                                                              \
    {                                                                  \
        MemoryStore* code = (MemoryStore*)programCounter;              \
        writeType value = readValue<readType>(bp, code->src1Offset()); \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset()); \
        memories[0]->store(state, offset, code->offset(), value);      \
        ADD_PROGRAM_COUNTER(MemoryStore);                              \
        NEXT_INSTRUCTION();                                            \
    }


#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
#if defined(WALRUS_COMPUTED_GOTO_INTERPRETER_INIT_WITH_NULL)
    if (UNLIKELY((((ByteCode*)programCounter)->m_opcodeInAddress) == NULL)) {
        goto FillOpcodeTableOpcodeLbl;
    }
#endif

#define DEFINE_OPCODE(codeName) codeName##OpcodeLbl
#define DEFINE_DEFAULT
#define NEXT_INSTRUCTION() goto NextInstruction;

NextInstruction:
    /* Execute first instruction. */
    goto*(((ByteCode*)programCounter)->m_opcodeInAddress);
#else

#define DEFINE_OPCODE(codeName) case codeName##Opcode
#define DEFINE_DEFAULT                \
    default:                          \
        RELEASE_ASSERT_NOT_REACHED(); \
        }
#define NEXT_INSTRUCTION() \
    goto NextInstruction;
NextInstruction:
    auto currentOpcode = ((ByteCode*)programCounter)->m_opcode;

    switch (currentOpcode) {
#endif

    DEFINE_OPCODE(Const32)
        :
    {
        Const32* code = (Const32*)programCounter;
        *reinterpret_cast<uint32_t*>(bp + code->dstOffset()) = code->value();
        ADD_PROGRAM_COUNTER(Const32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Const64)
        :
    {
        Const64* code = (Const64*)programCounter;
        *reinterpret_cast<uint64_t*>(bp + code->dstOffset()) = code->value();
        ADD_PROGRAM_COUNTER(Const64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Const128)
        :
    {
        Const128* code = (Const128*)programCounter;
        memcpy(bp + code->dstOffset(), code->value(), 16);
        ADD_PROGRAM_COUNTER(Const128);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Move32)
        :
    {
        Move32* code = (Move32*)programCounter;
        *reinterpret_cast<uint32_t*>(bp + code->dstOffset()) = *reinterpret_cast<uint32_t*>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(Move32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Move64)
        :
    {
        Move64* code = (Move64*)programCounter;
        *reinterpret_cast<uint64_t*>(bp + code->dstOffset()) = *reinterpret_cast<uint64_t*>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(Move64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Move128)
        :
    {
        Move128* code = (Move128*)programCounter;
        memcpy(bp + code->dstOffset(), bp + code->srcOffset(), 16);
        ADD_PROGRAM_COUNTER(Move128);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Load32)
        :
    {
        Load32* code = (Load32*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        memories[0]->load(state, offset, reinterpret_cast<uint32_t*>(bp + code->dstOffset()));
        ADD_PROGRAM_COUNTER(Load32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Load64)
        :
    {
        Load64* code = (Load64*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        memories[0]->load(state, offset, reinterpret_cast<uint64_t*>(bp + code->dstOffset()));
        ADD_PROGRAM_COUNTER(Load64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Store32)
        :
    {
        Store32* code = (Store32*)programCounter;
        uint32_t value = readValue<uint32_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        memories[0]->store(state, offset, value);
        ADD_PROGRAM_COUNTER(Store32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Store64)
        :
    {
        Store64* code = (Store64*)programCounter;
        uint64_t value = readValue<uint64_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        memories[0]->store(state, offset, value);
        ADD_PROGRAM_COUNTER(Store64);
        NEXT_INSTRUCTION();
    }

    FOR_EACH_BYTECODE_BINARY_OP(BINARY_OPERATION)
    FOR_EACH_BYTECODE_UNARY_OP(UNARY_OPERATION)
    FOR_EACH_BYTECODE_UNARY_OP_2(UNARY_OPERATION_2)
    FOR_EACH_BYTECODE_SIMD_BINARY_OP(SIMD_BINARY_OPERATION)
    FOR_EACH_BYTECODE_SIMD_UNARY_OP(SIMD_UNARY_OPERATION)

    DEFINE_OPCODE(Jump)
        :
    {
        Jump* code = (Jump*)programCounter;
        programCounter += code->offset();
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(JumpIfTrue)
        :
    {
        JumpIfTrue* code = (JumpIfTrue*)programCounter;
        if (readValue<int32_t>(bp, code->srcOffset())) {
            programCounter += code->offset();
        } else {
            ADD_PROGRAM_COUNTER(JumpIfTrue);
        }
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(JumpIfFalse)
        :
    {
        JumpIfFalse* code = (JumpIfFalse*)programCounter;
        if (readValue<int32_t>(bp, code->srcOffset())) {
            ADD_PROGRAM_COUNTER(JumpIfFalse);
        } else {
            programCounter += code->offset();
        }
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Call)
        :
    {
        callOperation(state, programCounter, bp, instance);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(CallIndirect)
        :
    {
        callIndirectOperation(state, programCounter, bp, instance);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Select)
        :
    {
        Select* code = (Select*)programCounter;
        auto cond = readValue<int32_t>(bp, code->condOffset());
        if (cond) {
            memmove(bp + code->dstOffset(), bp + code->src0Offset(), code->valueSize());
        } else {
            memmove(bp + code->dstOffset(), bp + code->src1Offset(), code->valueSize());
        }

        ADD_PROGRAM_COUNTER(Select);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(BrTable)
        :
    {
        BrTable* code = (BrTable*)programCounter;
        uint32_t value = readValue<uint32_t>(bp, code->condOffset());

        if (value >= code->tableSize()) {
            // default case
            programCounter += code->defaultOffset();
        } else {
            programCounter += code->jumpOffsets()[value];
        }
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalGet32)
        :
    {
        GlobalGet32* code = (GlobalGet32*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        globals[code->index()]->value().writeNBytesToMemory<4>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalGet64)
        :
    {
        GlobalGet64* code = (GlobalGet64*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        globals[code->index()]->value().writeNBytesToMemory<8>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalGet128)
        :
    {
        GlobalGet128* code = (GlobalGet128*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        globals[code->index()]->value().writeNBytesToMemory<16>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet128);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet32)
        :
    {
        GlobalSet32* code = (GlobalSet32*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        Value& val = globals[code->index()]->value();
        val.readFromStack<4>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet64)
        :
    {
        GlobalSet64* code = (GlobalSet64*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        Value& val = globals[code->index()]->value();
        val.readFromStack<8>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet128)
        :
    {
        GlobalSet128* code = (GlobalSet128*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        Value& val = globals[code->index()]->value();
        val.readFromStack<16>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet128);
        NEXT_INSTRUCTION();
    }

    FOR_EACH_BYTECODE_LOAD_OP(MEMORY_LOAD_OPERATION)
    FOR_EACH_BYTECODE_STORE_OP(MEMORY_STORE_OPERATION)

    DEFINE_OPCODE(MemorySize)
        :
    {
        MemorySize* code = (MemorySize*)programCounter;
        writeValue<int32_t>(bp, code->dstOffset(), memories[0]->sizeInPageSize());
        ADD_PROGRAM_COUNTER(MemorySize);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryGrow)
        :
    {
        MemoryGrow* code = (MemoryGrow*)programCounter;
        Memory* m = memories[0];
        auto oldSize = m->sizeInPageSize();
        if (m->grow(readValue<int32_t>(bp, code->srcOffset()) * (uint64_t)Memory::s_memoryPageSize)) {
            writeValue<int32_t>(bp, code->dstOffset(), oldSize);
        } else {
            writeValue<int32_t>(bp, code->dstOffset(), -1);
        }
        ADD_PROGRAM_COUNTER(MemoryGrow);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryInit)
        :
    {
        MemoryInit* code = (MemoryInit*)programCounter;
        Memory* m = memories[0];
        DataSegment& sg = instance->dataSegment(code->segmentIndex());
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->init(state, &sg, dstStart, srcStart, size);
        ADD_PROGRAM_COUNTER(MemoryInit);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryCopy)
        :
    {
        MemoryCopy* code = (MemoryCopy*)programCounter;
        Memory* m = memories[0];
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->copy(state, dstStart, srcStart, size);
        ADD_PROGRAM_COUNTER(MemoryCopy);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryFill)
        :
    {
        MemoryFill* code = (MemoryFill*)programCounter;
        Memory* m = memories[0];
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto value = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->fill(state, dstStart, value, size);
        ADD_PROGRAM_COUNTER(MemoryFill);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(DataDrop)
        :
    {
        DataDrop* code = (DataDrop*)programCounter;
        DataSegment& sg = instance->dataSegment(code->segmentIndex());
        sg.drop();
        ADD_PROGRAM_COUNTER(DataDrop);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableGet)
        :
    {
        TableGet* code = (TableGet*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = tables[code->tableIndex()];
        void* val = table->getElement(state, readValue<uint32_t>(bp, code->srcOffset()));
        writeValue(bp, code->dstOffset(), val);

        ADD_PROGRAM_COUNTER(TableGet);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableSet)
        :
    {
        TableSet* code = (TableSet*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = tables[code->tableIndex()];
        void* ptr = readValue<void*>(bp, code->src1Offset());
        table->setElement(state, readValue<uint32_t>(bp, code->src0Offset()), ptr);

        ADD_PROGRAM_COUNTER(TableSet);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableGrow)
        :
    {
        TableGrow* code = (TableGrow*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = tables[code->tableIndex()];
        size_t size = table->size();

        uint64_t newSize = (uint64_t)readValue<uint32_t>(bp, code->src1Offset()) + size;
        // FIXME read reference
        void* ptr = readValue<void*>(bp, code->src0Offset());

        if (newSize <= table->maximumSize()) {
            table->grow(newSize, ptr);
            writeValue<uint32_t>(bp, code->dstOffset(), size);
        } else {
            writeValue<uint32_t>(bp, code->dstOffset(), -1);
        }

        ADD_PROGRAM_COUNTER(TableGrow);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableSize)
        :
    {
        TableSize* code = (TableSize*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = tables[code->tableIndex()];
        size_t size = table->size();
        writeValue<uint32_t>(bp, code->dstOffset(), size);

        ADD_PROGRAM_COUNTER(TableSize);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableCopy)
        :
    {
        TableCopy* code = (TableCopy*)programCounter;
        ASSERT(code->dstIndex() < instance->module()->numberOfTableTypes());
        ASSERT(code->srcIndex() < instance->module()->numberOfTableTypes());
        Table* dstTable = tables[code->dstIndex()];
        Table* srcTable = tables[code->srcIndex()];

        uint32_t dstIndex = readValue<uint32_t>(bp, code->srcOffsets()[0]);
        uint32_t srcIndex = readValue<uint32_t>(bp, code->srcOffsets()[1]);
        uint32_t n = readValue<uint32_t>(bp, code->srcOffsets()[2]);

        dstTable->copy(state, srcTable, n, srcIndex, dstIndex);

        ADD_PROGRAM_COUNTER(TableCopy);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableFill)
        :
    {
        TableFill* code = (TableFill*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = tables[code->tableIndex()];

        int32_t index = readValue<int32_t>(bp, code->srcOffsets()[0]);
        void* ptr = readValue<void*>(bp, code->srcOffsets()[1]);
        int32_t n = readValue<int32_t>(bp, code->srcOffsets()[2]);
        table->fill(state, n, ptr, index);

        ADD_PROGRAM_COUNTER(TableFill);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableInit)
        :
    {
        TableInit* code = (TableInit*)programCounter;
        ElementSegment& sg = instance->elementSegment(code->segmentIndex());

        int32_t dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        int32_t srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        int32_t size = readValue<int32_t>(bp, code->srcOffsets()[2]);

        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = tables[code->tableIndex()];
        table->init(state, instance, &sg, dstStart, srcStart, size);
        ADD_PROGRAM_COUNTER(TableInit);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(ElemDrop)
        :
    {
        ElemDrop* code = (ElemDrop*)programCounter;
        instance->elementSegment(code->segmentIndex()).drop();
        ADD_PROGRAM_COUNTER(ElemDrop);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(RefFunc)
        :
    {
        RefFunc* code = (RefFunc*)programCounter;
        Value(instance->function(code->funcIndex())).writeToMemory(code->dstOffset() + bp);

        ADD_PROGRAM_COUNTER(RefFunc);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Throw)
        :
    {
        Throw* code = (Throw*)programCounter;
        Tag* tag = instance->tag(code->tagIndex());
        Vector<uint8_t> userExceptionData;
        size_t sz = tag->functionType()->paramStackSize();
        userExceptionData.resizeWithUninitializedValues(sz);

        uint8_t* ptr = userExceptionData.data();
        auto& param = tag->functionType()->param();
        for (size_t i = 0; i < param.size(); i++) {
            auto sz = valueSizeInStack(param[i]);
            memcpy(ptr, bp + code->dataOffsets()[i], sz);
            ptr += sz;
        }
        Trap::throwException(state, tag, std::move(userExceptionData));
        ASSERT_NOT_REACHED();
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Unreachable)
        :
    {
        Trap::throwException(state, "unreachable executed");
        ASSERT_NOT_REACHED();
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(End)
        :
    {
        End* code = (End*)programCounter;
        return code->resultOffsets();
    }

    DEFINE_OPCODE(FillOpcodeTable)
        :
    {
#if defined(COMPILER_GCC) && __GNUC__ >= 9
        __attribute__((cold));
#endif
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
        asm volatile("FillByteCodeOpcodeTableAsmLbl:");

#define REGISTER_TABLE(name, ...) \
    g_byteCodeTable.m_addressTable[ByteCode::name##Opcode] = &&name##OpcodeLbl;
        FOR_EACH_BYTECODE(REGISTER_TABLE)
#undef REGISTER_TABLE
#endif
        initAddressToOpcodeTable();
        return nullptr;
    }

    DEFINE_DEFAULT

    return nullptr;
}

NEVER_INLINE void Interpreter::callOperation(
    ExecutionState& state,
    size_t& programCounter,
    uint8_t* bp,
    Instance* instance)
{
    Call* code = (Call*)programCounter;

    Function* target = instance->function(code->index());
    const FunctionType* ft = target->functionType();
    const ValueTypeVector& param = ft->param();
    ALLOCA(Value, paramVector, sizeof(Value) * param.size(), isAllocaParam);

    size_t c = 0;
    for (size_t i = 0; i < param.size(); i++) {
        paramVector[i] = Value(param[i], bp + code->stackOffsets()[c++]);
    }

    const ValueTypeVector& result = ft->result();
    ALLOCA(Value, resultVector, sizeof(Value) * result.size(), isAllocaResult);
    size_t codeExtraOffsetsSize = sizeof(ByteCodeStackOffset) * ft->param().size() + sizeof(ByteCodeStackOffset) * ft->result().size();

    target->call(state, param.size(), paramVector, resultVector);

    for (size_t i = 0; i < result.size(); i++) {
        uint8_t* resultStackPointer = bp + code->stackOffsets()[c++];
        resultVector[i].writeToMemory(resultStackPointer);
    }

    if (UNLIKELY(!isAllocaParam)) {
        delete[] paramVector;
    }
    if (UNLIKELY(!isAllocaResult)) {
        delete[] resultVector;
    }

    programCounter += sizeof(Call) + codeExtraOffsetsSize;
}

NEVER_INLINE void Interpreter::callIndirectOperation(
    ExecutionState& state,
    size_t& programCounter,
    uint8_t* bp,
    Instance* instance)
{
    CallIndirect* code = (CallIndirect*)programCounter;
    Table* table = instance->table(code->tableIndex());

    uint32_t idx = readValue<uint32_t>(bp, code->calleeOffset());
    if (idx >= table->size()) {
        Trap::throwException(state, "undefined element");
    }
    auto target = reinterpret_cast<Function*>(table->uncheckedGetElement(idx));
    if (UNLIKELY(Value::isNull(target))) {
        Trap::throwException(state, "uninitialized element " + std::to_string(idx));
    }
    const FunctionType* ft = target->functionType();
    if (!ft->equals(code->functionType())) {
        Trap::throwException(state, "indirect call type mismatch");
    }
    const ValueTypeVector& param = ft->param();
    ALLOCA(Value, paramVector, sizeof(Value) * param.size(), isAllocaParam);

    size_t c = 0;
    for (size_t i = 0; i < param.size(); i++) {
        paramVector[i] = Value(param[i], bp + code->stackOffsets()[c++]);
    }

    const ValueTypeVector& result = ft->result();
    ALLOCA(Value, resultVector, sizeof(Value) * result.size(), isAllocaResult);
    size_t codeExtraOffsetsSize = sizeof(ByteCodeStackOffset) * ft->param().size() + sizeof(ByteCodeStackOffset) * ft->result().size();

    target->call(state, param.size(), paramVector, resultVector);

    for (size_t i = 0; i < result.size(); i++) {
        uint8_t* resultStackPointer = bp + code->stackOffsets()[c++];
        resultVector[i].writeToMemory(resultStackPointer);
    }

    if (UNLIKELY(!isAllocaParam)) {
        delete[] paramVector;
    }
    if (UNLIKELY(!isAllocaResult)) {
        delete[] resultVector;
    }

    programCounter += sizeof(CallIndirect) + codeExtraOffsetsSize;
}

} // namespace Walrus
