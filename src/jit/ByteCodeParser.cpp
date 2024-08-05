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
#include "runtime/JITExec.h"
#include "runtime/Module.h"

#include <map>

#if defined(COMPILER_MSVC)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

namespace Walrus {

#define COMPUTE_OFFSET(idx, offset) \
    (static_cast<size_t>(static_cast<ssize_t>(idx) + (offset)))

class TryRange {
public:
    TryRange(size_t start, size_t end)
        : m_start(start)
        , m_end(end)
    {
    }

    size_t start() const { return m_start; }
    size_t end() const { return m_end; }

    bool operator<(const TryRange& other) const
    {
        return m_start < other.m_start || (m_start == other.m_start && m_end > other.m_end);
    }

private:
    size_t m_start;
    size_t m_end;
};

static void buildCatchInfo(JITCompiler* compiler, ModuleFunction* function, std::map<size_t, Label*>& labels)
{
    std::map<TryRange, size_t> ranges;

    for (auto it : function->catchInfo()) {
        ranges[TryRange(it.m_tryStart, it.m_tryEnd)]++;
    }

    std::vector<TryBlock>& tryBlocks = compiler->tryBlocks();
    size_t counter = tryBlocks.size();

    tryBlocks.reserve(counter + ranges.size());

    // The it->second assignment does not work with auto iterator.
    for (std::map<TryRange, size_t>::iterator it = ranges.begin(); it != ranges.end(); it++) {
        Label* start = labels[it->first.start()];

        start->addInfo(Label::kHasTryInfo);

        tryBlocks.push_back(TryBlock(start, it->second));
        it->second = counter++;
    }

    for (auto it : function->catchInfo()) {
        Label* catchLabel = labels[it.m_catchStartPosition];
        size_t idx = ranges[TryRange(it.m_tryStart, it.m_tryEnd)];

        if (tryBlocks[idx].catchBlocks.size() == 0) {
            // The first catch block terminates the try block.
            catchLabel->addInfo(Label::kHasCatchInfo);
        }

        tryBlocks[idx].catchBlocks.push_back(TryBlock::CatchBlock(catchLabel, it.m_stackSizeToBe, it.m_tagIndex));
    }
}

static bool isFloatGlobal(uint32_t globalIndex, Module* module)
{
    Value::Type type = module->globalType(globalIndex)->type();

    return type == Value::F32 || type == Value::F64;
}

// Helpers for simplifying descriptor definitions.

#define I64_LOW (Instruction::Int64LowOperand | Instruction::TmpRequired)
#define I32 Instruction::Int32Operand
#define I64 Instruction::Int64Operand
#define V128 Instruction::V128Operand
#define F32 Instruction::Float32Operand
#define F64 Instruction::Float64Operand
#define TMP Instruction::TmpRequired
#define NOTMP Instruction::TmpNotAllowed
#define LOW Instruction::LowerHalfNeeded
#define S0 Instruction::Src0Allowed
#define S1 Instruction::Src1Allowed
#define S2 Instruction::Src2Allowed

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
#define PTR I32
#else /* !SLJIT_32BIT_ARCHITECTURE */
#define PTR I64
#endif /* SLJIT_32BIT_ARCHITECTURE */

// S/D/T represents Source Destination operands and Temporary registers.
// Example: SSDTT represents two source and one destination operands and two temporary registers.

// General rules:
//    - Registers assigned to source operands are read only, unless
//      a destination/temporary register is also assigned to a source register
//    - Destination operands may also require to be temporary registers
//    - Destination/temporary registers must be different registers
//    - Destination/temporary registers can reuse source registers
//      when Src0Allowed .. Src2Allowed flags are specified
//    - On 32 bit systems, i64 types requires two registers
//      The lower 32 bit (LowerHalfNeeded) might be enough for source registers
//    - On 32 bit systems, i64 types cannot be assigned to temporary registers

// Operand examples:
//   [SOURCE_TYPE] : A register is auto-assigned for float/simd immediates,
//                   no effect otherwise
//   [SOURCE_TYPE] | TMP : A temporary register must be assigned which is not modified
//   [SOURCE_TYPE] | NOTMP : A register is never assigned to this source value
//   [DESTINATION_TYPE] : No register is assigned to this destination operand
//   [DESTINATION_TYPE] | TMP : A temporary register must be required for this destination operand
//   [TEMPORARY_TYPE] | S0 | S2 : A temporary register is required, which can
//                                be the same as the first or third source operands

#define OPERAND_TYPE_LIST                                                               \
    OL2(OTOp1I32, /* SD */ I32, I32 | S0)                                               \
    OL3(OTOp2I32, /* SSD */ I32, I32, I32 | S0 | S1)                                    \
    OL2(OTOp1I64, /* SD */ I64, I64 | S0)                                               \
    OL2(OTOp1F32, /* SD */ F32, F32 | S0)                                               \
    OL2(OTOp1F64, /* SD */ F64, F64 | S0)                                               \
    OL3(OTOp2F32, /* SSD */ F32, F32, F32 | S0 | S1)                                    \
    OL3(OTOp2F64, /* SSD */ F64, F64, F64 | S0 | S1)                                    \
    OL1(OTGetI32, /* S */ I32)                                                          \
    OL1(OTPutI32, /* D */ I32)                                                          \
    OL1(OTPutI64, /* D */ I64)                                                          \
    OL1(OTPutV128, /* D */ V128)                                                        \
    OL1(OTPutPTR, /* D */ PTR)                                                          \
    OL2(OTMoveF32, /* SD */ F32 | NOTMP, F32 | S0)                                      \
    OL2(OTMoveF64, /* SD */ F64 | NOTMP, F64 | S0)                                      \
    OL2(OTMoveV128, /* SD */ V128, V128 | S0)                                           \
    OL2(OTI32ReinterpretF32, /* SD */ F32, I32)                                         \
    OL2(OTI64ReinterpretF64, /* SD */ F64, I64)                                         \
    OL2(OTF32ReinterpretI32, /* SD */ I32, F32)                                         \
    OL2(OTF64ReinterpretI64, /* SD */ I64, F64)                                         \
    OL2(OTEqzI64, /* SD */ I64, I32)                                                    \
    OL3(OTCompareI64, /* SSD */ I64, I64, I32)                                          \
    OL3(OTCompareF32, /* SSD */ F32, F32, I32)                                          \
    OL3(OTCompareF64, /* SSD */ F64, F64, I32)                                          \
    OL3(OTCopySignF32, /* SSD */ F32, F32, F32 | TMP | S0 | S1)                         \
    OL3(OTCopySignF64, /* SSD */ F64, F64, F64 | TMP | S0 | S1)                         \
    OL2(OTDemoteF64, /* SD */ F64, F32 | S0)                                            \
    OL2(OTPromoteF32, /* SD */ F32, F64 | S0)                                           \
    OL4(OTLoadI32, /* SDTT */ I32, I32 | S0, PTR, I32 | S0)                             \
    OL4(OTLoadF32, /* SDTT */ I32, F32, PTR, I32 | S0)                                  \
    OL4(OTLoadF64, /* SDTT */ I32, F64, PTR, I32 | S0)                                  \
    OL4(OTLoadV128, /* SDTT */ I32, V128 | TMP, PTR, I32 | S0)                          \
    OL5(OTLoadLaneV128, /* SSDTTT */ I32, V128 | NOTMP, V128 | TMP | S1, PTR, I32 | S0) \
    OL5(OTStoreI32, /* SSTTT */ I32, I32, PTR, I32 | S0, I32 | S1)                      \
    OL4(OTStoreF32, /* SSTT */ I32, F32 | NOTMP, PTR, I32 | S0)                         \
    OL5(OTStoreI64, /* SSTTT */ I32, I64, PTR, I32 | S0, PTR | S1)                      \
    OL4(OTStoreF64, /* SSTT */ I32, F64 | NOTMP, PTR, I32 | S0)                         \
    OL4(OTStoreV128, /* SSTT */ I32, V128 | TMP, PTR, I32 | S0)                         \
    OL3(OTCallback3Arg, /* SSS */ I32, I32, I32)                                        \
    OL3(OTTableGrow, /* SSD */ I32, PTR, I32 | S0 | S1)                                 \
    OL4(OTTableSet, /* SSTT */ I32, PTR, I32 | S0, PTR)                                 \
    OL3(OTTableGet, /* SDT */ I32, PTR | TMP | S0, I32)                                 \
    OL1(OTGlobalGetF32, /* D */ F32)                                                    \
    OL1(OTGlobalGetF64, /* D */ F64)                                                    \
    OL2(OTGlobalSetI32, /* ST */ I32, PTR)                                              \
    OL2(OTGlobalSetI64, /* ST */ I64, PTR)                                              \
    OL1(OTGlobalSetF32, /* S */ F32 | NOTMP)                                            \
    OL1(OTGlobalSetF64, /* S */ F64 | NOTMP)                                            \
    OL2(OTConvertInt32FromInt64, /* SD */ I64, I32)                                     \
    OL2(OTConvertInt64FromInt32, /* SD */ I32, I64)                                     \
    OL2(OTConvertInt32FromFloat32, /* SD */ F32 | TMP, I32 | TMP)                       \
    OL2(OTConvertInt32FromFloat64, /* SD */ F64 | TMP, I32 | TMP)                       \
    OL2(OTConvertInt64FromFloat32Callback, /* SD */ F32, I64)                           \
    OL2(OTConvertInt64FromFloat64Callback, /* SD */ F64, I64)                           \
    OL2(OTConvertFloat32FromInt32, /* SD */ I32, F32)                                   \
    OL2(OTConvertFloat64FromInt32, /* SD */ I32, F64)                                   \
    OL2(OTConvertFloat32FromInt64, /* SD */ I64, F32)                                   \
    OL2(OTConvertFloat64FromInt64, /* SD */ I64, F64)                                   \
    OL4(OTSelectI32, /* SSSD */ I32, I32, I32, I32 | S0 | S1)                           \
    OL4(OTSelectF32, /* SSSD */ F32, F32, I32, F32 | S0 | S1)                           \
    OL4(OTSelectF64, /* SSSD */ F64, F64, I32, F64 | S0 | S1)

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)

