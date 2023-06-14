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
#define DECLARE_BYTECODE_SIZE(name) sizeof(name),
    FOR_EACH_BYTECODE_OP(DECLARE_BYTECODE_SIZE)
#undef DECLARE_BYTECODE_SIZE
#define DECLARE_BYTECODE_SIZE(name, op, paramType, returnType) sizeof(name),
    FOR_EACH_BYTECODE_BINARY_OP(DECLARE_BYTECODE_SIZE)
    FOR_EACH_BYTECODE_UNARY_OP(DECLARE_BYTECODE_SIZE)
#undef DECLARE_BYTECODE_SIZE
#define DECLARE_BYTECODE_SIZE(name, op, paramType, returnType, T1, T2) sizeof(name),
    FOR_EACH_BYTECODE_UNARY_OP_2(DECLARE_BYTECODE_SIZE)
#undef DECLARE_BYTECODE_SIZE
#define DECLARE_BYTECODE_SIZE(name, readType, writeType) sizeof(name),
    FOR_EACH_BYTECODE_LOAD_OP(DECLARE_BYTECODE_SIZE)
    FOR_EACH_BYTECODE_STORE_OP(DECLARE_BYTECODE_SIZE)
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

size_t ByteCode::getSize()
{
    switch (this->opcode()) {
    case ThrowOpcode: {
        Throw* throwCode = reinterpret_cast<Throw*>(this);
        return sizeof(Throw) + sizeof(ByteCodeStackOffset) * throwCode->offsetsSize();
    }
    case CallOpcode: {
        Call* call = reinterpret_cast<Call*>(this);
        return sizeof(Call) + sizeof(ByteCodeStackOffset) * call->offsetsSize();
    }
    case BrTableOpcode: {
        BrTable* brTable = reinterpret_cast<BrTable*>(this);
        return sizeof(BrTable) + sizeof(int32_t) * brTable->tableSize();
    }
    case CallIndirectOpcode: {
        CallIndirect* callIndirect = reinterpret_cast<CallIndirect*>(this);
        size_t operands = callIndirect->functionType()->param().size() + callIndirect->functionType()->result().size();
        return sizeof(CallIndirect) + sizeof(ByteCodeStackOffset) * operands;
    }
    case EndOpcode: {
        End* end = reinterpret_cast<End*>(this);
        return sizeof(End) + sizeof(ByteCodeStackOffset) * end->offsetsSize();
    }
    default: {
        return g_byteCodeSize[this->opcode()];
    }
    }
    RELEASE_ASSERT_NOT_REACHED();
}


} // namespace Walrus
