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
#include "interpreter/ByteCode.h"
#include "runtime/ObjectType.h"
#include "runtime/Value.h"
#include <cstdint>

namespace Walrus {

// clang-format off
static const uint8_t g_byteCodeSize[ByteCode::OpcodeKindEnd] = {
#define DECLARE_BYTECODE_SIZE(name, ...) sizeof(name),
    FOR_EACH_BYTECODE(DECLARE_BYTECODE_SIZE)
#undef DECLARE_BYTECODE_SIZE
};
// clang-format on

ByteCode::ByteCode(ByteCode::Opcode opcode)
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    : m_opcodeInAddress(g_byteCodeTable.m_addressTable[opcode])
#else
    : m_opcode(opcode)
#endif
{
}

#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
ByteCode::Opcode ByteCode::opcode() const
{
    return static_cast<Opcode>(g_byteCodeTable.m_addressToOpcodeTable[m_opcodeInAddress]);
}
#else
ByteCode::Opcode ByteCode::opcode() const
{
    return m_opcode;
}
#endif

size_t ByteCode::getSize() const
{
    switch (this->opcode()) {
    case BrTableOpcode: {
        const BrTable *brTable = reinterpret_cast<const BrTable *>(this);
        return ByteCode::pointerAlignedSize(sizeof(BrTable) + sizeof(int32_t) * brTable->tableSize());
    }
    case CallOpcode: {
        const Call *call = reinterpret_cast<const Call *>(this);
        return ByteCode::pointerAlignedSize(sizeof(Call) + sizeof(ByteCodeStackOffset) * call->parameterOffsetsSize()
                                            + sizeof(ByteCodeStackOffset) * call->resultOffsetsSize());
    }
    case CallIndirectOpcode: {
        const CallIndirect *callIndirect = reinterpret_cast<const CallIndirect *>(this);
        return ByteCode::pointerAlignedSize(sizeof(CallIndirect) + sizeof(ByteCodeStackOffset) * callIndirect->parameterOffsetsSize()
                                            + sizeof(ByteCodeStackOffset) * callIndirect->resultOffsetsSize());
    }
    case CallRefOpcode: {
        const CallRef *callRef = reinterpret_cast<const CallRef *>(this);
        return ByteCode::pointerAlignedSize(sizeof(CallRef) + sizeof(ByteCodeStackOffset) * callRef->parameterOffsetsSize()
                                            + sizeof(ByteCodeStackOffset) * callRef->resultOffsetsSize());
    }
    case EndOpcode: {
        const End *end = reinterpret_cast<const End *>(this);
        return ByteCode::pointerAlignedSize(sizeof(End) + sizeof(ByteCodeStackOffset) * end->offsetsSize());
    }
    case ThrowOpcode: {
        const Throw *throwCode = reinterpret_cast<const Throw *>(this);
        return ByteCode::pointerAlignedSize(sizeof(Throw) + sizeof(ByteCodeStackOffset) * throwCode->offsetsSize());
    }
    case ArrayNewFixedOpcode: {
        const ArrayNewFixed *arrayNewFixedCode = reinterpret_cast<const ArrayNewFixed *>(this);
        return ByteCode::pointerAlignedSize(sizeof(ArrayNewFixed) + sizeof(ByteCodeStackOffset) * arrayNewFixedCode->offsetsSize());
    }
    case StructNewOpcode: {
        const StructNew *structNewCode = reinterpret_cast<const StructNew *>(this);
        return ByteCode::pointerAlignedSize(sizeof(StructNew) + sizeof(ByteCodeStackOffset) * structNewCode->offsetsSize());
    }
    default: {
        return g_byteCodeSize[this->opcode()];
    }
    }
    RELEASE_ASSERT_NOT_REACHED();
}

std::vector<Walrus::ByteCodeStackOffset> ByteCode::getByteCodeStackOffsets(FunctionType *funcType) const
{
    std::vector<Walrus::ByteCodeStackOffset> offsets;

    switch (this->opcode()) {
#define GENERATE_BINARY_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_BINARY_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_BINARY_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_BINARY_SHIFT_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_BINARY_OTHER(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OTHER(GENERATE_BINARY_CODE_CASE)
#undef GENERATE_BINARY_CODE_CASE
    case Walrus::ByteCode::MemoryCopyOpcode: {
        ByteCodeOffset3 *binOp = reinterpret_cast<Walrus::ByteCodeOffset3 *>(const_cast<ByteCode *>(this));
        offsets.push_back(binOp->stackOffset1());
        offsets.push_back(binOp->stackOffset2());
        offsets.push_back(binOp->stackOffset3());
        break;
    }
#define GENERATE_UNARY_CODE_CASE(name, ...) case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_UNARY_OP(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_UNARY_OP_2(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_UNARY_OP(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_UNARY_CONVERT_OP(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_UNARY_OTHER(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_UNARY_OTHER(GENERATE_UNARY_CODE_CASE)
#undef GENERATE_UNARY_CODE_CASE
    case Walrus::ByteCode::I64ReinterpretF64Opcode:
    case Walrus::ByteCode::F32ReinterpretI32Opcode:
    case Walrus::ByteCode::F64ReinterpretI64Opcode:
    case Walrus::ByteCode::I32ReinterpretF32Opcode:
    case Walrus::ByteCode::MoveI32Opcode:
    case Walrus::ByteCode::MoveF32Opcode:
    case Walrus::ByteCode::MoveI64Opcode:
    case Walrus::ByteCode::MoveF64Opcode:
    case Walrus::ByteCode::MoveV128Opcode:
    case Walrus::ByteCode::MemoryGrowOpcode:
    case Walrus::ByteCode::RefI31Opcode:
    case Walrus::ByteCode::Store32Opcode:
    case Walrus::ByteCode::Store64Opcode:
    case Walrus::ByteCode::Load32Opcode:
    case Walrus::ByteCode::Load64Opcode: {
        ByteCodeOffset2 *unOp = reinterpret_cast<Walrus::ByteCodeOffset2 *>(const_cast<ByteCode *>(this));
        offsets.push_back(unOp->stackOffset1());
        offsets.push_back(unOp->stackOffset2());
        break;
    }
#define GENERATE_TERNARY_CODE_CASE(name, ...) case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OP(GENERATE_TERNARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OTHER(GENERATE_TERNARY_CODE_CASE)
#undef GENERATE_TERNARY_CODE_CASE
        {
            ByteCodeOffset4 *ternary = reinterpret_cast<Walrus::ByteCodeOffset4 *>(const_cast<ByteCode *>(this));
            offsets.push_back(ternary->src0Offset());
            offsets.push_back(ternary->src1Offset());
            offsets.push_back(ternary->src2Offset());
            offsets.push_back(ternary->dstOffset());
            break;
        }
#define GENERATE_MEMORY_LOAD_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_LOAD_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_ETC_MEMIDX_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
        FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
#undef GENERATE_MEMORY_LOAD_CODE_CASE
    case Walrus::ByteCode::V128Load32ZeroOpcode:
    case Walrus::ByteCode::V128Load64ZeroOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::MemoryLoad *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::MemoryLoad *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
#define GENERATE_SIMD_MEMORY_LOAD_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(GENERATE_SIMD_MEMORY_LOAD_CASE)
#undef GENERATE_SIMD_MEMORY_LOAD_CASE
        {
            offsets.push_back(reinterpret_cast<Walrus::SIMDMemoryLoad *>(const_cast<ByteCode *>(this))->index());
            offsets.push_back(reinterpret_cast<Walrus::SIMDMemoryLoad *>(const_cast<ByteCode *>(this))->src0Offset());
            offsets.push_back(reinterpret_cast<Walrus::SIMDMemoryLoad *>(const_cast<ByteCode *>(this))->src1Offset());
            offsets.push_back(reinterpret_cast<Walrus::SIMDMemoryLoad *>(const_cast<ByteCode *>(this))->dstOffset());
            break;
        }
#define GENERATE_MEMORY_STORE_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_STORE_OP(GENERATE_MEMORY_STORE_CODE_CASE)
        FOR_EACH_BYTECODE_ATOMIC_STORE_OP(GENERATE_MEMORY_STORE_CODE_CASE)
#undef GENERATE_MEMORY_STORE_CODE_CASE
        {
            offsets.push_back(reinterpret_cast<Walrus::MemoryStore *>(const_cast<ByteCode *>(this))->src0Offset());
            offsets.push_back(reinterpret_cast<Walrus::MemoryStore *>(const_cast<ByteCode *>(this))->src1Offset());
            break;
        }
#define GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_STORE_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
        FOR_EACH_BYTECODE_LOAD_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
        FOR_EACH_BYTECODE_ATOMIC_STORE_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
        FOR_EACH_BYTECODE_ATOMIC_LOAD_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
#undef GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE
        {
            offsets.push_back(reinterpret_cast<Walrus::ByteCodeOffset2ValueMemIdx *>(const_cast<ByteCode *>(this))->stackOffset1());
            offsets.push_back(reinterpret_cast<Walrus::ByteCodeOffset2ValueMemIdx *>(const_cast<ByteCode *>(this))->stackOffset2());
            break;
        }
#define GENERATE_SIMD_MEMORY_STORE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(GENERATE_SIMD_MEMORY_STORE_CASE)
#undef GENERATE_SIMD_MEMORY_STORE_CASE
        {
            offsets.push_back(reinterpret_cast<Walrus::SIMDMemoryStore *>(const_cast<ByteCode *>(this))->index());
            offsets.push_back(reinterpret_cast<Walrus::SIMDMemoryStore *>(const_cast<ByteCode *>(this))->src0Offset());
            offsets.push_back(reinterpret_cast<Walrus::SIMDMemoryStore *>(const_cast<ByteCode *>(this))->src1Offset());
            break;
        }
#define GENERATE_SIMD_EXTRACT_LANE_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_EXTRACT_LANE_OP(GENERATE_SIMD_EXTRACT_LANE_CODE_CASE)
#undef GENERATE_SIMD_EXTRACT_LANE_CODE_CASE
        {
            // offsets.push_back(reinterpret_cast<Walrus::SIMDExtractLane *>(const_cast<ByteCode *>(this))->index());
            offsets.push_back(reinterpret_cast<Walrus::SIMDExtractLane *>(const_cast<ByteCode *>(this))->srcOffset());
            offsets.push_back(reinterpret_cast<Walrus::SIMDExtractLane *>(const_cast<ByteCode *>(this))->dstOffset());
            break;
        }
#define GENERATE_SIMD_REPLACE_LANE_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_REPLACE_LANE_OP(GENERATE_SIMD_REPLACE_LANE_CODE_CASE)
#undef GENERATE_SIMD_REPLACE_LANE_CODE_CASE
        {
            offsets.push_back(reinterpret_cast<Walrus::SIMDReplaceLane *>(const_cast<ByteCode *>(this))->index());
            offsets.push_back(reinterpret_cast<Walrus::SIMDReplaceLane *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
            offsets.push_back(reinterpret_cast<Walrus::SIMDReplaceLane *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
            offsets.push_back(reinterpret_cast<Walrus::SIMDReplaceLane *>(const_cast<ByteCode *>(this))->dstOffset());
            break;
        }
    // Special cases that require manual handling. This list needs to be extended if new byte codes are introduced.
    case Walrus::ByteCode::GlobalGet32Opcode:
    case Walrus::ByteCode::GlobalGet64Opcode:
    case Walrus::ByteCode::GlobalGet128Opcode: {
        offsets.push_back(reinterpret_cast<Walrus::GlobalGet32 *>(const_cast<Walrus::ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::GlobalSet32Opcode:
    case Walrus::ByteCode::GlobalSet64Opcode:
    case Walrus::ByteCode::GlobalSet128Opcode: {
        offsets.push_back(reinterpret_cast<Walrus::GlobalSet32 *>(const_cast<Walrus::ByteCode *>(this))->srcOffset());
        break;
    }
    case Walrus::ByteCode::SelectOpcode: {
        Walrus::Select *sel = reinterpret_cast<Walrus::Select *>(const_cast<ByteCode *>(this));
        offsets.push_back(sel->src0Offset());
        offsets.push_back(sel->src1Offset());
        offsets.push_back(sel->condOffset());
        offsets.push_back(sel->dstOffset());
        break;
    }
    case Walrus::ByteCode::Const32Opcode:
    case Walrus::ByteCode::Const64Opcode:
    case Walrus::ByteCode::Const128Opcode: {
        offsets.push_back(reinterpret_cast<Walrus::Const32 *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::MemorySizeOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::MemorySize *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::MemoryInitOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::MemoryInit *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
        offsets.push_back(reinterpret_cast<Walrus::MemoryInit *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
        offsets.push_back(reinterpret_cast<Walrus::MemoryInit *>(const_cast<ByteCode *>(this))->srcOffsets()[2]);
        break;
    }
    case Walrus::ByteCode::MemoryFillOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::MemoryFill *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
        offsets.push_back(reinterpret_cast<Walrus::MemoryFill *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
        offsets.push_back(reinterpret_cast<Walrus::MemoryFill *>(const_cast<ByteCode *>(this))->srcOffsets()[2]);
        break;
    }
    case Walrus::ByteCode::TableSizeOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::TableSize *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::TableGrowOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::TableGrow *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::TableGrow *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::TableGrow *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::TableGetOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::TableGet *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::TableGet *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::TableSetOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::TableSet *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::TableSet *>(const_cast<ByteCode *>(this))->src1Offset());
        break;
    }
    case Walrus::ByteCode::TableInitOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::TableInit *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
        offsets.push_back(reinterpret_cast<Walrus::TableInit *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
        offsets.push_back(reinterpret_cast<Walrus::TableInit *>(const_cast<ByteCode *>(this))->srcOffsets()[2]);
        break;
    }
    case Walrus::ByteCode::TableCopyOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::TableCopy *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
        offsets.push_back(reinterpret_cast<Walrus::TableCopy *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
        offsets.push_back(reinterpret_cast<Walrus::TableCopy *>(const_cast<ByteCode *>(this))->srcOffsets()[2]);
        break;
    }
    case Walrus::ByteCode::TableFillOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::TableFill *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
        offsets.push_back(reinterpret_cast<Walrus::TableFill *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
        offsets.push_back(reinterpret_cast<Walrus::TableFill *>(const_cast<ByteCode *>(this))->srcOffsets()[2]);
        break;
    }
    case Walrus::ByteCode::I8X16ShuffleOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::I8X16Shuffle *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
        offsets.push_back(reinterpret_cast<Walrus::I8X16Shuffle *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
        offsets.push_back(reinterpret_cast<Walrus::I8X16Shuffle *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::V128BitSelectOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::V128BitSelect *>(const_cast<ByteCode *>(this))->srcOffsets()[0]);
        offsets.push_back(reinterpret_cast<Walrus::V128BitSelect *>(const_cast<ByteCode *>(this))->srcOffsets()[1]);
        offsets.push_back(reinterpret_cast<Walrus::V128BitSelect *>(const_cast<ByteCode *>(this))->srcOffsets()[2]);
        offsets.push_back(reinterpret_cast<Walrus::V128BitSelect *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::JumpIfTrueOpcode:
    case Walrus::ByteCode::JumpIfFalseOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::JumpIfTrue *>(const_cast<ByteCode *>(this))->srcOffset());
        break;
    }
#define GENERATE_ATOMIC_RMW_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_ATOMIC_RMW_OP(GENERATE_ATOMIC_RMW_CODE_CASE)
#undef GENERATE_ATOMIC_RMW_CODE_CASE
        {
            offsets.push_back(reinterpret_cast<Walrus::AtomicRmw *>(const_cast<ByteCode *>(this))->src0Offset());
            offsets.push_back(reinterpret_cast<Walrus::AtomicRmw *>(const_cast<ByteCode *>(this))->src1Offset());
            offsets.push_back(reinterpret_cast<Walrus::AtomicRmw *>(const_cast<ByteCode *>(this))->dstOffset());
            break;
        }
#define GENERATE_ATOMIC_RMW_CMPXCHG_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(GENERATE_ATOMIC_RMW_CMPXCHG_CODE_CASE)
#undef GENERATE_ATOMIC_RMW_CODE_CASE
        {
            offsets.push_back(reinterpret_cast<Walrus::AtomicRmwCmpxchg *>(const_cast<ByteCode *>(this))->src0Offset());
            offsets.push_back(reinterpret_cast<Walrus::AtomicRmwCmpxchg *>(const_cast<ByteCode *>(this))->src1Offset());
            offsets.push_back(reinterpret_cast<Walrus::AtomicRmwCmpxchg *>(const_cast<ByteCode *>(this))->src2Offset());
            offsets.push_back(reinterpret_cast<Walrus::AtomicRmwCmpxchg *>(const_cast<ByteCode *>(this))->dstOffset());
            break;
        }
    case Walrus::ByteCode::MemoryAtomicNotifyOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::MemoryAtomicNotify *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::MemoryAtomicNotify *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::MemoryAtomicNotify *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::MemoryAtomicNotifyMemIdxOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::MemoryAtomicNotifyMemIdx *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::MemoryAtomicNotifyMemIdx *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::MemoryAtomicNotifyMemIdx *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::MemoryAtomicWait32Opcode:
    case Walrus::ByteCode::MemoryAtomicWait32MemIdxOpcode:
    case Walrus::ByteCode::MemoryAtomicWait64Opcode:
    case Walrus::ByteCode::MemoryAtomicWait64MemIdxOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ByteCodeOffset4Value *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ByteCodeOffset4Value *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ByteCodeOffset4Value *>(const_cast<ByteCode *>(this))->src2Offset());
        offsets.push_back(reinterpret_cast<Walrus::ByteCodeOffset4Value *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    // WebAssembly3 opcodes
    case Walrus::ByteCode::JumpIfNullOpcode:
    case Walrus::ByteCode::JumpIfNonNullOpcode:
    case Walrus::ByteCode::JumpIfCastGenericOpcode:
    case Walrus::ByteCode::JumpIfCastDefinedOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ByteCodeOffsetValue *>(const_cast<ByteCode *>(this))->stackOffset());
        break;
    }
    case Walrus::ByteCode::RefFuncOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::RefFunc *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::RefAsNonNullOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::RefAsNonNull *>(const_cast<ByteCode *>(this))->stackOffset());
        break;
    }
    case Walrus::ByteCode::RefCastGenericOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::RefCastGeneric *>(const_cast<ByteCode *>(this))->srcOffset());
        break;
    }
    case Walrus::ByteCode::RefCastDefinedOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::RefCastDefined *>(const_cast<ByteCode *>(this))->srcOffset());
        break;
    }
    case Walrus::ByteCode::RefTestGenericOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::RefTestGeneric *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::RefTestGeneric *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::RefTestDefinedOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::RefTestDefined *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::RefTestDefined *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::I31GetSOpcode:
    case Walrus::ByteCode::I31GetUOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::I31Get *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::I31Get *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::ArrayNewOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayNew *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayNew *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayNew *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::ArrayLenOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayLen *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayLen *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }

    case Walrus::ByteCode::ArrayNewDefaultOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewDefault *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewDefault *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::ArrayNewFixedOpcode: {
        Walrus::ArrayNewFixed *fixedArr = reinterpret_cast<Walrus::ArrayNewFixed *>(const_cast<ByteCode *>(this));
        for (uint32_t i = 0; i < fixedArr->length(); i++) {
            offsets.push_back(fixedArr->dataOffsets()[i]);
        }
        offsets.push_back(fixedArr->dstOffset());
        break;
    }
    case Walrus::ByteCode::ArrayNewDataOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewData *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewData *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewData *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::ArrayNewElemOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewElem *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewElem *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayNewElem *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::ArrayFillOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayFill *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayFill *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayFill *>(const_cast<ByteCode *>(this))->src2Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayFill *>(const_cast<ByteCode *>(this))->src3Offset());
        break;
    }
    case Walrus::ByteCode::ArrayCopyOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayCopy *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayCopy *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayCopy *>(const_cast<ByteCode *>(this))->src2Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayCopy *>(const_cast<ByteCode *>(this))->src3Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayCopy *>(const_cast<ByteCode *>(this))->src4Offset());
        break;
    }
    case Walrus::ByteCode::ArrayInitDataOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitData *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitData *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitData *>(const_cast<ByteCode *>(this))->src2Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitData *>(const_cast<ByteCode *>(this))->src3Offset());
        break;
    }
    case Walrus::ByteCode::ArrayInitElemOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitElem *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitElem *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitElem *>(const_cast<ByteCode *>(this))->src2Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayInitElem *>(const_cast<ByteCode *>(this))->src3Offset());
        break;
    }
    case Walrus::ByteCode::ArrayGetOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArrayGet *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayGet *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArrayGet *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::ArraySetOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::ArraySet *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArraySet *>(const_cast<ByteCode *>(this))->src1Offset());
        offsets.push_back(reinterpret_cast<Walrus::ArraySet *>(const_cast<ByteCode *>(this))->src2Offset());
        break;
    }
    case Walrus::ByteCode::StructNewOpcode: {
        Walrus::StructNew *structNew = reinterpret_cast<Walrus::StructNew *>(const_cast<ByteCode *>(this));
        for (uint32_t i = 0; i < structNew->offsetsSize(); i++) {
            offsets.push_back(structNew->dataOffsets()[i]);
        }
        offsets.push_back(structNew->dstOffset());
        break;
    }
    case Walrus::ByteCode::StructNewDefaultOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::StructNewDefault *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::StructGetOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::StructGet *>(const_cast<ByteCode *>(this))->srcOffset());
        offsets.push_back(reinterpret_cast<Walrus::StructGet *>(const_cast<ByteCode *>(this))->dstOffset());
        break;
    }
    case Walrus::ByteCode::StructSetOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::StructSet *>(const_cast<ByteCode *>(this))->src0Offset());
        offsets.push_back(reinterpret_cast<Walrus::StructSet *>(const_cast<ByteCode *>(this))->src1Offset());
        break;
    }
    case Walrus::ByteCode::EndOpcode: {
        Walrus::End *end = reinterpret_cast<Walrus::End *>(const_cast<ByteCode *>(this));
        for (uint32_t i = 0; i < end->offsetsSize(); i++) {
            offsets.push_back(end->resultOffsets()[i]);
        }
        break;
    }
    case Walrus::ByteCode::CallOpcode: {
        Walrus::Call *call = reinterpret_cast<Walrus::Call *>(const_cast<ByteCode *>(this));
        for (uint32_t i = 0; i < call->parameterOffsetsSize() + call->resultOffsetsSize(); i++) {
            offsets.push_back(call->stackOffsets()[i]);
        }
        break;
    }
    case Walrus::ByteCode::CallIndirectOpcode: {
        Walrus::CallIndirect *call = reinterpret_cast<Walrus::CallIndirect *>(const_cast<ByteCode *>(this));
        offsets.push_back(call->calleeOffset());

        size_t offsetCounter = 0;
#if defined(WALRUS_64)
        for (uint32_t i = 0; i < call->functionType()->param().size(); i++) {
            offsets.push_back(call->stackOffsets()[offsetCounter]);

            if (call->functionType()->param().types()[i] == Walrus::Value::Type::V128) {
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
            }
            offsetCounter++;
        }

        for (uint32_t i = 0; i < call->functionType()->result().size(); i++) {
            offsets.push_back(call->stackOffsets()[offsetCounter]);
            offsetCounter++;

            if (call->functionType()->result().types()[i] == Walrus::Value::Type::V128) {
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
            }
        }
#elif defined(WALRUS_32)
        for (uint32_t i = 0; i < call->functionType()->param().size(); i++) {
            offsets.push_back(call->stackOffsets()[offsetCounter]);
            offsetCounter++;

            switch (call->functionType()->param().types()[i]) {
            case Walrus::Value::Type::I64:
            case Walrus::Value::Type::F64: {
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                break;
            }
            case Walrus::Value::Type::V128: {
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                break;
            }
            default: {
                break;
            }
            }
        }
        for (uint32_t i = 0; i < call->functionType()->result().size(); i++) {
            offsets.push_back(call->stackOffsets()[offsetCounter]);
            offsetCounter++;

            switch (call->functionType()->result()[i]) {
            case Walrus::Value::Type::I64:
            case Walrus::Value::Type::F64: {
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                break;
            }
            case Walrus::Value::Type::V128: {
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                offsets.push_back(call->stackOffsets()[offsetCounter]);
                offsetCounter++;
                break;
            }
            default: {
                break;
            }
            }
        }
#endif
        break;
    }
    case Walrus::ByteCode::CallRefOpcode: {
        Walrus::CallRef *callRef = reinterpret_cast<Walrus::CallRef *>(const_cast<Walrus::ByteCode *>(this));
        offsets.push_back(callRef->calleeOffset());
        for (uint32_t i = 0; i < callRef->parameterOffsetsSize() + callRef->resultOffsetsSize(); i++) {
            offsets.push_back(callRef->stackOffsets()[i]);
        }
        break;
    }
    case Walrus::ByteCode::BrTableOpcode: {
        offsets.push_back(reinterpret_cast<Walrus::BrTable *>(const_cast<ByteCode *>(this))->condOffset());
        break;
    }
    case Walrus::ByteCode::ThrowOpcode: {
        Walrus::Throw *thr = reinterpret_cast<Walrus::Throw *>(const_cast<ByteCode *>(this));
        for (uint32_t i = 0; i < thr->offsetsSize(); i++) {
            offsets.push_back(thr->dataOffsets()[i]);
        }
        break;
    }
    case Walrus::ByteCode::ElemDropOpcode:
    case Walrus::ByteCode::DataDropOpcode:
    case Walrus::ByteCode::JumpOpcode:
    case Walrus::ByteCode::NopOpcode:
    case Walrus::ByteCode::UnreachableOpcode:
    case Walrus::ByteCode::AtomicFenceOpcode: {
        break;
    }
    default: {
        RELEASE_ASSERT_NOT_REACHED();
    }
    }

    return offsets;
}

