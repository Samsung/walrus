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
#include "runtime/Module.h"

#define DUMP_BYTECODE_OFFSET(name) \
    printf(#name ": %" PRIu32 " ", m_##name);
#endif

namespace Walrus {

class FunctionType;

class ByteCode {
public:
    OpcodeKind opcode() const { return m_opcode; }
    void* opcodeInAddress() const { return m_opcodeInAddress; }
#if !defined(NDEBUG)
    OpcodeKind orgOpcode() const
    {
        return m_orgOpcode;
    }
#endif

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
    friend class Interpreter;
    friend class OpcodeTable;
    ByteCode(OpcodeKind opcode)
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
        : m_opcodeInAddress(g_opcodeTable.m_addressTable[opcode])
#else
        : m_opcode(opcode)
#endif
#if !defined(NDEBUG)
        , m_orgOpcode(opcode)
#endif
    {
    }

    ByteCode()
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
        : m_opcodeInAddress(nullptr)
#else
        : m_opcode(OpcodeKind::InvalidOpcode)
#endif
#if !defined(NDEBUG)
        , m_orgOpcode(OpcodeKind::InvalidOpcode)
#endif
    {
    }

    union {
        OpcodeKind m_opcode;
        void* m_opcodeInAddress;
    };
#if !defined(NDEBUG)
    OpcodeKind m_orgOpcode;
#endif
};

class Const32 : public ByteCode {
public:
    Const32(uint32_t dstOffset, uint32_t value)
        : ByteCode(OpcodeKind::Const32Opcode)
        , m_dstOffset(dstOffset)
        , m_value(value)
    {
    }

    uint32_t dstOffset() const { return m_dstOffset; }
    uint32_t value() const { return m_value; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("value: %" PRId32, m_value);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Const32);
    }
#endif

protected:
    uint32_t m_dstOffset;
    uint32_t m_value;
};


class Const64 : public ByteCode {
public:
    Const64(uint32_t dstOffset, uint64_t value)
        : ByteCode(OpcodeKind::Const64Opcode)
        , m_dstOffset(dstOffset)
        , m_value(value)
    {
    }

    uint32_t dstOffset() const { return m_dstOffset; }
    uint64_t value() const { return m_value; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("value: %" PRIu64, m_value);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Const64);
    }
#endif

protected:
    uint32_t m_dstOffset;
    uint64_t m_value;
};

class BinaryOperation : public ByteCode {
public:
    BinaryOperation(OpcodeKind opcode, uint32_t src0Offset, uint32_t src1Offset, uint32_t dstOffset)
        : ByteCode(opcode)
        , m_srcOffset{ src0Offset, src1Offset }
        , m_dstOffset(dstOffset)
    {
    }

    const uint32_t* srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset[0]);
        DUMP_BYTECODE_OFFSET(srcOffset[1]);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(BinaryOperation);
    }
#endif

protected:
    uint32_t m_srcOffset[2];
    uint32_t m_dstOffset;
};

class UnaryOperation : public ByteCode {
public:
    UnaryOperation(OpcodeKind opcode, uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(UnaryOperation);
    }
#endif

protected:
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class Call : public ByteCode {
public:
    Call(uint32_t index
#if !defined(NDEBUG)
         ,
         FunctionType* functionType
#endif
         )
        : ByteCode(OpcodeKind::CallOpcode)
        , m_index(index)
#if !defined(NDEBUG)
        , m_functionType(functionType)
#endif
    {
    }

    uint32_t index() const { return m_index; }
    uint32_t* stackOffsets() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(this) + sizeof(Call));
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32 " ", m_index);
        size_t c = 0;
        auto arr = stackOffsets();
        printf("paramOffsets: ");
        for (size_t i = 0; i < m_functionType->param().size(); i++) {
            printf("%" PRIu32 " ", arr[c++]);
        }
        printf(" ");

        printf("resultOffsets: ");
        for (size_t i = 0; i < m_functionType->result().size(); i++) {
            printf("%" PRIu32 " ", arr[c++]);
        }
        printf(" ");
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Call) + sizeof(uint32_t) * m_functionType->param().size() + sizeof(uint32_t) * m_functionType->result().size();
    }
