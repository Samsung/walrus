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

#ifndef __WalrusOpcode__
#define __WalrusOpcode__

namespace Walrus {

enum OpcodeKind : size_t {
#define WABT_OPCODE(rtype, type1, type2, type3, memSize, prefix, code, name, \
                    text, decomp)                                            \
    name##Opcode,
#include "wabt/opcode.def"
#undef WABT_OPCODE
    InvalidOpcode,
};

struct ByteCodeInfo {
    enum ByteCodeType { ___,
                        I32,
                        I64,
                        F32,
                        F64,
                        V128 };
    OpcodeKind m_code;
    ByteCodeType m_resultType;
    ByteCodeType m_paramTypes[3];
    const char* m_name;

    size_t stackShrinkSize() const
    {
        ASSERT(m_code != OpcodeKind::InvalidOpcode);
        return byteCodeTypeToMemorySize(m_paramTypes[0]) + byteCodeTypeToMemorySize(m_paramTypes[1]) + byteCodeTypeToMemorySize(m_paramTypes[2]) + byteCodeTypeToMemorySize(m_paramTypes[3]);
    }

    size_t stackGrowSize() const
    {
        ASSERT(m_code != OpcodeKind::InvalidOpcode);
        return byteCodeTypeToMemorySize(m_resultType);
    }

    static size_t byteCodeTypeToMemorySize(ByteCodeType tp)
    {
        switch (tp) {
        case I32:
        case F32:
#if defined(ESCARGOT_32)
            return 4;
#else
            return 8; // for stack align
#endif
        case I64:
        case F64:
            return 8;
        case V128:
            return 16;
        default:
            return 0;
        }
    }
};

extern ByteCodeInfo g_byteCodeInfo[OpcodeKind::InvalidOpcode];

} // namespace Walrus

#endif // __WalrusOpcode__
