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

#define CHECK_BYTECODE_ALIGNMENT(name, ...) \
    COMPILE_ASSERT(alignof(name) == alignof(ByteCode), "");
FOR_EACH_BYTECODE(CHECK_BYTECODE_ALIGNMENT)
#undef CHECK_BYTECODE_ALIGNMENT
// clang-format on

ByteCode::ByteCode(ByteCode::Opcode opcode)
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    : m_opcodeInOffset(g_byteCodeTable.m_offsetTable[opcode])
#else
    : m_opcode(opcode)
#endif
{
}

#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
ByteCode::Opcode ByteCode::opcode() const
{
    return static_cast<Opcode>(g_byteCodeTable.m_offsetToOpcodeTable[m_opcodeInOffset]);
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
        return ByteCode::alignedSize(sizeof(BrTable) + sizeof(int32_t) * brTable->tableSize());
    }
    case CallOpcode: {
        const Call* call = reinterpret_cast<const Call*>(this);
        return ByteCode::alignedSize(sizeof(Call) + sizeof(ByteCodeStackOffset) * call->parameterOffsetsSize()
                                     + sizeof(ByteCodeStackOffset) * call->resultOffsetsSize());
    }
    case ReturnCallOpcode: {
        const ReturnCall* returnCall = reinterpret_cast<const ReturnCall*>(this);
        return ByteCode::alignedSize(sizeof(ReturnCall) + sizeof(ByteCodeStackOffset) * returnCall->parameterOffsetsSize()
                                     + sizeof(ByteCodeStackOffset) * returnCall->resultOffsetsSize());
    }
    case CallIndirectOpcode:
    case CallIndirectM64Opcode:
    case ReturnCallIndirectOpcode:
    case ReturnCallIndirectM64Opcode: {
        const CallTable* callTable = reinterpret_cast<const CallTable*>(this);
        return ByteCode::alignedSize(sizeof(CallTable) + sizeof(ByteCodeStackOffset) * callTable->parameterOffsetsSize()
                                     + sizeof(ByteCodeStackOffset) * callTable->resultOffsetsSize());
    }
    case CallRefOpcode: {
        const CallRef* callRef = reinterpret_cast<const CallRef*>(this);
        return ByteCode::alignedSize(sizeof(CallRef) + sizeof(ByteCodeStackOffset) * callRef->parameterOffsetsSize()
                                     + sizeof(ByteCodeStackOffset) * callRef->resultOffsetsSize());
    }
    case ReturnCallRefOpcode: {
        const ReturnCallRef* returnCallRef = reinterpret_cast<const ReturnCallRef*>(this);
        return ByteCode::alignedSize(sizeof(ReturnCallRef) + sizeof(ByteCodeStackOffset) * returnCallRef->parameterOffsetsSize()
                                     + sizeof(ByteCodeStackOffset) * returnCallRef->resultOffsetsSize());
    }
    case EndOpcode: {
        const End* end = reinterpret_cast<const End*>(this);
        return ByteCode::alignedSize(sizeof(End) + sizeof(ByteCodeStackOffset) * end->offsetsSize());
    }
    case ThrowOpcode: {
        const Throw* throwCode = reinterpret_cast<const Throw*>(this);
        return ByteCode::alignedSize(sizeof(Throw) + sizeof(ByteCodeStackOffset) * throwCode->offsetsSize());
    }
    case ArrayNewFixedOpcode: {
        const ArrayNewFixed* arrayNewFixedCode = reinterpret_cast<const ArrayNewFixed*>(this);
        return ByteCode::alignedSize(sizeof(ArrayNewFixed) + sizeof(ByteCodeStackOffset) * arrayNewFixedCode->offsetsSize());
    }
    case StructNewOpcode: {
        const StructNew* structNewCode = reinterpret_cast<const StructNew*>(this);
        return ByteCode::alignedSize(sizeof(StructNew) + sizeof(ByteCodeStackOffset) * structNewCode->offsetsSize());
    }
    default: {
        return g_byteCodeSize[this->opcode()];
    }
    }
    RELEASE_ASSERT_NOT_REACHED();
}


} // namespace Walrus