#endif

protected:
    uint32_t m_index;
#if !defined(NDEBUG)
    FunctionType* m_functionType;
#endif
};

class CallIndirect : public ByteCode {
public:
    CallIndirect(uint32_t stackOffset, uint32_t tableIndex, FunctionType* functionType)
        : ByteCode(OpcodeKind::CallIndirectOpcode)
        , m_calleeOffset(stackOffset)
        , m_tableIndex(tableIndex)
        , m_functionType(functionType)
    {
    }

    uint32_t calleeOffset() const { return m_calleeOffset; }
    uint32_t tableIndex() const { return m_tableIndex; }
    FunctionType* functionType() const { return m_functionType; }
    uint32_t* stackOffsets() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(this) + sizeof(CallIndirect));
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tableIndex: %" PRId32 " ", m_tableIndex);
        DUMP_BYTECODE_OFFSET(calleeOffset);

        size_t c = 0;
        auto arr = stackOffsets();
        printf("paramOffsets: ");
        for (size_t i = 0; i < m_functionType->param().size(); i++) {
            printf("%" PRIu32 " ", arr[c++]);
        }
        printf(" ");

        printf("resultOffsets: ");
        for (size_t i = 0; i < m_functionType->result().size(); i++) {
            printf("%" PRIu32 " ", arr[c++]);
        }
        printf(" ");
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(CallIndirect) + sizeof(uint32_t) * m_functionType->param().size() + sizeof(uint32_t) * m_functionType->result().size();
    }
#endif

protected:
    uint32_t m_calleeOffset;
    uint32_t m_tableIndex;
    FunctionType* m_functionType;
};

class Move32 : public ByteCode {
public:
    Move32(uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(OpcodeKind::Move32Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Move32);
    }
#endif

protected:
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class Move64 : public ByteCode {
public:
    Move64(uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(OpcodeKind::Move64Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Move64);
    }
#endif

protected:
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class Load32 : public ByteCode {
public:
    Load32(uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(OpcodeKind::Load32Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Load32);
    }
#endif
protected:
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class Load64 : public ByteCode {
public:
    Load64(uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(OpcodeKind::Load64Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Load64);
    }
#endif
protected:
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class Store32 : public ByteCode {
public:
    Store32(uint32_t src0, uint32_t src1)
        : ByteCode(OpcodeKind::Store32Opcode)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    uint32_t src0Offset() const { return m_src0Offset; }
    uint32_t src1Offset() const { return m_src1Offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Store32);
    }
#endif
protected:
    uint32_t m_src0Offset;
    uint32_t m_src1Offset;
};

class Store64 : public ByteCode {
public:
    Store64(uint32_t src0, uint32_t src1)
        : ByteCode(OpcodeKind::Store64Opcode)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    uint32_t src0Offset() const { return m_src0Offset; }
    uint32_t src1Offset() const { return m_src1Offset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Store64);
    }
#endif
protected:
    uint32_t m_src0Offset;
    uint32_t m_src1Offset;
};

class Drop : public ByteCode {
public:
    Drop(uint32_t stackOffset, uint32_t dropSize, uint32_t parameterSize)
        : ByteCode(OpcodeKind::DropOpcode)
        , m_stackOffset(stackOffset)
        , m_dropSize(dropSize)
        , m_parameterSize(parameterSize)
    {
    }

    uint32_t stackOffset() const { return m_stackOffset; }
    uint32_t dropSize() const { return m_dropSize; }
    uint32_t parameterSize() const { return m_parameterSize; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("dropSize: %" PRId32 ", parameterSize: %" PRId32, m_dropSize, m_parameterSize);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Drop);
    }
#endif

protected:
    uint32_t m_stackOffset;
    uint32_t m_dropSize;
    uint32_t m_parameterSize;
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
    uint32_t m_offset;
};

class JumpIfTrue : public ByteCode {
public:
    JumpIfTrue(uint32_t srcOffset, int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpIfTrueOpcode)
        , m_srcOffset(srcOffset)
        , m_offset(offset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        printf("dst: %" PRId32, (int32_t)pos + m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(JumpIfTrue);
    }
#endif

protected:
    uint32_t m_srcOffset;
    int32_t m_offset;
};

class JumpIfFalse : public ByteCode {
public:
    JumpIfFalse(uint32_t srcOffset, int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpIfFalseOpcode)
        , m_srcOffset(srcOffset)
        , m_offset(offset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        printf("dst: %" PRId32, (int32_t)pos + m_offset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(JumpIfFalse);
    }
#endif

protected:
    uint32_t m_srcOffset;
    int32_t m_offset;
};

class Select : public ByteCode {
public:
    Select(uint32_t condOffset, uint32_t size, uint32_t src0, uint32_t src1, uint32_t dst)
        : ByteCode(OpcodeKind::SelectOpcode)
        , m_condOffset(condOffset)
        , m_valueSize(size)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
    {
    }

