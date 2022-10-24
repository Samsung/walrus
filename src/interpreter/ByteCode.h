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

#ifndef __WalrusByteCode__
#define __WalrusByteCode__

#include "interpreter/Opcode.h"

namespace Walrus {

class ByteCode {
public:
    OpcodeKind opcode() const { return m_opcode; }

protected:
    ByteCode(OpcodeKind opcode)
        : m_opcode(opcode)
    {
    }

    union {
        OpcodeKind m_opcode;
        void* m_opcodeInAddress; // TODO
    };
};

class I32Const : public ByteCode {
public:
    I32Const(int32_t value)
        : ByteCode(OpcodeKind::I32ConstOpcode)
        , m_value(value)
    {
    }

    int32_t value() const { return m_value; }

protected:
    int32_t m_value;
};

class BinaryOperation : public ByteCode {
public:
    BinaryOperation(OpcodeKind opcode)
        : ByteCode(opcode)
    {
    }
};

class UnaryOperation : public ByteCode {
public:
    UnaryOperation(OpcodeKind opcode)
        : ByteCode(opcode)
    {
    }
};

class I64Const : public ByteCode {
public:
    I64Const(int64_t value)
        : ByteCode(OpcodeKind::I64ConstOpcode)
        , m_value(value)
    {
    }

    int64_t value() const { return m_value; }

protected:
    int64_t m_value;
};

class F32Const : public ByteCode {
public:
    F32Const(float value)
        : ByteCode(OpcodeKind::F32ConstOpcode)
        , m_value(value)
    {
    }

    float value() const { return m_value; }

protected:
    float m_value;
};

class F64Const : public ByteCode {
public:
    F64Const(double value)
        : ByteCode(OpcodeKind::F64ConstOpcode)
        , m_value(value)
    {
    }

    double value() const { return m_value; }

protected:
    double m_value;
};

class Call : public ByteCode {
public:
    Call(uint32_t index)
        : ByteCode(OpcodeKind::CallOpcode)
        , m_index(index)
    {
    }

    uint32_t index() const { return m_index; }

protected:
    uint32_t m_index;
};

class LocalGet : public ByteCode {
public:
    LocalGet(uint32_t offset, uint32_t size)
        : ByteCode(OpcodeKind::LocalGetOpcode)
        , m_offset(offset)
        , m_size(size)
    {
    }

    uint32_t offset() const { return m_offset; }
    uint32_t size() const { return m_size; }

protected:
    uint32_t m_offset;
    uint32_t m_size;
};

class LocalSet : public ByteCode {
public:
    LocalSet(uint32_t offset, uint32_t size)
        : ByteCode(OpcodeKind::LocalSetOpcode)
        , m_offset(offset)
        , m_size(size)
    {
    }

    uint32_t offset() const { return m_offset; }
    uint32_t size() const { return m_size; }

protected:
    uint32_t m_offset;
    uint32_t m_size;
};

class Drop : public ByteCode {
public:
    Drop(uint32_t size)
        : ByteCode(OpcodeKind::DropOpcode)
        , m_size(size)
    {
    }

    uint32_t size() const { return m_size; }

protected:
    uint32_t m_size;
};

class End : public ByteCode {
public:
    End()
        : ByteCode(OpcodeKind::EndOpcode)
    {
    }

protected:
};

} // namespace Walrus

#endif // __WalrusByteCode__
