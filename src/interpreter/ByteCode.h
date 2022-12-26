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
    void* opcodeInAddress() const { return m_opcodeInAddress; }
#if !defined(NDEBUG)
    OpcodeKind orgOpcode() const
    {
        return m_orgOpcode;
    }
#endif
    uint32_t stackOffset() const
    {
        return m_stackOffset;
    }
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
    ByteCode(OpcodeKind opcode, uint32_t stackOffset = 0)
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
        : m_opcodeInAddress(g_opcodeTable.m_addressTable[opcode])
#else
        : m_opcode(opcode)
#endif
#if !defined(NDEBUG)
        , m_orgOpcode(opcode)
#endif
        , m_stackOffset(stackOffset)
    {
    }

    ByteCode(uint32_t stackOffset = 0)
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
        : m_opcodeInAddress(nullptr)
#else
        : m_opcode(OpcodeKind::InvalidOpcode)
#endif
#if !defined(NDEBUG)
        , m_orgOpcode(OpcodeKind::InvalidOpcode)
#endif
        , m_stackOffset(stackOffset)
    {
    }

    union {
        OpcodeKind m_opcode;
        void* m_opcodeInAddress;
    };
#if !defined(NDEBUG)
    OpcodeKind m_orgOpcode;
#endif
    uint32_t m_stackOffset;
};

class Const32 : public ByteCode {
public:
    Const32(uint32_t stackOffset, uint32_t value)
        : ByteCode(OpcodeKind::Const32Opcode, stackOffset)
        , m_value(value)
    {
    }

    uint32_t value() const { return m_value; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("value: %" PRId32, m_value);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Const32);
    }
#endif

protected:
    uint32_t m_value;
};


class Const64 : public ByteCode {
public:
    Const64(uint32_t stackOffset, uint64_t value)
        : ByteCode(OpcodeKind::Const64Opcode, stackOffset)
        , m_value(value)
    {
    }

    uint64_t value() const { return m_value; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("value: %lf", static_cast<double>(m_value));
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Const64);
    }
#endif

protected:
    uint64_t m_value;
};

class BinaryOperation : public ByteCode {
public:
    BinaryOperation(OpcodeKind opcode, uint32_t stackOffset)
        : ByteCode(opcode, stackOffset)
    {
    }
};

class UnaryOperation : public ByteCode {
public:
    UnaryOperation(OpcodeKind opcode, uint32_t stackOffset)
        : ByteCode(opcode, stackOffset)
    {
    }
};

class Call : public ByteCode {
public:
    Call(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::CallOpcode, stackOffset)
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

class CallIndirect : public ByteCode {
public:
    CallIndirect(uint32_t stackOffset, uint32_t tableIndex, uint32_t functionTypeIndex)
        : ByteCode(OpcodeKind::CallIndirectOpcode, stackOffset)
        , m_tableIndex(tableIndex)
        , m_functionTypeIndex(functionTypeIndex)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    uint32_t functionTypeIndex() const { return m_functionTypeIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tableIndex: %" PRId32 "functionTypeIndex: %" PRId32, m_tableIndex, m_functionTypeIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(CallIndirect);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_functionTypeIndex;
};

class LocalGet4 : public ByteCode {
public:
    LocalGet4(uint32_t stackOffset, uint32_t offset)
        : ByteCode(OpcodeKind::LocalGet4Opcode, stackOffset)
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
    LocalGet8(uint32_t stackOffset, uint32_t offset)
        : ByteCode(OpcodeKind::LocalGet8Opcode, stackOffset)
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
    LocalSet4(uint32_t stackOffset, uint32_t offset)
        : ByteCode(OpcodeKind::LocalSet4Opcode, stackOffset)
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
    LocalSet8(uint32_t stackOffset, uint32_t offset)
        : ByteCode(OpcodeKind::LocalSet8Opcode, stackOffset)
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
    LocalTee4(uint32_t stackOffset, uint32_t offset)
        : ByteCode(OpcodeKind::LocalTee4Opcode, stackOffset)
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
    LocalTee8(uint32_t stackOffset, uint32_t offset)
        : ByteCode(OpcodeKind::LocalTee8Opcode, stackOffset)
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
    Drop(uint32_t stackOffset, uint32_t dropSize, uint32_t parameterSize)
        : ByteCode(OpcodeKind::DropOpcode, stackOffset)
        , m_dropSize(dropSize)
        , m_parameterSize(parameterSize)
    {
    }

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
    uint32_t m_dropSize;
    uint32_t m_parameterSize;
};

class Jump : public ByteCode {
public:
    Jump(int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpOpcode, offset)
    {
    }

