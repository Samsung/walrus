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

#if !defined(NDEBUG)
#include <cinttypes>
#endif

namespace Walrus {

class ByteCode {
public:
    OpcodeKind opcode() const { return m_opcode; }
#if !defined(NDEBUG)
    virtual ~ByteCode()
    {
    }

    virtual void dump(size_t pos)
    {
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(ByteCode);
    }
#endif

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

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("value: %" PRId32, m_value);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(I32Const);
    }
#endif

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

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("value: %" PRId64, m_value);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(I64Const);
    }
#endif

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

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("value: %f", m_value);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(F32Const);
    }
#endif

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

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("value: %lf", m_value);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(F64Const);
    }
#endif

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

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32, m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Call);
    }
#endif

protected:
    uint32_t m_index;
};

class LocalGet4 : public ByteCode {
public:
    LocalGet4(uint32_t offset)
        : ByteCode(OpcodeKind::LocalGet4Opcode)
        , m_offset(offset)
    {
    }

    uint32_t offset() const { return m_offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("offset: %" PRId32,
               m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(LocalGet4);
    }
#endif

protected:
    uint32_t m_offset;
};

class LocalGet8 : public ByteCode {
public:
    LocalGet8(uint32_t offset)
        : ByteCode(OpcodeKind::LocalGet8Opcode)
        , m_offset(offset)
    {
    }

    uint32_t offset() const { return m_offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("offset: %" PRId32,
               m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(LocalGet8);
    }
#endif

protected:
    uint32_t m_offset;
};

class LocalSet4 : public ByteCode {
public:
    LocalSet4(uint32_t offset)
        : ByteCode(OpcodeKind::LocalSet4Opcode)
        , m_offset(offset)
    {
    }

    uint32_t offset() const { return m_offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("offset: %" PRId32,
               m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(LocalSet4);
    }
#endif

protected:
    uint32_t m_offset;
};

class LocalSet8 : public ByteCode {
public:
    LocalSet8(uint32_t offset)
        : ByteCode(OpcodeKind::LocalSet8Opcode)
        , m_offset(offset)
    {
    }

    uint32_t offset() const { return m_offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("offset: %" PRId32,
               m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(LocalSet8);
    }
#endif

protected:
    uint32_t m_offset;
};

class LocalTee4 : public ByteCode {
public:
    LocalTee4(uint32_t offset)
        : ByteCode(OpcodeKind::LocalTee4Opcode)
        , m_offset(offset)
    {
    }

    uint32_t offset() const { return m_offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("offset: %" PRId32,
               m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(LocalTee4);
    }
#endif

protected:
    uint32_t m_offset;
};

class LocalTee8 : public ByteCode {
public:
    LocalTee8(uint32_t offset)
        : ByteCode(OpcodeKind::LocalTee8Opcode)
        , m_offset(offset)
    {
    }

    uint32_t offset() const { return m_offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("offset: %" PRId32,
               m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(LocalTee8);
    }
#endif

protected:
    uint32_t m_offset;
};

class Drop : public ByteCode {
public:
    Drop(uint32_t size)
        : ByteCode(OpcodeKind::DropOpcode)
        , m_size(size)
    {
    }

    uint32_t size() const { return m_size; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("size: %" PRId32, m_size);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Drop);
    }
#endif

protected:
    uint32_t m_size;
};

class Jump : public ByteCode {
public:
    Jump(int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpOpcode)
        , m_offset(offset)
    {
    }

    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("dst: %" PRId32, (int32_t)pos + m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Jump);
    }
#endif

protected:
    int32_t m_offset;
};

class JumpIfTrue : public ByteCode {
public:
    JumpIfTrue(int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpIfTrueOpcode)
        , m_offset(offset)
    {
    }

    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("dst: %" PRId32, (int32_t)pos + m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(JumpIfTrue);
    }
#endif

protected:
    int32_t m_offset;
};

class JumpIfFalse : public ByteCode {
public:
    JumpIfFalse(int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpIfFalseOpcode)
        , m_offset(offset)
    {
    }

    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("dst: %" PRId32, (int32_t)pos + m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(JumpIfFalse);
    }
#endif

protected:
    int32_t m_offset;
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