    uint32_t condOffset() const { return m_condOffset; }
    uint32_t valueSize() const
    {
        return m_valueSize;
    }
    uint32_t src0Offset() const { return m_src0Offset; }
    uint32_t src1Offset() const { return m_src1Offset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(condOffset);
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Select);
    }
#endif

protected:
    uint32_t m_condOffset;
    uint32_t m_valueSize;
    uint32_t m_src0Offset;
    uint32_t m_src1Offset;
    uint32_t m_dstOffset;
};

class BrTable : public ByteCode {
public:
    BrTable(uint32_t condOffset, uint32_t m_tableSize)
        : ByteCode(OpcodeKind::BrTableOpcode)
        , m_condOffset(condOffset)
        , m_defaultOffset(0)
        , m_tableSize(m_tableSize)
    {
    }

    uint32_t condOffset() const { return m_condOffset; }
    int32_t defaultOffset() const { return m_defaultOffset; }
    void setDefaultOffset(int32_t offset)
    {
        m_defaultOffset = offset;
    }

    uint32_t tableSize() const { return m_tableSize; }
    int32_t* jumpOffsets() const
    {
        return reinterpret_cast<int32_t*>(reinterpret_cast<size_t>(this) + sizeof(BrTable));
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(condOffset);
        printf("tableSize: %" PRIu32 ", defaultOffset: %" PRId32, m_tableSize, m_defaultOffset);
        printf(" table contents: ");
        for (size_t i = 0; i < m_tableSize; i++) {
            printf("%zu->%" PRId32 " ", i, jumpOffsets()[i]);
        }
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(BrTable) + sizeof(int32_t) * m_tableSize;
    }
#endif

protected:
    uint32_t m_condOffset;
    int32_t m_defaultOffset;
    uint32_t m_tableSize;
};

class MemorySize : public ByteCode {
public:
    MemorySize(uint32_t index, uint32_t dstOffset)
        : ByteCode(OpcodeKind::MemorySizeOpcode)
        , m_dstOffset(dstOffset)
    {
        ASSERT(index == 0);
    }

    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(dstOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(MemorySize);
    }
#endif

protected:
    uint32_t m_dstOffset;
};

class MemoryInit : public ByteCode {
public:
    MemoryInit(uint32_t index, uint32_t segmentIndex, uint32_t src0, uint32_t src1, uint32_t src2)
        : ByteCode(OpcodeKind::MemoryInitOpcode)
        , m_segmentIndex(segmentIndex)
        , m_srcOffsets{ src0, src1, src2 }
    {
        ASSERT(index == 0);
    }

    uint32_t segmentIndex() const
    {
        return m_segmentIndex;
    }