    int32_t offset() const { return m_stackOffset; }
    void setOffset(int32_t offset)
    {
        m_stackOffset = offset;
    }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("dst: %" PRId32, (int32_t)pos + m_stackOffset);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Jump);
    }
#endif
};

class JumpIfTrue : public ByteCode {
public:
    JumpIfTrue(uint32_t stackOffset, int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpIfTrueOpcode, stackOffset)
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
    JumpIfFalse(uint32_t stackOffset, int32_t offset = 0)
        : ByteCode(OpcodeKind::JumpIfFalseOpcode, stackOffset)
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

class Select : public ByteCode {
public:
    Select(uint32_t stackOffset, uint32_t size)
        : ByteCode(OpcodeKind::SelectOpcode, stackOffset)
        , m_size(size)
    {
    }

    uint32_t size() const { return m_size; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("size: %" PRIu32, m_size);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Select);
    }
#endif

protected:
    uint32_t m_size;
};

class BrTable : public ByteCode {
public:
    BrTable(uint32_t stackOffset, uint32_t m_tableSize)
        : ByteCode(OpcodeKind::BrTableOpcode, stackOffset)
        , m_defaultOffset(0)
        , m_tableSize(m_tableSize)
    {
    }

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
    int32_t m_defaultOffset;
    uint32_t m_tableSize;
};

class MemorySize : public ByteCode {
public:
    MemorySize(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::MemorySizeOpcode, stackOffset)
    {
        ASSERT(index == 0);
    }


#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemorySize);
    }
#endif

protected:
};

class MemoryInit : public ByteCode {
public:
    MemoryInit(uint32_t stackOffset, uint32_t index, uint32_t segmentIndex)
        : ByteCode(OpcodeKind::MemoryInitOpcode, stackOffset)
        , m_segmentIndex(segmentIndex)
    {
        ASSERT(index == 0);
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
        return sizeof(MemoryInit);
    }
#endif

protected:
    uint32_t m_segmentIndex;
};

class MemoryCopy : public ByteCode {
public:
    MemoryCopy(uint32_t stackOffset, uint32_t srcIndex, uint32_t dstIndex)
        : ByteCode(OpcodeKind::MemoryCopyOpcode, stackOffset)
    {
        ASSERT(srcIndex == 0);
        ASSERT(dstIndex == 0);
    }

#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryCopy);
    }
#endif
};

class MemoryFill : public ByteCode {
public:
    MemoryFill(uint32_t stackOffset, uint32_t memIdx)
        : ByteCode(OpcodeKind::MemoryFillOpcode, stackOffset)
    {
        ASSERT(memIdx == 0);
    }

#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryFill);
    }
#endif
};

class DataDrop : public ByteCode {
public:
    DataDrop(uint32_t stackOffset, uint32_t segmentIndex)
        : ByteCode(OpcodeKind::DataDropOpcode, stackOffset)
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
    MemoryGrow(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::MemoryGrowOpcode, stackOffset)
    {
        ASSERT(index == 0);
    }


#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryGrow);
    }
#endif

protected:
};

class MemoryLoad : public ByteCode {
public:
    MemoryLoad(OpcodeKind opcode, uint32_t stackOffset, uint32_t offset)
        : ByteCode(opcode, stackOffset)
        , m_offset(offset)
    {
    }

    uint32_t offset() const
    {
        return m_offset;
    }

#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryLoad);
    }

    virtual void dump(size_t pos)
    {
        printf("offset: %" PRIu32, m_offset);
    }
#endif
protected:
    uint32_t m_offset;
};

class MemoryStore : public ByteCode {
public:
    MemoryStore(OpcodeKind opcode, uint32_t stackOffset, uint32_t offset)
        : ByteCode(opcode, stackOffset)
        , m_offset(offset)
    {
    }

    uint32_t offset() const
    {
        return m_offset;
    }

#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(MemoryStore);
    }

    virtual void dump(size_t pos)
    {
        printf("offset: %" PRIu32, m_offset);
    }
#endif
protected:
    uint32_t m_offset;
};

class TableGet : public ByteCode {
public:
    TableGet(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::TableGetOpcode, stackOffset)
        , m_tableIndex(index)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableGet);
    }
#endif

protected:
    uint32_t m_tableIndex;
};

class TableSet : public ByteCode {
public:
    TableSet(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::TableSetOpcode, stackOffset)
        , m_tableIndex(index)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableSet);
    }
