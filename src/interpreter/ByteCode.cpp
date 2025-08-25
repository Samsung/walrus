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
        const BrTable* brTable = reinterpret_cast<const BrTable*>(this);
        return ByteCode::pointerAlignedSize(sizeof(BrTable) + sizeof(int32_t) * brTable->tableSize());
    }
    case CallOpcode: {
        const Call* call = reinterpret_cast<const Call*>(this);
        return ByteCode::pointerAlignedSize(sizeof(Call) + sizeof(ByteCodeStackOffset) * call->parameterOffsetsSize()
                                            + sizeof(ByteCodeStackOffset) * call->resultOffsetsSize());
    }
    case CallIndirectOpcode: {
        const CallIndirect* callIndirect = reinterpret_cast<const CallIndirect*>(this);
        return ByteCode::pointerAlignedSize(sizeof(CallIndirect) + sizeof(ByteCodeStackOffset) * callIndirect->parameterOffsetsSize()
                                            + sizeof(ByteCodeStackOffset) * callIndirect->resultOffsetsSize());
    }
    case CallRefOpcode: {
        const CallRef* callRef = reinterpret_cast<const CallRef*>(this);
        return ByteCode::pointerAlignedSize(sizeof(CallRef) + sizeof(ByteCodeStackOffset) * callRef->parameterOffsetsSize()
                                            + sizeof(ByteCodeStackOffset) * callRef->resultOffsetsSize());
    }
    case EndOpcode: {
        const End* end = reinterpret_cast<const End*>(this);
        return ByteCode::pointerAlignedSize(sizeof(End) + sizeof(ByteCodeStackOffset) * end->offsetsSize());
    }
    case ThrowOpcode: {
        const Throw* throwCode = reinterpret_cast<const Throw*>(this);
        return ByteCode::pointerAlignedSize(sizeof(Throw) + sizeof(ByteCodeStackOffset) * throwCode->offsetsSize());
    }
    case StructNewOpcode: {
        const StructNew* structNewCode = reinterpret_cast<const StructNew*>(this);
        return ByteCode::pointerAlignedSize(sizeof(StructNew) + sizeof(ByteCodeStackOffset) * structNewCode->offsetsSize());
    }
    default: {
        return g_byteCodeSize[this->opcode()];
    }
    }
    RELEASE_ASSERT_NOT_REACHED();
}


} // namespace Walrus