    const uint32_t* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("segmentIndex: %" PRIu32, m_segmentIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryInit);
    }
#endif

protected:
    uint32_t m_segmentIndex;
    uint32_t m_srcOffsets[3];
};

class MemoryCopy : public ByteCode {
public:
    MemoryCopy(uint32_t srcIndex, uint32_t dstIndex, uint32_t src0, uint32_t src1, uint32_t src2)
        : ByteCode(OpcodeKind::MemoryCopyOpcode)
        , m_srcOffsets{ src0, src1, src2 }
    {
        ASSERT(srcIndex == 0);
        ASSERT(dstIndex == 0);
    }

    const uint32_t* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryCopy);
    }
#endif
protected:
    uint32_t m_srcOffsets[3];
};

class MemoryFill : public ByteCode {
public:
    MemoryFill(uint32_t memIdx, uint32_t src0, uint32_t src1, uint32_t src2)
        : ByteCode(OpcodeKind::MemoryFillOpcode)
        , m_srcOffsets{ src0, src1, src2 }
    {
        ASSERT(memIdx == 0);
    }

    const uint32_t* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryFill);
    }
#endif
protected:
    uint32_t m_srcOffsets[3];
};

class DataDrop : public ByteCode {
public:
    DataDrop(uint32_t segmentIndex)
        : ByteCode(OpcodeKind::DataDropOpcode)
        , m_segmentIndex(segmentIndex)
    {
    }

    uint32_t segmentIndex() const
    {
        return m_segmentIndex;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("segmentIndex: %" PRIu32, m_segmentIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(DataDrop);
    }
#endif

protected:
    uint32_t m_segmentIndex;
};

class MemoryGrow : public ByteCode {
public:
    MemoryGrow(uint32_t index, uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(OpcodeKind::MemoryGrowOpcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
        ASSERT(index == 0);
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryGrow);
    }
#endif

protected:
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class MemoryLoad : public ByteCode {
public:
    MemoryLoad(OpcodeKind opcode, uint32_t offset, uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(opcode)
        , m_offset(offset)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t offset() const
    {
        return m_offset;
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryLoad);
    }

    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("offset: %" PRIu32, m_offset);
    }
#endif
protected:
    uint32_t m_offset;
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class MemoryStore : public ByteCode {
public:
    MemoryStore(OpcodeKind opcode, uint32_t offset, uint32_t src0, uint32_t src1)
        : ByteCode(opcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    uint32_t offset() const
    {
        return m_offset;
    }

    uint32_t src0Offset() const { return m_src0Offset; }
    uint32_t src1Offset() const { return m_src1Offset; }

#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryStore);
    }

    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
        printf("offset: %" PRIu32, m_offset);
    }
#endif
protected:
    uint32_t m_offset;
    uint32_t m_src0Offset;
    uint32_t m_src1Offset;
};

class TableGet : public ByteCode {
public:
    TableGet(uint32_t index, uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(OpcodeKind::TableGetOpcode)
        , m_tableIndex(index)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableGet);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class TableSet : public ByteCode {
public:
    TableSet(uint32_t index, uint32_t src0, uint32_t src1)
        : ByteCode(OpcodeKind::TableSetOpcode)
        , m_tableIndex(index)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    uint32_t src0Offset() const { return m_src0Offset; }
    uint32_t src1Offset() const { return m_src1Offset; }
    uint32_t tableIndex() const { return m_tableIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableSet);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_src0Offset;
    uint32_t m_src1Offset;
};

class TableGrow : public ByteCode {
public:
    TableGrow(uint32_t index, uint32_t src0, uint32_t src1, uint32_t dst)
        : ByteCode(OpcodeKind::TableGrowOpcode)
        , m_tableIndex(index)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    uint32_t src0Offset() const { return m_src0Offset; }
    uint32_t src1Offset() const { return m_src1Offset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableGrow);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_src0Offset;
    uint32_t m_src1Offset;
    uint32_t m_dstOffset;
};

class TableSize : public ByteCode {
public:
    TableSize(uint32_t index, uint32_t dst)
        : ByteCode(OpcodeKind::TableSizeOpcode)
        , m_tableIndex(index)
        , m_dstOffset(dst)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableSize);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_dstOffset;
};

class TableCopy : public ByteCode {
public:
    TableCopy(uint32_t dstIndex, uint32_t srcIndex, uint32_t src0, uint32_t src1, uint32_t src2)
        : ByteCode(OpcodeKind::TableCopyOpcode)
        , m_dstIndex(dstIndex)
        , m_srcIndex(srcIndex)
        , m_srcOffsets{ src0, src1, src2 }
    {
    }