#define OPERAND_TYPE_LIST_MATH                                              \
    OL3(OTOp2I64, /* SSD */ I64, I64, I64 | TMP | S0 | S1)                  \
    OL3(OTShiftI64, /* SSD */ I64, I64 | LOW, I64 | TMP | S0)               \
    OL3(OTMulI64, /* SSDT */ I64, I64, I64 | S0 | S1)                       \
    OL3(OTDivRemI64, /* SSD */ I64, I64, I64 | S0 | S1)                     \
    OL2(OTCountZeroesI64, /* SD */ I64, I64 | TMP | S0)                     \
    OL4(OTLoadI64, /* SDTT */ I32, I64, PTR, I32 | S0)                      \
    OL5(OTStoreI64Low, /* SSTTT */ I32, I64 | LOW, PTR, I32 | S0, PTR | S1) \
    OL1(OTGlobalGetI64, /* D */ I64_LOW)                                    \
    OL2(OTConvertInt32FromFloat32Callback, /* SD */ F32, I32)               \
    OL2(OTConvertInt32FromFloat64Callback, /* SD */ F64, I32)               \
    OL4(OTSelectI64, /* SSSD */ I64, I64, I32, I64_LOW | S0 | S1)

#else /* !SLJIT_32BIT_ARCHITECTURE */

#define OPERAND_TYPE_LIST_MATH                                    \
    OL3(OTOp2I64, /* SSD */ I64, I64, I64 | S0 | S1)              \
    OL4(OTLoadI64, /* SDTT */ I32, I64 | S0, PTR, I32 | S0)       \
    OL1(OTGlobalGetI64, /* D */ I64)                              \
    OL2(OTConvertInt64FromFloat32, /* SD */ F32 | TMP, I64 | TMP) \
    OL2(OTConvertInt64FromFloat64, /* SD */ F64 | TMP, I64 | TMP) \
    OL4(OTSelectI64, /* SSSD */ I64, I64, I32, I64 | S0 | S1)

#endif /* SLJIT_32BIT_ARCHITECTURE */

#if defined(ENABLE_EXTENDED_FEATURES)
#define OPERAND_TYPE_LIST_EXTENDED                                      \
    OL5(OTAtomicRmwI32, /* SSDTT */ I32, I32, I32 | TMP, PTR, I32 | S1) \
    OL5(OTAtomicRmwI64, /* SSDTT */ I32, I64, I64 | TMP, PTR, I64 | S1)
#else /* !ENABLE_EXTENDED_FEATURES */
#define OPERAND_TYPE_LIST_EXTENDED
#endif /* ENABLE_EXTENDED_FEATURES */

#define OPERAND_TYPE_LIST_SIMD                                                             \
    OL2(OTOp1V128, /* SD */ V128 | NOTMP, V128 | TMP | S0)                                 \
    OL2(OTOpCondV128, /* SD */ V128 | TMP, I32)                                            \
    OL1(OTGlobalGetV128, /* D */ V128)                                                     \
    OL1(OTGlobalSetV128, /* S */ V128 | NOTMP)                                             \
    OL2(OTSplatI32, /* SD */ I32, V128 | TMP)                                              \
    OL2(OTSplatI64, /* SD */ I64, V128 | TMP)                                              \
    OL2(OTSplatF32, /* SD */ F32 | NOTMP, V128 | TMP)                                      \
    OL2(OTSplatF64, /* SD */ F64 | NOTMP, V128 | TMP)                                      \
    OL2(OTV128ToI32, /* SD */ V128 | TMP, I32)                                             \
    OL4(OTBitSelectV128, /* SSSD */ V128 | TMP, V128 | TMP, V128 | NOTMP, V128 | TMP | S2) \
    OL2(OTExtractLaneI64, /* SD */ V128 | TMP, I64)                                        \
    OL2(OTExtractLaneF32, /* SD */ V128 | TMP, F32 | S0)                                   \
    OL2(OTExtractLaneF64, /* SD */ V128 | TMP, F64 | S0)                                   \
    OL3(OTReplaceLaneI32, /* SSD */ V128 | NOTMP, I32, V128 | TMP | S0)                    \
    OL3(OTReplaceLaneI64, /* SSD */ V128 | NOTMP, I64, V128 | TMP | S0)                    \
    OL3(OTReplaceLaneF32, /* SSD */ V128 | NOTMP, F32 | NOTMP, V128 | TMP | S0)            \
    OL3(OTReplaceLaneF64, /* SSD */ V128 | NOTMP, F64 | NOTMP, V128 | TMP | S0)            \
    OL4(OTSelectV128, /* SSSD */ V128, V128, I32, V128 | S0 | S1)

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)

#define OPERAND_TYPE_LIST_SIMD_ARCH                                               \
    OL3(OTOp2V128, /* SSD */ V128 | NOTMP, V128 | TMP, V128 | TMP | S0)           \
    OL3(OTOp1V128Tmp, /* SDT */ V128 | NOTMP, V128 | TMP | S0, V128)              \
    OL4(OTOp2V128Tmp, /* SSDT */ V128 | NOTMP, V128 | TMP, V128 | TMP | S0, V128) \
    OL3(OTOp2V128Rev, /* SSD */ V128 | TMP, V128 | NOTMP, V128 | TMP | S1)        \
    OL3(OTShuffleV128, /* SSD */ V128 | NOTMP, V128 | NOTMP, V128 | TMP | S0)     \
    OL3(OTPopcntV128, /* SDT */ V128 | NOTMP, V128 | TMP | S0, V128)              \
    OL3(OTShiftV128, /* SSD */ V128 | NOTMP, I32, V128 | TMP | S0)                \
    OL4(OTShiftV128Tmp, /* SSDT */ V128 | NOTMP, I32, V128 | TMP | S0, V128)

// List of aliases.
#define OTOp1V128CB OTOp1V128
#define OTMinMaxV128 OTOp2V128
#define OTPMinMaxV128 OTOp2V128
#define OTSwizzleV128 OTOp2V128

#elif (defined SLJIT_CONFIG_ARM_64 && SLJIT_CONFIG_ARM_64)

#define OPERAND_TYPE_LIST_SIMD_ARCH                                                \
    OL3(OTOp2V128, /* SSD */ V128 | TMP, V128 | TMP, V128 | TMP | S0 | S1)         \
    OL3(OTPMinMaxV128, /* SSD */ V128 | TMP, V128 | TMP, V128 | TMP)               \
    OL3(OTShuffleV128, /* SSD */ V128 | NOTMP, V128 | NOTMP, V128 | TMP | S0 | S1) \
    OL3(OTShiftV128, /* SSD */ V128 | NOTMP, I32, V128 | TMP | S0)

// List of aliases.
#define OTOp1V128Tmp OTOp1V128
#define OTOp1V128CB OTOp1V128
#define OTOp2V128Tmp OTOp2V128
#define OTOp2V128Rev OTOp2V128
#define OTMinMaxV128 OTOp2V128
#define OTPopcntV128 OTOp1V128
#define OTSwizzleV128 OTOp2V128
#define OTShiftV128Tmp OTShiftV128

#elif (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)

#define OPERAND_TYPE_LIST_SIMD_ARCH                                         \
    OL2(OTOp1V128CB, /* SD */ V128 | NOTMP, V128 | NOTMP)                   \
    OL3(OTOp2V128, /* SSD */ V128 | TMP, V128 | TMP, V128 | TMP | S0 | S1)  \
    OL3(OTMinMaxV128, /* SSD */ V128 | NOTMP, V128 | NOTMP, V128 | NOTMP)   \
    OL3(OTSwizzleV128, /* SSD */ V128 | TMP, V128 | NOTMP, V128 | TMP | S1) \
    OL3(OTShuffleV128, /* SSD */ V128 | TMP, V128 | TMP, V128 | TMP)        \
    OL3(OTShiftV128, /* SSD */ V128 | NOTMP, I32, V128 | TMP | S0)

// List of aliases.
#define OTOp1V128Tmp OTOp1V128
#define OTOp2V128Tmp OTOp2V128
#define OTOp2V128Rev OTOp2V128
#define OTPMinMaxV128 OTOp2V128
#define OTPopcntV128 OTOp1V128
#define OTShiftV128Tmp OTShiftV128

#endif /* SLJIT_CONFIG_ARM */

// Constructing read-only operand descriptors.

// OL = Operand List
#define OL1(name, o1) \
    o1, 0,
#define OL2(name, o1, o2) \
    o1, o2, 0,
#define OL3(name, o1, o2, o3) \
    o1, o2, o3, 0,
#define OL4(name, o1, o2, o3, o4) \
    o1, o2, o3, o4, 0,
#define OL5(name, o1, o2, o3, o4, o5) \
    o1, o2, o3, o4, o5, 0,

const uint8_t Instruction::m_operandDescriptors[] = {
    0,
    OPERAND_TYPE_LIST OPERAND_TYPE_LIST_MATH
        OPERAND_TYPE_LIST_SIMD OPERAND_TYPE_LIST_SIMD_ARCH
            OPERAND_TYPE_LIST_EXTENDED
};

#undef OL1
#undef OL2
#undef OL3
#undef OL4
#undef OL5

// Besides the list names, these macros define enum
// types with ByteN suffix. These are unused.

#define OL1(name, o1) \
    name, name##Byte1,
#define OL2(name, o1, o2) \
    name, name##Byte1, name##Byte2,
#define OL3(name, o1, o2, o3) \
    name, name##Byte1, name##Byte2, name##Byte3,
#define OL4(name, o1, o2, o3, o4) \
    name, name##Byte1, name##Byte2, name##Byte3, name##Byte4,
#define OL5(name, o1, o2, o3, o4, o5) \
    name, name##Byte1, name##Byte2, name##Byte3, name##Byte4, name##Byte5,

enum OperandTypes : uint32_t {
    OTNone,
    OPERAND_TYPE_LIST
        OPERAND_TYPE_LIST_MATH
            OPERAND_TYPE_LIST_SIMD
                OPERAND_TYPE_LIST_SIMD_ARCH
                    OPERAND_TYPE_LIST_EXTENDED
};

#undef OL1
#undef OL2
#undef OL3
#undef OL4
#undef OL5

