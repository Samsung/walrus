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
#include "interpreter/Opcode.h"
#include "runtime/ObjectType.h"

namespace Walrus {

extern ByteCodeInfo g_byteCodeInfo[OpcodeKind::InvalidOpcode];

static const uint8_t g_byteCodeSize[OpcodeKind::InvalidOpcode] = {
#define WABT_OPCODE(rtype, type1, type2, type3, memSize, prefix, code, name, \
                    text, decomp, size)                                      \
    size,
#include "interpreter/opcode.def"
#undef WABT_OPCODE
};

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