    uint32_t dstIndex() const { return m_dstIndex; }
    uint32_t srcIndex() const { return m_srcIndex; }
    const uint32_t* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("dstIndex: %" PRIu32 " srcIndex: %" PRIu32, m_dstIndex, m_srcIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableCopy);
    }
#endif

protected:
    uint32_t m_dstIndex;
    uint32_t m_srcIndex;
    uint32_t m_srcOffsets[3];
};

class TableFill : public ByteCode {
public:
    TableFill(uint32_t index, uint32_t src0, uint32_t src1, uint32_t src2)
        : ByteCode(OpcodeKind::TableFillOpcode)
        , m_tableIndex(index)
        , m_srcOffsets{ src0, src1, src2 }
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    const uint32_t* srcOffsets() const
    {
        return m_srcOffsets;
    }
#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableFill);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_srcOffsets[3];
};

class TableInit : public ByteCode {
public:
    TableInit(uint32_t tableIndex, uint32_t segmentIndex, uint32_t src0, uint32_t src1, uint32_t src2)
        : ByteCode(OpcodeKind::TableInitOpcode)
        , m_tableIndex(tableIndex)
        , m_segmentIndex(segmentIndex)
        , m_srcOffsets{ src0, src1, src2 }
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    uint32_t segmentIndex() const { return m_segmentIndex; }
    const uint32_t* srcOffsets() const
    {
        return m_srcOffsets;
    }
#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("tableIndex: %" PRIu32 " segmentIndex: %" PRIu32, m_tableIndex, m_segmentIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableInit);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_segmentIndex;
    uint32_t m_srcOffsets[3];
};

class ElemDrop : public ByteCode {
public:
    ElemDrop(uint32_t segmentIndex)
        : ByteCode(OpcodeKind::ElemDropOpcode)
        , m_segmentIndex(segmentIndex)
    {
    }

    uint32_t segmentIndex() const
    {
        return m_segmentIndex;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("segmentIndex: %" PRIu32, m_segmentIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(ElemDrop);
    }
#endif

protected:
    uint32_t m_segmentIndex;
};

class RefFunc : public ByteCode {
public:
    RefFunc(uint32_t funcIndex, uint32_t dstOffset)
        : ByteCode(OpcodeKind::RefFuncOpcode)
        , m_funcIndex(funcIndex)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t dstOffset() const { return m_dstOffset; }
    uint32_t funcIndex() const { return m_funcIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("funcIndex: %" PRIu32, m_funcIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(RefFunc);
    }
#endif

private:
    uint32_t m_funcIndex;
    uint32_t m_dstOffset;
};

class RefNull : public ByteCode {
public:
    RefNull(Value::Type type, uint32_t dstOffset)
        : ByteCode(OpcodeKind::RefNullOpcode)
        , m_type(type)
        , m_dstOffset(dstOffset)
    {
    }