enum ParamTypes {
    NoParam,
    ParamSrc,
    ParamDst,
    ParamSrcDst,
    ParamSrc2,
    ParamSrc2Dst,
};

static void compileFunction(JITCompiler* compiler)
{
    size_t idx = 0;
    ModuleFunction* function = compiler->moduleFunction();
    size_t endIdx = function->currentByteCodeSize();

    if (endIdx == 0) {
        // If a function has no End opcode, it is an imported function.
        return;
    }

    std::map<size_t, Label*> labels;

    // Construct labels first
    while (idx < endIdx) {
        ByteCode* byteCode = function->peekByteCode<ByteCode>(idx);
        ByteCode::Opcode opcode = byteCode->opcode();

        switch (opcode) {
        case ByteCode::JumpOpcode: {
            Jump* jump = reinterpret_cast<Jump*>(byteCode);
            labels[COMPUTE_OFFSET(idx, jump->offset())] = nullptr;
            break;
        }
        case ByteCode::JumpIfTrueOpcode:
        case ByteCode::JumpIfFalseOpcode: {
            ByteCodeOffsetValue* offsetValue = reinterpret_cast<ByteCodeOffsetValue*>(byteCode);
            labels[COMPUTE_OFFSET(idx, offsetValue->int32Value())] = nullptr;
            break;
        }
        case ByteCode::BrTableOpcode: {
            BrTable* brTable = reinterpret_cast<BrTable*>(byteCode);
            labels[COMPUTE_OFFSET(idx, brTable->defaultOffset())] = nullptr;

            int32_t* jumpOffsets = brTable->jumpOffsets();
            int32_t* jumpOffsetsEnd = jumpOffsets + brTable->tableSize();

            while (jumpOffsets < jumpOffsetsEnd) {
                labels[COMPUTE_OFFSET(idx, *jumpOffsets)] = nullptr;
                jumpOffsets++;
            }
            break;
        }
        default: {
            break;
        }
        }

        idx += byteCode->getSize();
    }

    for (auto it : function->catchInfo()) {
        labels[it.m_tryStart] = nullptr;
        labels[it.m_catchStartPosition] = nullptr;
    }

    std::map<size_t, Label*>::iterator it;

    // Values needs to be modified.
    for (it = labels.begin(); it != labels.end(); it++) {
        it->second = new Label();
    }

    compiler->initTryBlockStart();
    buildCatchInfo(compiler, function, labels);

    it = labels.begin();
    size_t nextLabelIndex = ~static_cast<size_t>(0);

    if (it != labels.end()) {
        nextLabelIndex = it->first;
    }

    idx = 0;
    while (idx < endIdx) {
        if (idx == nextLabelIndex) {
            compiler->appendLabel(it->second);

            it++;
            nextLabelIndex = ~static_cast<size_t>(0);

            if (it != labels.end()) {
                nextLabelIndex = it->first;
            }
        }

        ByteCode* byteCode = function->peekByteCode<ByteCode>(idx);
        ByteCode::Opcode opcode = byteCode->opcode();
        Instruction::Group group = Instruction::Any;
        uint8_t paramType = ParamTypes::NoParam;
        uint32_t requiredInit = OTNone;
        uint16_t info = 0;

        switch (opcode) {
        // Binary operation
        case ByteCode::I32AddOpcode:
        case ByteCode::I32SubOpcode:
        case ByteCode::I32MulOpcode:
        case ByteCode::I32RotlOpcode:
        case ByteCode::I32RotrOpcode:
        case ByteCode::I32AndOpcode:
        case ByteCode::I32OrOpcode:
        case ByteCode::I32XorOpcode: {
            group = Instruction::Binary;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIs32Bit;
            requiredInit = OTOp2I32;
            break;
        }
        case ByteCode::I32ShlOpcode:
        case ByteCode::I32ShrSOpcode:
        case ByteCode::I32ShrUOpcode: {
            group = Instruction::Binary;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIs32Bit | Instruction::kIsShift;
            requiredInit = OTOp2I32;
            break;
        }
        case ByteCode::I32DivSOpcode:
        case ByteCode::I32DivUOpcode:
        case ByteCode::I32RemSOpcode:
        case ByteCode::I32RemUOpcode: {
            group = Instruction::Binary;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIs32Bit | Instruction::kDestroysR0R1;
            requiredInit = OTOp2I32;
            break;
        }
        case ByteCode::I64AddOpcode:
        case ByteCode::I64SubOpcode:
        case ByteCode::I64AndOpcode:
        case ByteCode::I64OrOpcode:
        case ByteCode::I64XorOpcode: {
            group = Instruction::Binary;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTOp2I64;
            break;
        }
        case ByteCode::I64MulOpcode: {
            group = Instruction::Binary;
            paramType = ParamTypes::ParamSrc2Dst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kDestroysR0R1;
            requiredInit = OTMulI64;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTOp2I64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64DivSOpcode:
        case ByteCode::I64DivUOpcode:
        case ByteCode::I64RemSOpcode:
        case ByteCode::I64RemUOpcode: {
            group = Instruction::Binary;
            paramType = ParamTypes::ParamSrc2Dst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTDivRemI64;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            info = Instruction::kDestroysR0R1;
            requiredInit = OTOp2I64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64RotlOpcode:
        case ByteCode::I64RotrOpcode:
        case ByteCode::I64ShlOpcode:
        case ByteCode::I64ShrSOpcode:
        case ByteCode::I64ShrUOpcode: {
            group = Instruction::Binary;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIsShift;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            requiredInit = OTShiftI64;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTOp2I64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I32EqOpcode:
        case ByteCode::I32NeOpcode:
        case ByteCode::I32LtSOpcode:
        case ByteCode::I32LtUOpcode:
        case ByteCode::I32GtSOpcode:
        case ByteCode::I32GtUOpcode:
        case ByteCode::I32LeSOpcode:
        case ByteCode::I32LeUOpcode:
        case ByteCode::I32GeSOpcode:
        case ByteCode::I32GeUOpcode: {
            group = Instruction::Compare;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIs32Bit | Instruction::kIsMergeCompare;
            requiredInit = OTOp2I32;
            break;
        }
        case ByteCode::I64EqOpcode:
        case ByteCode::I64NeOpcode:
        case ByteCode::I64LtSOpcode:
        case ByteCode::I64LtUOpcode:
        case ByteCode::I64GtSOpcode:
        case ByteCode::I64GtUOpcode:
        case ByteCode::I64LeSOpcode:
        case ByteCode::I64LeUOpcode:
        case ByteCode::I64GeSOpcode:
        case ByteCode::I64GeUOpcode: {
            group = Instruction::Compare;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIsMergeCompare | Instruction::kFreeUnusedEarly;
            requiredInit = OTCompareI64;
            break;
        }
        case ByteCode::F32AddOpcode:
        case ByteCode::F32SubOpcode:
        case ByteCode::F32MulOpcode:
        case ByteCode::F32DivOpcode:
            requiredInit = OTOp2F32;
            FALLTHROUGH;
        case ByteCode::F64AddOpcode:
        case ByteCode::F64SubOpcode:
        case ByteCode::F64MulOpcode:
        case ByteCode::F64DivOpcode: {
            group = Instruction::BinaryFloat;
            paramType = ParamTypes::ParamSrc2Dst;
            if (requiredInit == OTNone)
                requiredInit = OTOp2F64;
            break;
        }
        case ByteCode::F32MaxOpcode:
        case ByteCode::F32MinOpcode:
            requiredInit = OTOp2F32;
            FALLTHROUGH;
        case ByteCode::F64MaxOpcode:
        case ByteCode::F64MinOpcode: {
            group = Instruction::BinaryFloat;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIsCallback;
            if (requiredInit == OTNone)
                requiredInit = OTOp2F64;
            break;
        }
        case ByteCode::F32CopysignOpcode:
        case ByteCode::F64CopysignOpcode: {
            group = Instruction::BinaryFloat;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = opcode == ByteCode::F32CopysignOpcode ? OTCopySignF32 : OTCopySignF64;
            break;
        }
        case ByteCode::F32EqOpcode:
        case ByteCode::F32NeOpcode:
        case ByteCode::F32LtOpcode:
        case ByteCode::F32LeOpcode:
        case ByteCode::F32GtOpcode:
        case ByteCode::F32GeOpcode:
            requiredInit = OTCompareF32;
            FALLTHROUGH;
        case ByteCode::F64EqOpcode:
        case ByteCode::F64NeOpcode:
        case ByteCode::F64LtOpcode:
        case ByteCode::F64LeOpcode:
        case ByteCode::F64GtOpcode:
        case ByteCode::F64GeOpcode: {
            group = Instruction::CompareFloat;
            paramType = ParamTypes::ParamSrc2Dst;
            info = Instruction::kIsMergeCompare;
            if (requiredInit == OTNone)
                requiredInit = OTCompareF64;
            break;
        }
        case ByteCode::I32PopcntOpcode: {
            group = Instruction::Unary;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIs32Bit | Instruction::kIsCallback;
            requiredInit = OTOp1I32;
            break;
        }
        case ByteCode::I32Extend8SOpcode:
        case ByteCode::I32Extend16SOpcode: {
            group = Instruction::Unary;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIs32Bit;
            requiredInit = OTOp1I32;
            break;
        }
        case ByteCode::I32ClzOpcode:
        case ByteCode::I32CtzOpcode: {
            group = Instruction::Unary;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIs32Bit;
            requiredInit = OTOp1I32;
            break;
        }
        case ByteCode::I64ClzOpcode:
        case ByteCode::I64CtzOpcode: {
            group = Instruction::Unary;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            requiredInit = OTCountZeroesI64;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTOp1I64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64PopcntOpcode: {
            group = Instruction::Unary;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIsCallback;
            requiredInit = OTOp1I64;
            break;
        }
        case ByteCode::I64Extend8SOpcode:
        case ByteCode::I64Extend16SOpcode:
        case ByteCode::I64Extend32SOpcode: {
            group = Instruction::Unary;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTOp1I64;
            break;
        }
        case ByteCode::F32CeilOpcode:
        case ByteCode::F32FloorOpcode:
        case ByteCode::F32TruncOpcode:
        case ByteCode::F32NearestOpcode:
        case ByteCode::F32SqrtOpcode:
            requiredInit = OTOp1F32;
            FALLTHROUGH;
        case ByteCode::F64CeilOpcode:
        case ByteCode::F64FloorOpcode:
        case ByteCode::F64TruncOpcode:
        case ByteCode::F64NearestOpcode:
        case ByteCode::F64SqrtOpcode: {
            group = Instruction::UnaryFloat;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIsCallback;
            if (requiredInit == OTNone)
                requiredInit = OTOp1F64;
            break;
        }
        case ByteCode::F32AbsOpcode:
        case ByteCode::F32NegOpcode:
            requiredInit = OTOp1F32;
            FALLTHROUGH;
        case ByteCode::F64AbsOpcode:
        case ByteCode::F64NegOpcode: {
            group = Instruction::UnaryFloat;
            paramType = ParamTypes::ParamSrcDst;
            if (requiredInit == OTNone)
                requiredInit = OTOp1F64;
            break;
        }
        case ByteCode::F32DemoteF64Opcode:
        case ByteCode::F64PromoteF32Opcode: {
            group = Instruction::UnaryFloat;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = opcode == ByteCode::F32DemoteF64Opcode ? OTDemoteF64 : OTPromoteF32;
            break;
        }
        case ByteCode::I32EqzOpcode: {
            group = Instruction::Compare;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIs32Bit | Instruction::kIsMergeCompare;
            requiredInit = OTOp1I32;
            break;
        }
        case ByteCode::I64EqzOpcode: {
            group = Instruction::Compare;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIsMergeCompare | Instruction::kFreeUnusedEarly;
            requiredInit = OTEqzI64;
            break;
        }
        case ByteCode::I32WrapI64Opcode: {
            group = Instruction::Convert;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kFreeUnusedEarly;
            requiredInit = OTConvertInt32FromInt64;
            break;
        }
        case ByteCode::I64ExtendI32SOpcode:
        case ByteCode::I64ExtendI32UOpcode: {
            group = Instruction::Convert;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kFreeUnusedEarly;
            requiredInit = OTConvertInt64FromInt32;
            break;
        }
        case ByteCode::I32TruncF32SOpcode:
        case ByteCode::I32TruncSatF32SOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTConvertInt32FromFloat32;
            break;
        }
        case ByteCode::I32TruncF32UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt32FromFloat32Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt32FromFloat32;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I32TruncF64SOpcode:
        case ByteCode::I32TruncSatF64SOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTConvertInt32FromFloat64;
            break;
        }
        case ByteCode::I32TruncF64UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt32FromFloat64Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt32FromFloat64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64TruncF32SOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt64FromFloat32Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt64FromFloat32;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64TruncF32UOpcode:
        case ByteCode::I64TruncSatF32UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt64FromFloat32Callback;
            break;
        }
        case ByteCode::I64TruncF64SOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt64FromFloat64Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt64FromFloat64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64TruncF64UOpcode:
        case ByteCode::I64TruncSatF64UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt64FromFloat64Callback;
            break;
        }
        case ByteCode::I32TruncSatF32UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt32FromFloat32Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt32FromFloat32;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I32TruncSatF64UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt32FromFloat64Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt32FromFloat64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64TruncSatF32SOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt64FromFloat32Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt64FromFloat32;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::I64TruncSatF64SOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
            requiredInit = OTConvertInt64FromFloat64Callback;
#else /* !SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertInt64FromFloat64;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            break;
        }
        case ByteCode::F32ConvertI32SOpcode:
        case ByteCode::F32ConvertI32UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTConvertFloat32FromInt32;
            break;
        }
        case ByteCode::F32ConvertI64SOpcode:
        case ByteCode::F32ConvertI64UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertFloat32FromInt64;
            break;
        }
        case ByteCode::F64ConvertI32SOpcode:
        case ByteCode::F64ConvertI32UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTConvertFloat64FromInt32;
            break;
        }
        case ByteCode::F64ConvertI64SOpcode:
        case ByteCode::F64ConvertI64UOpcode: {
            group = Instruction::ConvertFloat;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            info = Instruction::kIsCallback;
#endif /* SLJIT_32BIT_ARCHITECTURE */
            requiredInit = OTConvertFloat64FromInt64;
            break;
        }
        case ByteCode::SelectOpcode: {
            Instruction* instr = compiler->append(byteCode, group, opcode, 3, 1);
            Select* select = reinterpret_cast<Select*>(byteCode);
            Operand* operands = instr->operands();

            if (select->valueSize() == 16) {
                requiredInit = OTSelectV128;
            } else if (!select->isFloat()) {
                requiredInit = select->valueSize() == 4 ? OTSelectI32 : OTSelectI64;
            } else {
                requiredInit = select->valueSize() == 4 ? OTSelectF32 : OTSelectF64;
            }

            instr->setRequiredRegsDescriptor(requiredInit);

            operands[0] = STACK_OFFSET(select->src0Offset());
            operands[1] = STACK_OFFSET(select->src1Offset());
            operands[2] = STACK_OFFSET(select->condOffset());
            operands[3] = STACK_OFFSET(select->dstOffset());
            break;
        }
        case ByteCode::CallOpcode:
        case ByteCode::CallIndirectOpcode: {
            FunctionType* functionType;
            ByteCodeStackOffset* stackOffset;
            uint32_t callerCount;

            if (opcode == ByteCode::CallOpcode) {
                Call* call = reinterpret_cast<Call*>(byteCode);
                functionType = compiler->module()->function(call->index())->functionType();
                stackOffset = call->stackOffsets();
                callerCount = 0;
            } else {
                CallIndirect* callIndirect = reinterpret_cast<CallIndirect*>(byteCode);
                functionType = callIndirect->functionType();
                stackOffset = callIndirect->stackOffsets();
                callerCount = 1;
            }

            Instruction* instr = compiler->appendExtended(byteCode, Instruction::Call, opcode,
                                                          functionType->param().size() + callerCount, functionType->result().size());
            Operand* operand = instr->operands();
            instr->addInfo(Instruction::kIsCallback | Instruction::kFreeUnusedEarly);

            for (auto it : functionType->param()) {
                *operand++ = STACK_OFFSET(*stackOffset);
                stackOffset += (valueSize(it) + (sizeof(size_t) - 1)) / sizeof(size_t);
            }

            if (opcode == ByteCode::CallIndirectOpcode) {
                *operand++ = STACK_OFFSET(reinterpret_cast<CallIndirect*>(byteCode)->calleeOffset());
            }

            for (auto it : functionType->result()) {
                *operand++ = STACK_OFFSET(*stackOffset);
                stackOffset += (valueSize(it) + (sizeof(size_t) - 1)) / sizeof(size_t);
            }

            ASSERT(operand == instr->operands() + instr->paramCount() + instr->resultCount());
            break;
        }
        case ByteCode::ThrowOpcode: {
            Throw* throwTag = reinterpret_cast<Throw*>(byteCode);
            TagType* tagType = compiler->module()->tagType(throwTag->tagIndex());
            uint32_t size = compiler->module()->functionType(tagType->sigIndex())->param().size();

            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, size, 0);
            Operand* param = instr->params();
            Operand* end = param + size;
            ByteCodeStackOffset* stackOffset = throwTag->dataOffsets();

            // Does not use pointer sized offsets.
            while (param < end) {
                *param++ = STACK_OFFSET(*stackOffset++);
            }
            break;
        }
        case ByteCode::Load32Opcode: {
            group = Instruction::Load;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTLoadI32;
            break;
        }
        case ByteCode::Load64Opcode: {
            group = Instruction::Load;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTLoadI64;
            break;
        }
        case ByteCode::I32LoadOpcode:
        case ByteCode::I32Load8SOpcode:
        case ByteCode::I32Load8UOpcode:
        case ByteCode::I32Load16SOpcode:
        case ByteCode::I32Load16UOpcode:
            requiredInit = OTLoadI32;
            FALLTHROUGH;
        case ByteCode::I64LoadOpcode:
        case ByteCode::I64Load8SOpcode:
        case ByteCode::I64Load8UOpcode:
        case ByteCode::I64Load16SOpcode:
        case ByteCode::I64Load16UOpcode:
        case ByteCode::I64Load32SOpcode:
        case ByteCode::I64Load32UOpcode: {
            MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Load, opcode, 1, 1);
            instr->setRequiredRegsDescriptor(requiredInit != OTNone ? requiredInit : OTLoadI64);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(loadOperation->srcOffset());
            operands[1] = STACK_OFFSET(loadOperation->dstOffset());
            break;
        }
        case ByteCode::F32LoadOpcode:
        case ByteCode::F64LoadOpcode:
        case ByteCode::V128LoadOpcode:
        case ByteCode::V128Load8SplatOpcode:
        case ByteCode::V128Load16SplatOpcode:
        case ByteCode::V128Load32SplatOpcode:
        case ByteCode::V128Load64SplatOpcode:
        case ByteCode::V128Load8X8SOpcode:
        case ByteCode::V128Load8X8UOpcode:
        case ByteCode::V128Load16X4SOpcode:
        case ByteCode::V128Load16X4UOpcode:
        case ByteCode::V128Load32X2SOpcode:
        case ByteCode::V128Load32X2UOpcode:
        case ByteCode::V128Load32ZeroOpcode:
        case ByteCode::V128Load64ZeroOpcode: {
            MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Load, opcode, 1, 1);

            if (opcode == ByteCode::F32LoadOpcode)
                requiredInit = OTLoadF32;
            else if (opcode == ByteCode::F64LoadOpcode)
                requiredInit = OTLoadF64;
            else
                requiredInit = OTLoadV128;

            instr->setRequiredRegsDescriptor(requiredInit);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(loadOperation->srcOffset());
            operands[1] = STACK_OFFSET(loadOperation->dstOffset());
            break;
        }
        case ByteCode::V128Load8LaneOpcode:
        case ByteCode::V128Load16LaneOpcode:
        case ByteCode::V128Load32LaneOpcode:
        case ByteCode::V128Load64LaneOpcode: {
            SIMDMemoryLoad* loadOperation = reinterpret_cast<SIMDMemoryLoad*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::LoadLaneSIMD, opcode, 2, 1);
            instr->setRequiredRegsDescriptor(OTLoadLaneV128);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(loadOperation->src0Offset());
            operands[1] = STACK_OFFSET(loadOperation->src1Offset());
            operands[2] = STACK_OFFSET(loadOperation->dstOffset());
            break;
        }
        case ByteCode::Store32Opcode: {
            group = Instruction::Store;
            paramType = ParamTypes::ParamSrc2;
            requiredInit = OTStoreI32;
            break;
        }
        case ByteCode::Store64Opcode: {
            group = Instruction::Store;
            paramType = ParamTypes::ParamSrc2;
            requiredInit = OTStoreI64;
            break;
        }
        case ByteCode::I32StoreOpcode:
        case ByteCode::I32Store8Opcode:
        case ByteCode::I32Store16Opcode:
            requiredInit = OTStoreI32;
            FALLTHROUGH;
        case ByteCode::I64Store8Opcode:
        case ByteCode::I64Store16Opcode:
        case ByteCode::I64Store32Opcode:
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            if (requiredInit == OTNone)
                requiredInit = OTStoreI64Low;
            FALLTHROUGH;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        case ByteCode::I64StoreOpcode: {
            MemoryStore* storeOperation = reinterpret_cast<MemoryStore*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);
            instr->setRequiredRegsDescriptor(requiredInit != OTNone ? requiredInit : OTStoreI64);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(storeOperation->src0Offset());
            operands[1] = STACK_OFFSET(storeOperation->src1Offset());
            break;
        }
        case ByteCode::F32StoreOpcode:
        case ByteCode::F64StoreOpcode:
        case ByteCode::V128StoreOpcode: {
            MemoryStore* storeOperation = reinterpret_cast<MemoryStore*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);

            if (opcode == ByteCode::F32StoreOpcode)
                requiredInit = OTStoreF32;
            else if (opcode == ByteCode::F64StoreOpcode)
                requiredInit = OTStoreF64;
            else
                requiredInit = OTStoreV128;

            instr->setRequiredRegsDescriptor(requiredInit);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(storeOperation->src0Offset());
            operands[1] = STACK_OFFSET(storeOperation->src1Offset());
            break;
        }
        case ByteCode::V128Store8LaneOpcode:
        case ByteCode::V128Store16LaneOpcode:
        case ByteCode::V128Store32LaneOpcode:
        case ByteCode::V128Store64LaneOpcode: {
            SIMDMemoryStore* storeOperation = reinterpret_cast<SIMDMemoryStore*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);
            instr->setRequiredRegsDescriptor(OTStoreV128);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(storeOperation->src0Offset());
            operands[1] = STACK_OFFSET(storeOperation->src1Offset());
            break;
        }
        case ByteCode::I8X16ExtractLaneSOpcode:
        case ByteCode::I8X16ExtractLaneUOpcode:
        case ByteCode::I16X8ExtractLaneSOpcode:
        case ByteCode::I16X8ExtractLaneUOpcode:
        case ByteCode::I32X4ExtractLaneOpcode:
            requiredInit = OTV128ToI32;
            FALLTHROUGH;
        case ByteCode::I64X2ExtractLaneOpcode:
            if (requiredInit == OTNone) {
                requiredInit = OTExtractLaneI64;
            }
            FALLTHROUGH;
        case ByteCode::F32X4ExtractLaneOpcode:
            if (requiredInit == OTNone) {
                requiredInit = OTExtractLaneF32;
            }
            FALLTHROUGH;
        case ByteCode::F64X2ExtractLaneOpcode: {
            SIMDExtractLane* extractLane = reinterpret_cast<SIMDExtractLane*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::ExtractLaneSIMD, opcode, 1, 1);
            instr->setRequiredRegsDescriptor(requiredInit == OTNone ? OTExtractLaneF64 : requiredInit);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(extractLane->srcOffset());
            operands[1] = STACK_OFFSET(extractLane->dstOffset());
            break;
        }
        case ByteCode::I8X16ReplaceLaneOpcode:
        case ByteCode::I16X8ReplaceLaneOpcode:
        case ByteCode::I32X4ReplaceLaneOpcode:
            requiredInit = OTReplaceLaneI32;
            FALLTHROUGH;
        case ByteCode::I64X2ReplaceLaneOpcode:
            if (requiredInit == OTNone) {
                requiredInit = OTReplaceLaneI64;
            }
            FALLTHROUGH;
        case ByteCode::F32X4ReplaceLaneOpcode:
            if (requiredInit == OTNone) {
                requiredInit = OTReplaceLaneF32;
            }
            FALLTHROUGH;
        case ByteCode::F64X2ReplaceLaneOpcode: {
            SIMDReplaceLane* replaceLane = reinterpret_cast<SIMDReplaceLane*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::ReplaceLaneSIMD, opcode, 2, 1);
            instr->setRequiredRegsDescriptor(requiredInit == OTNone ? OTReplaceLaneF64 : requiredInit);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(replaceLane->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(replaceLane->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(replaceLane->dstOffset());
            break;
        }
        case ByteCode::I8X16SplatOpcode:
        case ByteCode::I16X8SplatOpcode:
        case ByteCode::I32X4SplatOpcode: {
            group = Instruction::SplatSIMD;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTSplatI32;
            break;
        }
        case ByteCode::I64X2SplatOpcode: {
            group = Instruction::SplatSIMD;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTSplatI64;
            break;
        }
        case ByteCode::F32X4SplatOpcode: {
            group = Instruction::SplatSIMD;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kFreeUnusedEarly;
            requiredInit = OTSplatF32;
            break;
        }
        case ByteCode::F64X2SplatOpcode: {
            group = Instruction::SplatSIMD;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kFreeUnusedEarly;
            requiredInit = OTSplatF64;
            break;
        }
        case ByteCode::I8X16BitmaskOpcode:
        case ByteCode::I16X8BitmaskOpcode:
        case ByteCode::I32X4BitmaskOpcode:
        case ByteCode::I64X2BitmaskOpcode: {
            group = Instruction::BitMaskSIMD;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTV128ToI32;
            break;
        }
        case ByteCode::TableInitOpcode: {
            auto tableInit = reinterpret_cast<TableInit*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 3, 0);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTCallback3Arg);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(tableInit->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(tableInit->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(tableInit->srcOffsets()[2]);
            break;
        }
        case ByteCode::TableSizeOpcode: {
            group = Instruction::Table;
            paramType = ParamTypes::ParamDst;
            requiredInit = OTPutI32;
            break;
        }
        case ByteCode::TableCopyOpcode: {
            auto tableCopy = reinterpret_cast<TableCopy*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 3, 0);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTCallback3Arg);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(tableCopy->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(tableCopy->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(tableCopy->srcOffsets()[2]);
            break;
        }
        case ByteCode::TableFillOpcode: {
            auto tableFill = reinterpret_cast<TableFill*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 3, 0);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTCallback3Arg);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(tableFill->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(tableFill->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(tableFill->srcOffsets()[2]);
            break;
        }
        case ByteCode::TableGrowOpcode: {
            auto tableGrow = reinterpret_cast<TableGrow*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 2, 1);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTTableGrow);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(tableGrow->src0Offset());
            operands[1] = STACK_OFFSET(tableGrow->src1Offset());
            operands[2] = STACK_OFFSET(tableGrow->dstOffset());
            break;
        }
        case ByteCode::TableSetOpcode: {
            auto tableSet = reinterpret_cast<TableSet*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 2, 0);
            instr->setRequiredRegsDescriptor(OTTableSet);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(tableSet->src0Offset());
            operands[1] = STACK_OFFSET(tableSet->src1Offset());
            break;
        }
        case ByteCode::TableGetOpcode: {
            auto tableGet = reinterpret_cast<TableGet*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 1, 1);
            instr->setRequiredRegsDescriptor(OTTableGet);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(tableGet->srcOffset());
            operands[1] = STACK_OFFSET(tableGet->dstOffset());
            break;
        }
        case ByteCode::MemorySizeOpcode: {
            MemorySize* memorySize = reinterpret_cast<MemorySize*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 0, 1);
            instr->setRequiredRegsDescriptor(OTPutI32);

            *instr->operands() = STACK_OFFSET(memorySize->dstOffset());
            break;
        }
        case ByteCode::MemoryInitOpcode: {
            MemoryInit* memoryInit = reinterpret_cast<MemoryInit*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 3, 0);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTCallback3Arg);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(memoryInit->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(memoryInit->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(memoryInit->srcOffsets()[2]);
            break;
        }
        case ByteCode::MemoryCopyOpcode: {
            MemoryCopy* memoryCopy = reinterpret_cast<MemoryCopy*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 3, 0);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTCallback3Arg);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(memoryCopy->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(memoryCopy->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(memoryCopy->srcOffsets()[2]);
            break;
        }
        case ByteCode::MemoryFillOpcode: {
            MemoryFill* memoryFill = reinterpret_cast<MemoryFill*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 3, 0);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTCallback3Arg);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(memoryFill->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(memoryFill->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(memoryFill->srcOffsets()[2]);
            break;
        }
        case ByteCode::MemoryGrowOpcode: {
            MemoryGrow* memoryGrow = reinterpret_cast<MemoryGrow*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 1, 1);
            instr->addInfo(Instruction::kIsCallback);
            instr->setRequiredRegsDescriptor(OTOp1I32);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(memoryGrow->srcOffset());
            operands[1] = STACK_OFFSET(memoryGrow->dstOffset());
            break;
        }
        case ByteCode::DataDropOpcode: {
            Instruction* instr = compiler->append(byteCode, group, opcode, 0, 0);
            instr->addInfo(Instruction::kIsCallback);
            break;
        }
        case ByteCode::ElemDropOpcode: {
            Instruction* instr = compiler->append(byteCode, group, opcode, 0, 0);
            instr->addInfo(Instruction::kIsCallback);
            break;
        }
        case ByteCode::UnreachableOpcode: {
            compiler->append(byteCode, group, opcode, 0, 0);
            break;
        }
        case ByteCode::JumpOpcode: {
            Jump* jump = reinterpret_cast<Jump*>(byteCode);
            compiler->appendBranch(jump, opcode, labels[COMPUTE_OFFSET(idx, jump->offset())], 0);
            break;
        }
        case ByteCode::JumpIfTrueOpcode:
        case ByteCode::JumpIfFalseOpcode: {
            ByteCodeOffsetValue* offsetValue = reinterpret_cast<ByteCodeOffsetValue*>(byteCode);
            Instruction* instr = compiler->appendBranch(byteCode, opcode, labels[COMPUTE_OFFSET(idx, offsetValue->int32Value())], STACK_OFFSET(offsetValue->stackOffset()));
            instr->setRequiredRegsDescriptor(OTGetI32);
            break;
        }
        case ByteCode::BrTableOpcode: {
            BrTable* brTable = reinterpret_cast<BrTable*>(byteCode);
            uint32_t tableSize = brTable->tableSize();
            BrTableInstruction* instr = compiler->appendBrTable(brTable, tableSize, STACK_OFFSET(brTable->condOffset()));

            instr->setRequiredRegsDescriptor(OTGetI32);

            Label** labelList = instr->targetLabels();
            int32_t* jumpOffsets = brTable->jumpOffsets();
            int32_t* jumpOffsetsEnd = jumpOffsets + tableSize;

            while (jumpOffsets < jumpOffsetsEnd) {
                Label* label = labels[COMPUTE_OFFSET(idx, *jumpOffsets)];

                label->append(instr);
                *labelList++ = label;
                jumpOffsets++;
            }

            Label* label = labels[COMPUTE_OFFSET(idx, brTable->defaultOffset())];

            label->append(instr);
            *labelList = label;

            if (compiler->options() & JITCompiler::kHasCondMov) {
                tableSize++;
            }

            compiler->increaseBranchTableSize(tableSize);
            break;
        }
        case ByteCode::Const32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, ByteCode::Const32Opcode, 0, 1);
            instr->setRequiredRegsDescriptor(OTPutI32);

            Const32* const32 = reinterpret_cast<Const32*>(byteCode);
            *instr->operands() = STACK_OFFSET(const32->dstOffset());
            break;
        }
        case ByteCode::Const64Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, ByteCode::Const64Opcode, 0, 1);
            instr->setRequiredRegsDescriptor(OTPutI64);

            Const64* const64 = reinterpret_cast<Const64*>(byteCode);
            *instr->operands() = STACK_OFFSET(const64->dstOffset());
            break;
        }
        case ByteCode::MoveI32Opcode:
        case ByteCode::MoveF32Opcode:
        case ByteCode::MoveI64Opcode:
        case ByteCode::MoveF64Opcode:
        case ByteCode::MoveV128Opcode:
        case ByteCode::I32ReinterpretF32Opcode:
        case ByteCode::I64ReinterpretF64Opcode:
        case ByteCode::F32ReinterpretI32Opcode:
        case ByteCode::F64ReinterpretI64Opcode: {
            group = Instruction::Move;
            paramType = ParamTypes::ParamSrcDst;

            switch (opcode) {
            case ByteCode::MoveI32Opcode:
                requiredInit = OTOp1I32;
                break;
            case ByteCode::MoveF32Opcode:
                requiredInit = OTMoveF32;
                break;
            case ByteCode::MoveI64Opcode:
                requiredInit = OTOp1I64;
                break;
            case ByteCode::MoveF64Opcode:
                requiredInit = OTMoveF64;
                break;
            case ByteCode::MoveV128Opcode:
                requiredInit = OTMoveV128;
                break;
            case ByteCode::I32ReinterpretF32Opcode:
                requiredInit = OTI32ReinterpretF32;
                break;
            case ByteCode::I64ReinterpretF64Opcode:
                requiredInit = OTI64ReinterpretF64;
                break;
            case ByteCode::F32ReinterpretI32Opcode:
                requiredInit = OTF32ReinterpretI32;
                break;
            default:
                requiredInit = OTF64ReinterpretI64;
                break;
            }
            break;
        }
        case ByteCode::GlobalGet32Opcode: {
            GlobalGet32* globalGet32 = reinterpret_cast<GlobalGet32*>(byteCode);
            group = Instruction::Any;
            paramType = ParamTypes::ParamDst;
            requiredInit = isFloatGlobal(globalGet32->index(), compiler->module()) ? OTGlobalGetF32 : OTGetI32;
            break;
        }
        case ByteCode::GlobalGet64Opcode: {
            GlobalGet64* globalGet64 = reinterpret_cast<GlobalGet64*>(byteCode);
            group = Instruction::Any;
            paramType = ParamTypes::ParamDst;
            requiredInit = isFloatGlobal(globalGet64->index(), compiler->module()) ? OTGlobalGetF64 : OTGlobalGetI64;
            break;
        }
        case ByteCode::GlobalGet128Opcode: {
            group = Instruction::Any;
            paramType = ParamTypes::ParamDst;
            requiredInit = OTGlobalGetV128;
            break;
        }
        case ByteCode::GlobalSet32Opcode: {
            GlobalSet32* globalSet32 = reinterpret_cast<GlobalSet32*>(byteCode);
            group = Instruction::Any;
            paramType = ParamTypes::ParamSrc;
            requiredInit = isFloatGlobal(globalSet32->index(), compiler->module()) ? OTGlobalSetF32 : OTGlobalSetI32;
            break;
        }
        case ByteCode::GlobalSet64Opcode: {
            GlobalSet64* globalSet64 = reinterpret_cast<GlobalSet64*>(byteCode);
            group = Instruction::Any;
            paramType = ParamTypes::ParamSrc;
            requiredInit = isFloatGlobal(globalSet64->index(), compiler->module()) ? OTGlobalSetF64 : OTGlobalSetI64;
            break;
        }
        case ByteCode::GlobalSet128Opcode: {
            group = Instruction::Any;
            paramType = ParamTypes::ParamSrc;
            requiredInit = OTGlobalSetV128;
            break;
        }
        case ByteCode::RefFuncOpcode: {
            group = Instruction::Any;
            paramType = ParamTypes::ParamDst;
            requiredInit = OTPutPTR;
            break;
        }
        case ByteCode::EndOpcode: {
            const ValueTypeVector& result = function->functionType()->result();

            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, result.size(), 0);
            Operand* param = instr->params();
            ByteCodeStackOffset* offsets = reinterpret_cast<End*>(byteCode)->resultOffsets();

            for (auto it : result) {
                *param++ = STACK_OFFSET(*offsets);
                offsets += (valueSize(it) + (sizeof(size_t) - 1)) / sizeof(size_t);
            }

            idx += byteCode->getSize();

            if (idx != endIdx) {
                instr->addInfo(Instruction::kEarlyReturn);
            }

            continue;
        }
        /* SIMD support. */
        case ByteCode::Const128Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, ByteCode::Const128Opcode, 0, 1);
            instr->setRequiredRegsDescriptor(OTPutV128);

            Const128* const128 = reinterpret_cast<Const128*>(byteCode);
            *instr->operands() = STACK_OFFSET(const128->dstOffset());
            break;
        }
        case ByteCode::I8X16SubOpcode:
        case ByteCode::I8X16AddOpcode:
        case ByteCode::I8X16AddSatSOpcode:
        case ByteCode::I8X16AddSatUOpcode:
        case ByteCode::I8X16SubSatSOpcode:
        case ByteCode::I8X16SubSatUOpcode:
        case ByteCode::I8X16EqOpcode:
        case ByteCode::I8X16NeOpcode:
        case ByteCode::I8X16LtSOpcode:
        case ByteCode::I8X16LtUOpcode:
        case ByteCode::I8X16LeSOpcode:
        case ByteCode::I8X16LeUOpcode:
        case ByteCode::I8X16GtSOpcode:
        case ByteCode::I8X16GtUOpcode:
        case ByteCode::I8X16GeSOpcode:
        case ByteCode::I8X16GeUOpcode:
        case ByteCode::I8X16MinSOpcode:
        case ByteCode::I8X16MinUOpcode:
        case ByteCode::I8X16MaxSOpcode:
        case ByteCode::I8X16MaxUOpcode:
        case ByteCode::I8X16AvgrUOpcode:
        case ByteCode::I8X16NarrowI16X8SOpcode:
        case ByteCode::I8X16NarrowI16X8UOpcode:
        case ByteCode::I16X8AddOpcode:
        case ByteCode::I16X8SubOpcode:
        case ByteCode::I16X8MulOpcode:
        case ByteCode::I16X8AddSatSOpcode:
        case ByteCode::I16X8AddSatUOpcode:
        case ByteCode::I16X8SubSatSOpcode:
        case ByteCode::I16X8SubSatUOpcode:
        case ByteCode::I16X8EqOpcode:
        case ByteCode::I16X8NeOpcode:
        case ByteCode::I16X8LtSOpcode:
        case ByteCode::I16X8LtUOpcode:
        case ByteCode::I16X8LeSOpcode:
        case ByteCode::I16X8LeUOpcode:
        case ByteCode::I16X8GtSOpcode:
        case ByteCode::I16X8GtUOpcode:
        case ByteCode::I16X8GeSOpcode:
        case ByteCode::I16X8GeUOpcode:
        case ByteCode::I16X8MinSOpcode:
        case ByteCode::I16X8MinUOpcode:
        case ByteCode::I16X8MaxSOpcode:
        case ByteCode::I16X8MaxUOpcode:
        case ByteCode::I16X8AvgrUOpcode:
        case ByteCode::I16X8ExtmulLowI8X16SOpcode:
        case ByteCode::I16X8ExtmulHighI8X16SOpcode:
        case ByteCode::I16X8ExtmulLowI8X16UOpcode:
        case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        case ByteCode::I16X8NarrowI32X4SOpcode:
        case ByteCode::I16X8NarrowI32X4UOpcode:
        case ByteCode::I16X8Q15mulrSatSOpcode:
        case ByteCode::I32X4AddOpcode:
        case ByteCode::I32X4SubOpcode:
        case ByteCode::I32X4MulOpcode:
        case ByteCode::I32X4EqOpcode:
        case ByteCode::I32X4NeOpcode:
        case ByteCode::I32X4LtSOpcode:
        case ByteCode::I32X4LtUOpcode:
        case ByteCode::I32X4LeSOpcode:
        case ByteCode::I32X4LeUOpcode:
        case ByteCode::I32X4GtSOpcode:
        case ByteCode::I32X4GtUOpcode:
        case ByteCode::I32X4GeSOpcode:
        case ByteCode::I32X4GeUOpcode:
        case ByteCode::I32X4MinSOpcode:
        case ByteCode::I32X4MinUOpcode:
        case ByteCode::I32X4MaxSOpcode:
        case ByteCode::I32X4MaxUOpcode:
        case ByteCode::I32X4ExtmulLowI16X8SOpcode:
        case ByteCode::I32X4ExtmulHighI16X8SOpcode:
        case ByteCode::I32X4ExtmulLowI16X8UOpcode:
        case ByteCode::I32X4ExtmulHighI16X8UOpcode:
        case ByteCode::I32X4DotI16X8SOpcode:
        case ByteCode::I64X2AddOpcode:
        case ByteCode::I64X2SubOpcode:
        case ByteCode::I64X2EqOpcode:
        case ByteCode::I64X2NeOpcode:
        case ByteCode::I64X2ExtmulLowI32X4SOpcode:
        case ByteCode::I64X2ExtmulHighI32X4SOpcode:
        case ByteCode::I64X2ExtmulLowI32X4UOpcode:
        case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        case ByteCode::F32X4EqOpcode:
        case ByteCode::F32X4NeOpcode:
        case ByteCode::F32X4LtOpcode:
        case ByteCode::F32X4LeOpcode:
        case ByteCode::F32X4AddOpcode:
        case ByteCode::F32X4DivOpcode:
        case ByteCode::F32X4MulOpcode:
        case ByteCode::F32X4SubOpcode:
        case ByteCode::F64X2AddOpcode:
        case ByteCode::F64X2DivOpcode:
        case ByteCode::F64X2MulOpcode:
        case ByteCode::F64X2SubOpcode:
        case ByteCode::F64X2EqOpcode:
        case ByteCode::F64X2NeOpcode:
        case ByteCode::F64X2LtOpcode:
        case ByteCode::F64X2LeOpcode:
        case ByteCode::V128AndOpcode:
        case ByteCode::V128OrOpcode:
        case ByteCode::V128XorOpcode: {
            group = Instruction::BinarySIMD;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTOp2V128;
            break;
        }
        case ByteCode::I8X16SwizzleOpcode: {
            group = Instruction::BinarySIMD;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTSwizzleV128;
            break;
        }
        case ByteCode::I64X2MulOpcode:
        case ByteCode::I64X2LtSOpcode:
        case ByteCode::I64X2LeSOpcode:
        case ByteCode::I64X2GtSOpcode:
        case ByteCode::I64X2GeSOpcode: {
            group = Instruction::BinarySIMD;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTOp2V128Tmp;
            break;
        }
        case ByteCode::F32X4MaxOpcode:
        case ByteCode::F32X4MinOpcode:
        case ByteCode::F64X2MaxOpcode:
        case ByteCode::F64X2MinOpcode: {
            group = Instruction::BinarySIMD;
            paramType = ParamTypes::ParamSrc2Dst;
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
            info = Instruction::kIsCallback;
#endif /* SLJIT_CONFIG_ARM_32 */
            requiredInit = OTMinMaxV128;
            break;
        }
        case ByteCode::F32X4PMaxOpcode:
        case ByteCode::F32X4PMinOpcode:
        case ByteCode::F64X2PMaxOpcode:
        case ByteCode::F64X2PMinOpcode: {
            group = Instruction::BinarySIMD;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTPMinMaxV128;
            break;
        }
        case ByteCode::F32X4GtOpcode:
        case ByteCode::F32X4GeOpcode:
        case ByteCode::F64X2GtOpcode:
        case ByteCode::F64X2GeOpcode:
        case ByteCode::V128AndnotOpcode: {
            group = Instruction::BinarySIMD;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTOp2V128Rev;
            break;
        }
        case ByteCode::I8X16NegOpcode:
        case ByteCode::I8X16AbsOpcode:
        case ByteCode::I16X8NegOpcode:
        case ByteCode::I16X8AbsOpcode:
        case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
        case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        case ByteCode::I16X8ExtendLowI8X16SOpcode:
        case ByteCode::I16X8ExtendHighI8X16SOpcode:
        case ByteCode::I16X8ExtendLowI8X16UOpcode:
        case ByteCode::I16X8ExtendHighI8X16UOpcode:
        case ByteCode::I32X4NegOpcode:
        case ByteCode::I32X4AbsOpcode:
        case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
        case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        case ByteCode::I32X4ExtendLowI16X8SOpcode:
        case ByteCode::I32X4ExtendHighI16X8SOpcode:
        case ByteCode::I32X4ExtendLowI16X8UOpcode:
        case ByteCode::I32X4ExtendHighI16X8UOpcode:
        case ByteCode::I32X4TruncSatF32X4SOpcode:
        case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
        case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        case ByteCode::I64X2NegOpcode:
        case ByteCode::I64X2AbsOpcode:
        case ByteCode::I64X2ExtendLowI32X4SOpcode:
        case ByteCode::I64X2ExtendHighI32X4SOpcode:
        case ByteCode::I64X2ExtendLowI32X4UOpcode:
        case ByteCode::I64X2ExtendHighI32X4UOpcode:
        case ByteCode::F32X4AbsOpcode:
        case ByteCode::F32X4NegOpcode:
        case ByteCode::F32X4SqrtOpcode:
        case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        case ByteCode::F32X4ConvertI32X4SOpcode:
        case ByteCode::F64X2AbsOpcode:
        case ByteCode::F64X2NegOpcode:
        case ByteCode::F64X2SqrtOpcode:
        case ByteCode::F64X2PromoteLowF32X4Opcode:
        case ByteCode::F64X2ConvertLowI32X4SOpcode:
        case ByteCode::V128NotOpcode: {
            group = Instruction::UnarySIMD;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTOp1V128;
            break;
        }
        case ByteCode::F32X4CeilOpcode:
        case ByteCode::F32X4FloorOpcode:
        case ByteCode::F32X4TruncOpcode:
        case ByteCode::F32X4NearestOpcode:
        case ByteCode::F64X2CeilOpcode:
        case ByteCode::F64X2FloorOpcode:
        case ByteCode::F64X2TruncOpcode:
        case ByteCode::F64X2NearestOpcode: {
            group = Instruction::UnarySIMD;
            paramType = ParamTypes::ParamSrcDst;
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
            info = Instruction::kIsCallback;
#endif /* SLJIT_CONFIG_ARM_32 */
            requiredInit = OTOp1V128CB;
            break;
        }
        case ByteCode::I32X4TruncSatF32X4UOpcode:
        case ByteCode::F32X4ConvertI32X4UOpcode:
        case ByteCode::F64X2ConvertLowI32X4UOpcode: {
            group = Instruction::UnarySIMD;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTOp1V128Tmp;
            break;
        }
        case ByteCode::I8X16AllTrueOpcode:
        case ByteCode::I16X8AllTrueOpcode:
        case ByteCode::I32X4AllTrueOpcode:
        case ByteCode::I64X2AllTrueOpcode:
        case ByteCode::V128AnyTrueOpcode: {
            group = Instruction::UnaryCondSIMD;
            paramType = ParamTypes::ParamSrcDst;
            info = Instruction::kIsMergeCompare;
            requiredInit = OTOpCondV128;
            break;
        }
        case ByteCode::I16X8ShlOpcode:
        case ByteCode::I16X8ShrSOpcode:
        case ByteCode::I16X8ShrUOpcode:
        case ByteCode::I32X4ShlOpcode:
        case ByteCode::I32X4ShrSOpcode:
        case ByteCode::I32X4ShrUOpcode:
        case ByteCode::I64X2ShlOpcode:
        case ByteCode::I64X2ShrUOpcode: {
            group = Instruction::ShiftSIMD;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTShiftV128;
            break;
        }
        case ByteCode::I8X16ShlOpcode:
        case ByteCode::I8X16ShrSOpcode:
        case ByteCode::I8X16ShrUOpcode:
        case ByteCode::I64X2ShrSOpcode: {
            group = Instruction::ShiftSIMD;
            paramType = ParamTypes::ParamSrc2Dst;
            requiredInit = OTShiftV128Tmp;
            break;
        }
        case ByteCode::I8X16PopcntOpcode: {
            group = Instruction::UnarySIMD;
            paramType = ParamTypes::ParamSrcDst;
            requiredInit = OTPopcntV128;
            break;
        }
        case ByteCode::V128BitSelectOpcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, 3, 1);
            instr->setRequiredRegsDescriptor(OTBitSelectV128);

            V128BitSelect* bitSelect = reinterpret_cast<V128BitSelect*>(byteCode);
            Operand* operands = instr->operands();

            operands[0] = STACK_OFFSET(bitSelect->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(bitSelect->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(bitSelect->srcOffsets()[2]);
            operands[3] = STACK_OFFSET(bitSelect->dstOffset());
            break;
        }
        case ByteCode::I8X16ShuffleOpcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, 2, 1);
            instr->setRequiredRegsDescriptor(OTShuffleV128);

            I8X16Shuffle* shuffle = reinterpret_cast<I8X16Shuffle*>(byteCode);
            Operand* operands = instr->operands();

            operands[0] = STACK_OFFSET(shuffle->srcOffsets()[0]);
            operands[1] = STACK_OFFSET(shuffle->srcOffsets()[1]);
            operands[2] = STACK_OFFSET(shuffle->dstOffset());

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
            if (compiler->context().shuffleOffset == 0) {
                compiler->context().shuffleOffset = 16 - sizeof(sljit_up);
            }
            compiler->context().shuffleOffset += 32;