void ByteCode::setByteCodeOffset(size_t index, Walrus::ByteCodeStackOffset offset, Walrus::ByteCodeStackOffset original)
{
    switch (this->opcode()) {
#define GENERATE_BINARY_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_BINARY_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_BINARY_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_BINARY_SHIFT_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_BINARY_OTHER(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OP(GENERATE_BINARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OTHER(GENERATE_BINARY_CODE_CASE)
#undef GENERATE_BINARY_CODE_CASE
    case Walrus::ByteCode::MemoryCopyOpcode:
    case Walrus::ByteCode::MemoryFillOpcode: {
        ByteCodeOffset3 *code = reinterpret_cast<ByteCodeOffset3 *>(this);
        code->setStackOffset(index, offset);
        break;
    }
#define GENERATE_UNARY_CODE_CASE(name, ...) case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_UNARY_OP(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_UNARY_OP_2(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_UNARY_OP(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_UNARY_CONVERT_OP(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_UNARY_OTHER(GENERATE_UNARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_UNARY_OTHER(GENERATE_UNARY_CODE_CASE)
#undef GENERATE_UNARY_CODE_CASE
    case Walrus::ByteCode::I64ReinterpretF64Opcode:
    case Walrus::ByteCode::F32ReinterpretI32Opcode:
    case Walrus::ByteCode::F64ReinterpretI64Opcode:
    case Walrus::ByteCode::I32ReinterpretF32Opcode:
    case Walrus::ByteCode::MoveI32Opcode:
    case Walrus::ByteCode::MoveF32Opcode:
    case Walrus::ByteCode::MoveI64Opcode:
    case Walrus::ByteCode::MoveF64Opcode:
    case Walrus::ByteCode::MoveV128Opcode:
    case Walrus::ByteCode::MemoryGrowOpcode:
    case Walrus::ByteCode::Load32Opcode:
    case Walrus::ByteCode::Load64Opcode:
    case Walrus::ByteCode::Store32Opcode:
    case Walrus::ByteCode::Store64Opcode:
    case Walrus::ByteCode::RefI31Opcode: {
        ByteCodeOffset2 *code = reinterpret_cast<ByteCodeOffset2 *>(this);
        if (index == 0) {
            code->setStackOffset1(offset);
        } else {
            code->setStackOffset2(offset);
        }

        break;
    }
#define GENERATE_TERNARY_CODE_CASE(name, ...) case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OP(GENERATE_TERNARY_CODE_CASE)
        FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OTHER(GENERATE_TERNARY_CODE_CASE)
#undef GENERATE_TERNARY_CODE_CASE
        {
            ByteCodeOffset4 *ternary = reinterpret_cast<Walrus::ByteCodeOffset4 *>(const_cast<ByteCode *>(this));
            ternary->setStackOffset(index, offset);
            break;
        }
#define GENERATE_MEMORY_LOAD_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_LOAD_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
        FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
        FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(GENERATE_MEMORY_LOAD_CODE_CASE)
#undef GENERATE_MEMORY_LOAD_CODE_CASE
#define GENERATE_MEMORY_STORE_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_STORE_OP(GENERATE_MEMORY_STORE_CODE_CASE)
        FOR_EACH_BYTECODE_ATOMIC_STORE_OP(GENERATE_MEMORY_STORE_CODE_CASE)
#undef GENERATE_MEMORY_STORE_CODE_CASE
    case Walrus::ByteCode::TableGetOpcode:
    case Walrus::ByteCode::TableSetOpcode:
    case Walrus::ByteCode::I31GetSOpcode:
    case Walrus::ByteCode::I31GetUOpcode:
    case Walrus::ByteCode::ArrayLenOpcode:
    case Walrus::ByteCode::V128Load32ZeroOpcode:
    case Walrus::ByteCode::V128Load64ZeroOpcode: {
        ByteCodeOffset2Value *code = reinterpret_cast<ByteCodeOffset2Value *>(this);
        if (index == 0) {
            code->setStackOffset1(offset);
        } else {
            code->setStackOffset2(offset);
        }
        break;
    }
#define GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_STORE_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
        FOR_EACH_BYTECODE_LOAD_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
        FOR_EACH_BYTECODE_ATOMIC_STORE_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
        FOR_EACH_BYTECODE_ATOMIC_LOAD_MEMIDX_OP(GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE)
#undef GENERATE_BYTECODE_OFFSET2VALUE_MEMIDX_CASE
        {
            ByteCodeOffset2ValueMemIdx *code = reinterpret_cast<ByteCodeOffset2ValueMemIdx *>(this);
            if (index == 0) {
                code->setStackOffset1(offset);
            } else {
                code->setStackOffset2(offset);
            }
            break;
        }

#define GENERATE_SIMD_MEMORY_LOAD_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(GENERATE_SIMD_MEMORY_LOAD_CASE)
#undef GENERATE_SIMD_MEMORY_LOAD_CASE
        {
            SIMDMemoryLoad *memoryLoad = reinterpret_cast<SIMDMemoryLoad *>(const_cast<ByteCode *>(this));
            switch (index) {
            case 0: {
                memoryLoad->setIndex(offset);
                break;
            }
            case 1: {
                memoryLoad->setSrc0Offset(offset);
                break;
            }
            case 2: {
                memoryLoad->setSrc1Offset(offset);
                break;
            }
            case 3: {
                memoryLoad->setDstOffset(offset);
                break;
            }
            }
            break;
        }
#define GENERATE_SIMD_MEMORY_LOAD_LANE_MEMIDX_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_LOAD_LANE_MEMIDX_OP(GENERATE_SIMD_MEMORY_LOAD_LANE_MEMIDX_CASE)
#undef GENERATE_SIMD_MEMORY_LOAD_LANE_MEMIDX_CASE
        {
            SIMDMemoryLoadMemIdx *memoryLoad = reinterpret_cast<SIMDMemoryLoadMemIdx *>(const_cast<ByteCode *>(this));
            switch (index) {
            case 0: {
                memoryLoad->setIndex(offset);
                break;
            }
            case 1: {
                memoryLoad->setSrc0Offset(offset);
                break;
            }
            case 2: {
                memoryLoad->setSrc1Offset(offset);
                break;
            }
            case 3: {
                memoryLoad->setDstOffset(offset);
                break;
            }
            }
            break;
        }
#define GENERATE_SIMD_MEMORY_STORE_LANE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(GENERATE_SIMD_MEMORY_STORE_LANE_CASE)
#undef GENERATE_SIMD_MEMORY_STORE_LANE_CASE
        {
            SIMDMemoryStore *memoryStore = reinterpret_cast<SIMDMemoryStore *>(const_cast<ByteCode *>(this));
            if (index == 0) {
                memoryStore->setIndex(offset);
            } else if (index == 1) {
                memoryStore->setSrc0Offset(offset);
            } else {
                memoryStore->setSrc1Offset(offset);
            }
            break;
        }
#define GENERATE_SIMD_MEMORY_STORE_LANE_MEMIDX_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_STORE_LANE_MEMIDX_OP(GENERATE_SIMD_MEMORY_STORE_LANE_MEMIDX_CASE)
#undef GENERATE_SIMD_MEMORY_STORE_LANE_MEMIDX_CASE
        {
            SIMDMemoryStoreMemIdx *memoryStore = reinterpret_cast<SIMDMemoryStoreMemIdx *>(const_cast<ByteCode *>(this));
            if (index == 0) {
                memoryStore->setIndex(offset);
            } else if (index == 1) {
                memoryStore->setSrc0Offset(offset);
            } else {
                memoryStore->setSrc1Offset(offset);
            }
            break;
        }
#define GENERATE_SIMD_EXTRACT_LANE_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_EXTRACT_LANE_OP(GENERATE_SIMD_EXTRACT_LANE_CODE_CASE)
#undef GENERATE_SIMD_EXTRACT_LANE_CODE_CASE
        {
            SIMDExtractLane *extractLane = reinterpret_cast<SIMDExtractLane *>(const_cast<ByteCode *>(this));
            // if (index == 0) {
            // extractLane->setIndex(offset);
            // } else
            if (index == 0) {
                extractLane->setSrcOffset(offset);
            } else {
                extractLane->setDstOffset(offset);
            }
            break;
        }
#define GENERATE_SIMD_REPLACE_LANE_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_SIMD_REPLACE_LANE_OP(GENERATE_SIMD_REPLACE_LANE_CODE_CASE)
#undef GENERATE_SIMD_REPLACE_LANE_CODE_CASE
        {
            SIMDReplaceLane *replaceLane = reinterpret_cast<SIMDReplaceLane *>(const_cast<ByteCode *>(this));

            switch (index) {
            case 0: {
                replaceLane->setIndex(offset);
                break;
            }
            case 1: {
                replaceLane->setSrc0Offset(offset);
                break;
            }
            case 2: {
                replaceLane->setSrc1Offset(offset);
                break;
            }
            case 3: {
                replaceLane->setDstOffset(offset);
                break;
            }
            }
            break;
        }
    case Walrus::ByteCode::SelectOpcode: {
        Walrus::Select *sel = reinterpret_cast<Walrus::Select *>(const_cast<ByteCode *>(this));
        switch (index) {
        case 0: {
            sel->setSrc0Offset(offset);
            break;
        }
        case 1: {
            sel->setSrc1Offset(offset);
            break;
        }
        case 2: {
            sel->setCondOffset(offset);
            break;
        }
        case 3: {
            sel->setDstOffset(offset);
            break;
        }
        default: {
            RELEASE_ASSERT_NOT_REACHED();
        }
        }
        break;
    }
    case Walrus::ByteCode::Const32Opcode:
    case Walrus::ByteCode::Const64Opcode:
    case Walrus::ByteCode::Const128Opcode: {
        Const32 *code = reinterpret_cast<Walrus::Const32 *>(const_cast<ByteCode *>(this));
        code->setDstOffset(offset);
        break;
    }
    case Walrus::ByteCode::MemorySizeOpcode: {
        MemorySize *memorySize = reinterpret_cast<Walrus::MemorySize *>(const_cast<ByteCode *>(this));
        memorySize->setDstOffset(offset);
        break;
    }
    case Walrus::ByteCode::MemoryInitOpcode: {
        MemoryInit *memoryInit = reinterpret_cast<Walrus::MemoryInit *>(const_cast<ByteCode *>(this));
        memoryInit->setStackOffset(index, offset);
        break;
    }
    case Walrus::ByteCode::TableSizeOpcode: {
        ByteCodeOffsetValue *tableSize = reinterpret_cast<Walrus::ByteCodeOffsetValue *>(const_cast<ByteCode *>(this));
        tableSize->setStackOffset(offset);
        break;
    }
    case Walrus::ByteCode::TableGrowOpcode: {
        TableGrow *tableGrow = reinterpret_cast<Walrus::TableGrow *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            tableGrow->setSrc0Offset(offset);
        } else if (index == 1) {
            tableGrow->setSrc1Offset(offset);
        } else {
            tableGrow->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::TableInitOpcode: {
        TableInit *tableInit = reinterpret_cast<Walrus::TableInit *>(const_cast<ByteCode *>(this));
        tableInit->setStackOffset(index, offset);
        break;
    }
    case Walrus::ByteCode::TableCopyOpcode: {
        TableCopy *tableCopy = reinterpret_cast<Walrus::TableCopy *>(const_cast<ByteCode *>(this));
        tableCopy->setStackOffset(index, offset);
        break;
    }
    case Walrus::ByteCode::TableFillOpcode: {
        TableFill *tableFill = reinterpret_cast<Walrus::TableFill *>(const_cast<ByteCode *>(this));
        tableFill->setSrcOffset(index, offset);
        break;
    }
    case Walrus::ByteCode::I8X16ShuffleOpcode: {
        I8X16Shuffle *shuffle = reinterpret_cast<Walrus::I8X16Shuffle *>(const_cast<ByteCode *>(this));
        if (index < 2) {
            shuffle->setSrcOffset(index, offset);
        } else {
            shuffle->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::V128BitSelectOpcode: {
        V128BitSelect *select = reinterpret_cast<Walrus::V128BitSelect *>(const_cast<ByteCode *>(this));
        select->setStackOffset(index, offset);
        break;
    }
    case Walrus::ByteCode::JumpIfTrueOpcode:
    case Walrus::ByteCode::JumpIfFalseOpcode:
    case Walrus::ByteCode::GlobalGet32Opcode:
    case Walrus::ByteCode::GlobalGet64Opcode:
    case Walrus::ByteCode::GlobalGet128Opcode:
    case Walrus::ByteCode::GlobalSet32Opcode:
    case Walrus::ByteCode::GlobalSet64Opcode:
    case Walrus::ByteCode::GlobalSet128Opcode:
    case Walrus::ByteCode::RefFuncOpcode:
    case Walrus::ByteCode::JumpIfNullOpcode:
    case Walrus::ByteCode::JumpIfNonNullOpcode:
    case Walrus::ByteCode::JumpIfCastGenericOpcode:
    case Walrus::ByteCode::JumpIfCastDefinedOpcode: {
        Walrus::ByteCodeOffsetValue *code = reinterpret_cast<Walrus::ByteCodeOffsetValue *>(const_cast<ByteCode *>(this));
        code->setStackOffset(offset);
        break;
    }
    case Walrus::ByteCode::EndOpcode: {
        Walrus::End *end = reinterpret_cast<Walrus::End *>(const_cast<ByteCode *>(this));
        end->setResultOffset(index, offset);
        break;
    }
    case Walrus::ByteCode::CallOpcode: {
        Walrus::Call *call = reinterpret_cast<Walrus::Call *>(const_cast<ByteCode *>(this));
        call->setStackOffset(index, offset);
        break;
    }
    case Walrus::ByteCode::CallIndirectOpcode: {
        Walrus::CallIndirect *call = reinterpret_cast<Walrus::CallIndirect *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            call->setCalleeOffset(offset);
        } else {
            call->setStackOffset(index - 1, offset);
        }
        break;
    }
    case Walrus::ByteCode::CallRefOpcode: {
        Walrus::CallRef *callRef = reinterpret_cast<Walrus::CallRef *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            callRef->setCalleeOffset(offset);
        } else {
            callRef->setStackOffset(index - 1, offset);
        }
        break;
    }
    case Walrus::ByteCode::BrTableOpcode: {
        Walrus::BrTable *brTable = reinterpret_cast<Walrus::BrTable *>(const_cast<ByteCode *>(this));
        brTable->setCondOffset(offset);
        break;
    }
    case Walrus::ByteCode::ThrowOpcode: {
        Walrus::Throw *thr = reinterpret_cast<Walrus::Throw *>(const_cast<ByteCode *>(this));
        thr->setDataOffset(index, offset);
        break;
    }
#define GENERATE_ATOMIC_RMW_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_ATOMIC_RMW_OP(GENERATE_ATOMIC_RMW_CODE_CASE)
#undef GENERATE_ATOMIC_RMW_CODE_CASE
        {
            Walrus::AtomicRmw *atomic = reinterpret_cast<Walrus::AtomicRmw *>(const_cast<ByteCode *>(this));
            switch (index) {
            case 0: {
                atomic->setSrc0Offset(offset);
                break;
            }
            case 1: {
                atomic->setSrc1Offset(offset);
                break;
            }
            case 2: {
                atomic->setDstOffset(offset);
                break;
            }
            }
            break;
        }

#define GENERATE_ATOMIC_RMW_CMPXCHG_CODE_CASE(name, ...) \
    case Walrus::ByteCode::name##Opcode:
        FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(GENERATE_ATOMIC_RMW_CMPXCHG_CODE_CASE)
#undef GENERATE_ATOMIC_RMW_CODE_CASE
        {
            Walrus::ByteCodeOffset4Value *code = reinterpret_cast<Walrus::ByteCodeOffset4Value *>(const_cast<ByteCode *>(this));

            switch (index) {
            case 0: {
                code->setSrc0Offset(offset);
                break;
            }
            case 1: {
                code->setSrc1Offset(offset);
                break;
            }
            case 2: {
                code->setSrc2Offset(offset);
                break;
            }
            case 3: {
                code->setDstOffset(offset);
                break;
            }
            }
            break;
        }
    case Walrus::ByteCode::MemoryAtomicNotifyOpcode: {
        Walrus::MemoryAtomicNotify *atomic = reinterpret_cast<Walrus::MemoryAtomicNotify *>(const_cast<ByteCode *>(this));
        switch (index) {
        case 0: {
            atomic->setSrc0Offset(offset);
            break;
        }
        case 1: {
            atomic->setSrc1Offset(offset);
            break;
        }
        case 2: {
            atomic->setDstOffset(offset);
            break;
        }
        }
        break;
    }
    case Walrus::ByteCode::MemoryAtomicNotifyMemIdxOpcode: {
        Walrus::MemoryAtomicNotifyMemIdx *atomic = reinterpret_cast<Walrus::MemoryAtomicNotifyMemIdx *>(const_cast<ByteCode *>(this));
        switch (index) {
        case 0: {
            atomic->setSrc0Offset(offset);
            break;
        }
        case 1: {
            atomic->setSrc1Offset(offset);
            break;
        }
        case 2: {
            atomic->setDstOffset(offset);
            break;
        }
        }
        break;
    }
    case Walrus::ByteCode::MemoryAtomicWait32Opcode:
    case Walrus::ByteCode::MemoryAtomicWait32MemIdxOpcode:
    case Walrus::ByteCode::MemoryAtomicWait64Opcode:
    case Walrus::ByteCode::MemoryAtomicWait64MemIdxOpcode: {
        Walrus::ByteCodeOffset4Value *atomic = reinterpret_cast<Walrus::ByteCodeOffset4Value *>(const_cast<ByteCode *>(this));
        switch (index) {
        case 0: {
            atomic->setSrc0Offset(offset);
            break;
        }
        case 1: {
            atomic->setSrc1Offset(offset);
            break;
        }
        case 2: {
            atomic->setSrc2Offset(offset);
            break;
        }
        case 3: {
            atomic->setDstOffset(offset);
            break;
        }
        }
        break;
    }
    case Walrus::ByteCode::RefAsNonNullOpcode: {
        Walrus::RefAsNonNull *refAs = reinterpret_cast<Walrus::RefAsNonNull *>(const_cast<ByteCode *>(this));
        refAs->setStackOffset(offset);
        break;
    }
    case Walrus::ByteCode::RefCastGenericOpcode: {
        Walrus::RefCastGeneric *generic = reinterpret_cast<Walrus::RefCastGeneric *>(const_cast<ByteCode *>(this));
        generic->setSrcOffset(offset);
        break;
    }
    case Walrus::ByteCode::RefCastDefinedOpcode: {
        Walrus::RefCastDefined *generic = reinterpret_cast<Walrus::RefCastDefined *>(const_cast<ByteCode *>(this));
        generic->setSrcOffset(offset);
        break;
    }
    case Walrus::ByteCode::RefTestGenericOpcode: {
        Walrus::RefTestGeneric *generic = reinterpret_cast<Walrus::RefTestGeneric *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            generic->setSrcOffset(offset);
        } else {
            generic->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::RefTestDefinedOpcode: {
        Walrus::RefTestDefined *generic = reinterpret_cast<Walrus::RefTestDefined *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            generic->setSrcOffset(offset);
        } else {
            generic->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayNewOpcode: {
        Walrus::ArrayNew *arrNew = reinterpret_cast<Walrus::ArrayNew *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arrNew->setSrc0Offset(offset);
        } else if (index == 1) {
            arrNew->setSrc1Offset(offset);
        } else {
            arrNew->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayNewDefaultOpcode: {
        Walrus::ArrayNewDefault *arrNew = reinterpret_cast<Walrus::ArrayNewDefault *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arrNew->setSrcOffset(offset);
        } else {
            arrNew->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayNewFixedOpcode: {
        Walrus::ArrayNewFixed *fixedArr = reinterpret_cast<Walrus::ArrayNewFixed *>(const_cast<ByteCode *>(this));
        if (index < fixedArr->length()) {
            fixedArr->dataOffsets()[index] = offset;
        } else {
            fixedArr->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayNewDataOpcode: {
        Walrus::ArrayNewData *arr = reinterpret_cast<Walrus::ArrayNewData *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arr->setSrc0Offset(offset);
        } else if (index == 1) {
            arr->setSrc1Offset(offset);
        } else {
            arr->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayNewElemOpcode: {
        Walrus::ArrayNewElem *arr = reinterpret_cast<Walrus::ArrayNewElem *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arr->setSrc0Offset(offset);
        } else if (index == 1) {
            arr->setSrc1Offset(offset);
        } else {
            arr->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayFillOpcode: {
        Walrus::ArrayFill *arr = reinterpret_cast<Walrus::ArrayFill *>(const_cast<ByteCode *>(this));
        switch (index) {
        case 0: {
            arr->setSrc0Offset(offset);
            break;
        }
        case 1: {
            arr->setSrc1Offset(offset);
            break;
        }
        case 2: {
            arr->setSrc2Offset(offset);
            break;
        }
        case 3: {
            arr->setSrc3Offset(offset);
            break;
        }
        }
        break;
    }
    case Walrus::ByteCode::ArrayCopyOpcode: {
        Walrus::ArrayCopy *arr = reinterpret_cast<Walrus::ArrayCopy *>(const_cast<ByteCode *>(this));
        switch (index) {
        case 0: {
            arr->setSrc0Offset(offset);
            break;
        }
        case 1: {
            arr->setSrc1Offset(offset);
            break;
        }
        case 2: {
            arr->setSrc2Offset(offset);
            break;
        }
        case 3: {
            arr->setSrc3Offset(offset);
            break;
        }
        case 4: {
            arr->setSrc4Offset(offset);
            break;
        }
        }
        break;
    }
    case Walrus::ByteCode::ArrayInitDataOpcode: {
        Walrus::ArrayInitData *arr = reinterpret_cast<Walrus::ArrayInitData *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arr->setSrc0Offset(offset);
        } else if (index == 1) {
            arr->setSrc1Offset(offset);
        } else if (index == 2) {
            arr->setSrc2Offset(offset);
        } else {
            arr->setSrc3Offset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayInitElemOpcode: {
        Walrus::ArrayInitElem *arr = reinterpret_cast<Walrus::ArrayInitElem *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arr->setSrc0Offset(offset);
        } else if (index == 1) {
            arr->setSrc1Offset(offset);
        } else if (index == 2) {
            arr->setSrc2Offset(offset);
        } else {
            arr->setSrc3Offset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArrayGetOpcode: {
        Walrus::ArrayGet *arr = reinterpret_cast<Walrus::ArrayGet *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arr->setSrc0Offset(offset);
        } else if (index == 1) {
            arr->setSrc1Offset(offset);
        } else {
            arr->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ArraySetOpcode: {
        Walrus::ArraySet *arr = reinterpret_cast<Walrus::ArraySet *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            arr->setSrc0Offset(offset);
        } else if (index == 1) {
            arr->setSrc1Offset(offset);
        } else {
            arr->setSrc2Offset(offset);
        }
        break;
    }
    case Walrus::ByteCode::StructNewOpcode: {
        Walrus::StructNew *structNew = reinterpret_cast<Walrus::StructNew *>(const_cast<ByteCode *>(this));
        if (index < structNew->offsetsSize()) {
            structNew->setDataOffset(offset, index);
        } else {
            structNew->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::StructNewDefaultOpcode: {
        Walrus::StructNewDefault *structNewDefault = reinterpret_cast<Walrus::StructNewDefault *>(const_cast<ByteCode *>(this));
        structNewDefault->setDstOffset(offset);
        break;
    }
    case Walrus::ByteCode::StructGetOpcode: {
        Walrus::StructGet *structGet = reinterpret_cast<Walrus::StructGet *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            structGet->setSrcOffset(offset);
        } else {
            structGet->setDstOffset(offset);
        }
        break;
    }
    case Walrus::ByteCode::StructSetOpcode: {
        Walrus::StructSet *structSet = reinterpret_cast<Walrus::StructSet *>(const_cast<ByteCode *>(this));
        if (index == 0) {
            structSet->setSrc0Offset(offset);
        } else {
            structSet->setSrc1Offset(offset);
        }
        break;
    }
    case Walrus::ByteCode::ElemDropOpcode:
    case Walrus::ByteCode::DataDropOpcode:
    case Walrus::ByteCode::JumpOpcode:
    case Walrus::ByteCode::NopOpcode:
    case Walrus::ByteCode::UnreachableOpcode:
    case Walrus::ByteCode::AtomicFenceOpcode: {
        break;
    }
    default: {
        RELEASE_ASSERT_NOT_REACHED();
    }
    }
}
} // namespace Walrus