    Value::Type type() const { return m_type; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(dstOffset);
        if (m_type == Value::FuncRef) {
            printf("funcref");
        } else {
            ASSERT(m_type == Value::ExternRef);
            printf("externref");
        }
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(RefNull);
    }
#endif

private:
    Value::Type m_type;
    uint32_t m_dstOffset;
};

class RefIsNull : public ByteCode {
public:
    RefIsNull(uint32_t srcOffset, uint32_t dstOffset)
        : ByteCode(OpcodeKind::RefIsNullOpcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
    virtual size_t byteCodeSize()
    {
        return sizeof(RefIsNull);
    }
#endif
protected:
    uint32_t m_srcOffset;
    uint32_t m_dstOffset;
};

class GlobalGet32 : public ByteCode {
public:
    GlobalGet32(uint32_t dstOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalGet32Opcode)
        , m_dstOffset(dstOffset)
        , m_index(index)
    {
    }

    uint32_t dstOffset() const { return m_dstOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalGet32);
    }
#endif

protected:
    uint32_t m_dstOffset;
    uint32_t m_index;
};

class GlobalGet64 : public ByteCode {
public:
    GlobalGet64(uint32_t dstOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalGet64Opcode)
        , m_dstOffset(dstOffset)
        , m_index(index)
    {
    }

    uint32_t dstOffset() const { return m_dstOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalGet64);
    }
#endif

protected:
    uint32_t m_dstOffset;
    uint32_t m_index;
};

class GlobalSet32 : public ByteCode {
public:
    GlobalSet32(uint32_t srcOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalSet32Opcode)
        , m_srcOffset(srcOffset)
        , m_index(index)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalSet32);
    }
#endif

protected:
    uint32_t m_srcOffset;
    uint32_t m_index;
};

class GlobalSet64 : public ByteCode {
public:
    GlobalSet64(uint32_t srcOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalSet64Opcode)
        , m_srcOffset(srcOffset)
        , m_index(index)
    {
    }

    uint32_t srcOffset() const { return m_srcOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalSet64);
    }
#endif

protected:
    uint32_t m_srcOffset;
    uint32_t m_index;
};

class Throw : public ByteCode {
public:
    Throw(uint32_t index
#if !defined(NDEBUG)
          ,
          FunctionType* functionType
#endif
          )
        : ByteCode(OpcodeKind::ThrowOpcode)
        , m_tagIndex(index)
#if !defined(NDEBUG)
        , m_functionType(functionType)
#endif
    {
    }

    uint32_t tagIndex() const { return m_tagIndex; }
    uint32_t* dataOffsets() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(this) + sizeof(Throw));
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tagIndex: %" PRId32 " ",
               m_tagIndex);

        if (m_functionType) {
            auto arr = dataOffsets();
            printf("resultOffsets: ");
            for (size_t i = 0; i < m_functionType->result().size(); i++) {
                printf("%" PRIu32 " ", arr[i]);
            }
            printf(" ");
        }
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Throw) + m_functionType ? sizeof(uint32_t) * m_functionType->result().size() : 0;
    }
#endif

protected:
    uint32_t m_tagIndex;
#if !defined(NDEBUG)
    FunctionType* m_functionType;
#endif
};

class Unreachable : public ByteCode {
public:
    Unreachable()
        : ByteCode(OpcodeKind::UnreachableOpcode)
    {
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Unreachable);
    }
#endif

protected:
};

class End : public ByteCode {
public:
    End(
#if !defined(NDEBUG)
        FunctionType* functionType
#endif
        )
        : ByteCode(OpcodeKind::EndOpcode)
#if !defined(NDEBUG)
        , m_functionType(functionType)
#endif
    {
    }

    uint32_t* resultOffsets() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(this) + sizeof(End));
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        auto arr = resultOffsets();
        printf("resultOffsets: ");
        for (size_t i = 0; i < m_functionType->result().size(); i++) {
            printf("%" PRIu32 " ", arr[i]);
        }
        printf(" ");
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(End) + sizeof(uint32_t) * m_functionType->result().size();
    }

protected:
    FunctionType* m_functionType;
#endif
};

} // namespace Walrus

#endif // __WalrusByteCode__