#endif /* SLJIT_CONFIG_X86 */
            break;
        }
#if defined(ENABLE_EXTENDED_FEATURES)
        case ByteCode::I32AtomicLoadOpcode:
        case ByteCode::I32AtomicLoad8UOpcode:
        case ByteCode::I32AtomicLoad16UOpcode: {
            info = Instruction::kIs32Bit;
            requiredInit = OTLoadI32;
            FALLTHROUGH;
        }
        case ByteCode::I64AtomicLoadOpcode:
        case ByteCode::I64AtomicLoad8UOpcode:
        case ByteCode::I64AtomicLoad16UOpcode:
        case ByteCode::I64AtomicLoad32UOpcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Load, opcode, 1, 1);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            if (opcode == ByteCode::I64AtomicLoadOpcode) {
                info = Instruction::kIsCallback;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */
            instr->addInfo(info);
            instr->setRequiredRegsDescriptor(requiredInit != OTNone ? requiredInit : OTLoadI64);

            MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(byteCode);
            Operand* operands = instr->operands();

            operands[0] = STACK_OFFSET(loadOperation->srcOffset());
            operands[1] = STACK_OFFSET(loadOperation->dstOffset());
            break;
        }
        case ByteCode::I32AtomicStoreOpcode:
        case ByteCode::I32AtomicStore8Opcode:
        case ByteCode::I32AtomicStore16Opcode: {
            info = Instruction::kIs32Bit;
            requiredInit = OTStoreI32;
            FALLTHROUGH;
        }
        case ByteCode::I64AtomicStoreOpcode:
        case ByteCode::I64AtomicStore8Opcode:
        case ByteCode::I64AtomicStore16Opcode:
        case ByteCode::I64AtomicStore32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            if (opcode == ByteCode::I64AtomicStoreOpcode) {
                info = Instruction::kIsCallback;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */
            instr->addInfo(info);
            instr->setRequiredRegsDescriptor(requiredInit != OTNone ? requiredInit : OTStoreI64);

            MemoryStore* atomicStore = reinterpret_cast<MemoryStore*>(byteCode);
            Operand* operands = instr->operands();

            operands[0] = STACK_OFFSET(atomicStore->src0Offset());
            operands[1] = STACK_OFFSET(atomicStore->src1Offset());
            break;
        }
        case ByteCode::I32AtomicRmwAddOpcode:
        case ByteCode::I32AtomicRmw8AddUOpcode:
        case ByteCode::I32AtomicRmw16AddUOpcode:
        case ByteCode::I32AtomicRmwSubOpcode:
        case ByteCode::I32AtomicRmw8SubUOpcode:
        case ByteCode::I32AtomicRmw16SubUOpcode:
        case ByteCode::I32AtomicRmwAndOpcode:
        case ByteCode::I32AtomicRmw8AndUOpcode:
        case ByteCode::I32AtomicRmw16AndUOpcode:
        case ByteCode::I32AtomicRmwOrOpcode:
        case ByteCode::I32AtomicRmw8OrUOpcode:
        case ByteCode::I32AtomicRmw16OrUOpcode:
        case ByteCode::I32AtomicRmwXorOpcode:
        case ByteCode::I32AtomicRmw8XorUOpcode:
        case ByteCode::I32AtomicRmw16XorUOpcode:
        case ByteCode::I32AtomicRmwXchgOpcode:
        case ByteCode::I32AtomicRmw8XchgUOpcode:
        case ByteCode::I32AtomicRmw16XchgUOpcode: {
            info = Instruction::kIs32Bit;
            requiredInit = OTAtomicRmwI32;
            FALLTHROUGH;
        }
        case ByteCode::I64AtomicRmwAddOpcode:
        case ByteCode::I64AtomicRmwSubOpcode:
        case ByteCode::I64AtomicRmwAndOpcode:
        case ByteCode::I64AtomicRmwOrOpcode:
        case ByteCode::I64AtomicRmwXorOpcode:
        case ByteCode::I64AtomicRmwXchgOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            if (info == 0) {
                info = Instruction::kIsCallback;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */
            FALLTHROUGH;
        }
        case ByteCode::I64AtomicRmw8AddUOpcode:
        case ByteCode::I64AtomicRmw16AddUOpcode:
        case ByteCode::I64AtomicRmw32AddUOpcode:
        case ByteCode::I64AtomicRmw8SubUOpcode:
        case ByteCode::I64AtomicRmw16SubUOpcode:
        case ByteCode::I64AtomicRmw32SubUOpcode:
        case ByteCode::I64AtomicRmw8AndUOpcode:
        case ByteCode::I64AtomicRmw16AndUOpcode:
        case ByteCode::I64AtomicRmw32AndUOpcode:
        case ByteCode::I64AtomicRmw8OrUOpcode:
        case ByteCode::I64AtomicRmw16OrUOpcode:
        case ByteCode::I64AtomicRmw32OrUOpcode:
        case ByteCode::I64AtomicRmw8XorUOpcode:
        case ByteCode::I64AtomicRmw16XorUOpcode:
        case ByteCode::I64AtomicRmw32XorUOpcode:
        case ByteCode::I64AtomicRmw8XchgUOpcode:
        case ByteCode::I64AtomicRmw16XchgUOpcode:
        case ByteCode::I64AtomicRmw32XchgUOpcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Atomic, opcode, 2, 1);
            instr->addInfo(info);

            AtomicRmw* atomicRmw = reinterpret_cast<AtomicRmw*>(byteCode);
            Operand* operands = instr->operands();
            instr->setRequiredRegsDescriptor(requiredInit != OTNone ? requiredInit : OTAtomicRmwI64);

            operands[0] = STACK_OFFSET(atomicRmw->src0Offset());
            operands[1] = STACK_OFFSET(atomicRmw->src1Offset());
            operands[2] = STACK_OFFSET(atomicRmw->dstOffset());
            break;
        }