#endif

protected:
    uint32_t m_tableIndex;
};

class TableGrow : public ByteCode {
public:
    TableGrow(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::TableGrowOpcode, stackOffset)
        , m_tableIndex(index)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableGrow);
    }
#endif

protected:
    uint32_t m_tableIndex;
};

class TableSize : public ByteCode {
public:
    TableSize(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::TableSizeOpcode, stackOffset)
        , m_tableIndex(index)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableSize);
    }
#endif

protected:
    uint32_t m_tableIndex;
};

class TableCopy : public ByteCode {
public:
    TableCopy(uint32_t stackOffset, uint32_t dstIndex, uint32_t srcIndex)
        : ByteCode(OpcodeKind::TableCopyOpcode, stackOffset)
        , m_dstIndex(dstIndex)
        , m_srcIndex(srcIndex)
    {
    }

    uint32_t dstIndex() const { return m_dstIndex; }
    uint32_t srcIndex() const { return m_srcIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
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
};

class TableFill : public ByteCode {
public:
    TableFill(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::TableFillOpcode, stackOffset)
        , m_tableIndex(index)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(TableFill);
    }
#endif

protected:
    uint32_t m_tableIndex;
};

class TableInit : public ByteCode {
public:
    TableInit(uint32_t stackOffset, uint32_t tableIndex, uint32_t segmentIndex)
        : ByteCode(OpcodeKind::TableInitOpcode, stackOffset)
        , m_tableIndex(tableIndex)
        , m_segmentIndex(segmentIndex)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    uint32_t segmentIndex() const { return m_segmentIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
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
    RefFunc(uint32_t stackOffset, uint32_t funcIndex)
        : ByteCode(OpcodeKind::RefFuncOpcode, stackOffset)
        , m_funcIndex(funcIndex)
    {
    }

    uint32_t funcIndex() const { return m_funcIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("funcIndex: %" PRIu32, m_funcIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(RefFunc);
    }
#endif

private:
    uint32_t m_funcIndex;
};

class RefNull : public ByteCode {
public:
    RefNull(uint32_t stackOffset, Value::Type type)
        : ByteCode(OpcodeKind::RefNullOpcode, stackOffset)
        , m_type(type)
    {
    }

    Value::Type type() const { return m_type; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
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
};

class RefIsNull : public ByteCode {
public:
    RefIsNull(uint32_t stackOffset)
        : ByteCode(OpcodeKind::RefIsNullOpcode, stackOffset)
    {
    }

#if !defined(NDEBUG)
    virtual size_t byteCodeSize()
    {
        return sizeof(RefIsNull);
    }
#endif
};

class GlobalGet4 : public ByteCode {
public:
    GlobalGet4(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalGet4Opcode, stackOffset)
        , m_index(index)
    {
    }

    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalGet4);
    }
#endif

protected:
    uint32_t m_index;
};

class GlobalGet8 : public ByteCode {
public:
    GlobalGet8(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalGet8Opcode, stackOffset)
        , m_index(index)
    {
    }

    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalGet8);
    }
#endif

protected:
    uint32_t m_index;
};

class GlobalSet4 : public ByteCode {
public:
    GlobalSet4(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalSet4Opcode, stackOffset)
        , m_index(index)
    {
    }

    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalSet4);
    }
#endif

protected:
    uint32_t m_index;
};

class GlobalSet8 : public ByteCode {
public:
    GlobalSet8(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::GlobalSet8Opcode, stackOffset)
        , m_index(index)
    {
    }

    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("index: %" PRId32,
               m_index);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(GlobalSet8);
    }
#endif

protected:
    uint32_t m_index;
};

class Throw : public ByteCode {
public:
    Throw(uint32_t stackOffset, uint32_t index)
        : ByteCode(OpcodeKind::ThrowOpcode, stackOffset)
        , m_tagIndex(index)
    {
    }

    uint32_t tagIndex() const { return m_tagIndex; }

#if !defined(NDEBUG)
    virtual void dump(size_t pos)
    {
        printf("tagIndex: %" PRId32,
               m_tagIndex);
    }

    virtual size_t byteCodeSize()
    {
        return sizeof(Throw);
    }
#endif

protected:
    uint32_t m_tagIndex;
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
    End(uint32_t stackOffset)
        : ByteCode(OpcodeKind::EndOpcode, stackOffset)
    {
    }

protected:
};

} // namespace Walrus

#endif // __WalrusByteCode__