#endif /* ENABLE_EXTENDED_FEATURES */
        default: {
            ASSERT_NOT_REACHED();
            break;
        }
        }

        switch (paramType) {
        case ParamSrc:
        case ParamDst: {
            uint32_t resultCount = (paramType == ParamTypes::ParamDst) ? 1 : 0;
            Instruction* instr = compiler->append(byteCode, group, opcode, 1 - resultCount, resultCount);
            instr->addInfo(info);
            instr->setRequiredRegsDescriptor(requiredInit);

            ByteCodeOffsetValue* offsetValueOperation = reinterpret_cast<ByteCodeOffsetValue*>(byteCode);

            *instr->operands() = STACK_OFFSET(offsetValueOperation->stackOffset());
            break;
        }
        case ParamTypes::ParamSrcDst:
        case ParamTypes::ParamSrc2: {
            ASSERT(group != Instruction::Any);

            uint32_t resultCount = (paramType == ParamTypes::ParamSrcDst) ? 1 : 0;
            Instruction* instr = compiler->append(byteCode, group, opcode, 2 - resultCount, resultCount);
            instr->addInfo(info);
            instr->setRequiredRegsDescriptor(requiredInit);

            ByteCodeOffset2* offset2Operation = reinterpret_cast<ByteCodeOffset2*>(byteCode);

            Operand* operands = instr->operands();
            operands[0] = STACK_OFFSET(offset2Operation->stackOffset1());
            operands[1] = STACK_OFFSET(offset2Operation->stackOffset2());
            break;
        }
        case ParamTypes::ParamSrc2Dst: {
            ASSERT(group != Instruction::Any);

            Instruction* instr = compiler->append(byteCode, group, opcode, 2, 1);
            instr->addInfo(info);
            instr->setRequiredRegsDescriptor(requiredInit);

            BinaryOperation* binaryOperation = reinterpret_cast<BinaryOperation*>(byteCode);
            Operand* operands = instr->operands();

            operands[0] = STACK_OFFSET(binaryOperation->srcOffset()[0]);
            operands[1] = STACK_OFFSET(binaryOperation->srcOffset()[1]);
            operands[2] = STACK_OFFSET(binaryOperation->dstOffset());
            break;
        }
        default: {
            ASSERT(paramType == ParamTypes::NoParam);
            break;
        }
        }

        idx += byteCode->getSize();
    }

    compiler->buildVariables(STACK_OFFSET(function->requiredStackSize()));

    if (compiler->JITFlags() & JITFlagValue::disableRegAlloc) {
        compiler->allocateRegistersSimple();
    } else {
        compiler->allocateRegisters();
    }

#if !defined(NDEBUG)
    if (compiler->JITFlags() & JITFlagValue::JITVerbose) {
        compiler->dump();
    }
#endif /* !NDEBUG */

    compiler->freeVariables();

    Walrus::JITFunction* jitFunc = new JITFunction();

    function->setJITFunction(jitFunc);
    compiler->compileFunction(jitFunc, true);
}

const uint8_t* VariableList::getOperandDescriptor(Instruction* instr)
{
    uint32_t requiredInit = OTNone;

    switch (instr->opcode()) {
    case ByteCode::Load32Opcode:
        requiredInit = OTLoadF32;
        break;
    case ByteCode::Load64Opcode:
        requiredInit = OTLoadF64;
        break;
    case ByteCode::Store32Opcode:
        requiredInit = OTStoreF32;
        break;
    case ByteCode::Store64Opcode:
        requiredInit = OTStoreF64;
        break;
    default:
        break;
    }

    if (requiredInit != OTNone) {
        ASSERT((instr->paramCount() + instr->resultCount()) == 2);
        VariableList::Variable& variable = variables[*instr->getParam(1)];

        if (variable.info & Instruction::FloatOperandMarker) {
            return Instruction::getOperandDescriptorByOffset(requiredInit);
        }
    }

    return instr->getOperandDescriptor();
}

void Module::jitCompile(ModuleFunction** functions, size_t functionsLength, uint32_t JITFlags)
{
    JITCompiler compiler(this, JITFlags);

    if (functionsLength == 0) {
        size_t functionCount = m_functions.size();

        for (size_t i = 0; i < functionCount; i++) {
            if (m_functions[i]->jitFunction() == nullptr) {
                if (JITFlags & JITFlagValue::JITVerbose) {
                    printf("[[[[[[[  Function %3d  ]]]]]]]\n", static_cast<int>(i));
                }

                compiler.setModuleFunction(m_functions[i]);
                compileFunction(&compiler);
            }
        }
    } else {
        do {
            if ((*functions)->jitFunction() == nullptr) {
                if (JITFlags & JITFlagValue::JITVerbose) {
                    printf("[[[[[[[  Function %p  ]]]]]]]\n", *functions);
                }

                compiler.setModuleFunction(*functions);
                compileFunction(&compiler);
            }

            functions++;
        } while (--functionsLength != 0);
    }

    compiler.generateCode();
}

} // namespace Walrus
