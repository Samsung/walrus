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

#ifndef __WalrusMemory__
#define __WalrusMemory__

#include "util/BitOperation.h"
#include "runtime/ExecutionState.h"
#include "runtime/Object.h"

namespace Walrus {

class Store;
class DataSegment;

class Memory : public Extern {
public:
    static const uint32_t s_memoryPageSize = 1024 * 64;

    // Caching memory target for fast access.
    struct TargetBuffer {
        TargetBuffer* prev;
        uint64_t sizeInByte;
        uint8_t* buffer;
    };

    static Memory* createMemory(Store* store, uint64_t initialSizeInByte, uint64_t maximumSizeInByte);

    ~Memory();

    virtual Object::Kind kind() const override
    {
        return Object::MemoryKind;
    }

    virtual bool isMemory() const override
    {
        return true;
    }

    uint8_t* buffer() const
    {
        return m_buffer;
    }

    uint64_t sizeInByte() const
    {
        return m_sizeInByte;
    }

    uint64_t sizeInPageSize() const
    {
        return sizeInByte() / s_memoryPageSize;
    }

    uint64_t maximumSizeInByte() const
    {
        return m_maximumSizeInByte;
    }

    uint64_t maximumSizeInPageSize() const
    {
        return m_maximumSizeInByte / s_memoryPageSize;
    }

    bool grow(uint64_t growSizeInByte);

    template <typename T>
    void load(ExecutionState& state, uint32_t offset, uint32_t addend, T* out) const
    {
        checkAccess(state, offset, sizeof(T), addend);

        memcpyEndianAware(out, m_buffer, sizeof(T), m_sizeInByte, 0, offset + addend, sizeof(T));
    }

    template <typename T>
    void load(ExecutionState& state, uint32_t offset, T* out) const
    {
        checkAccess(state, offset, sizeof(T));
#if defined(WALRUS_BIG_ENDIAN)
        *out = *(reinterpret_cast<T*>(&m_buffer[m_sizeInByte - sizeof(T) - offset]));
#else
        *out = *(reinterpret_cast<T*>(&m_buffer[offset]));
#endif
    }

#ifdef CPU_ARM32

#define defineUnalignedLoad(TYPE)                                                              \
    template <typename T = TYPE>                                                               \
    void load(ExecutionState& state, uint32_t offset, TYPE* out) const                         \
    {                                                                                          \
        checkAccess(state, offset, sizeof(TYPE));                                              \
        memcpyEndianAware(out, m_buffer, sizeof(TYPE), m_sizeInByte, 0, offset, sizeof(TYPE)); \
    }
    defineUnalignedLoad(uint64_t);
    defineUnalignedLoad(int64_t);
    defineUnalignedLoad(double);
#undef defineUnalignedLoad

#endif

    template <typename T>
    void store(ExecutionState& state, uint32_t offset, uint32_t addend, const T& val) const
    {
        checkAccess(state, offset, sizeof(T), addend);

        memcpyEndianAware(m_buffer, &val, m_sizeInByte, sizeof(T), offset + addend, 0, sizeof(T));
    }

    template <typename T>
    void store(ExecutionState& state, uint32_t offset, const T& val) const
    {
        checkAccess(state, offset, sizeof(T));
#if defined(WALRUS_BIG_ENDIAN)
        *(reinterpret_cast<T*>(&m_buffer[m_sizeInByte - sizeof(T) - offset])) = val;
#else
        *(reinterpret_cast<T*>(&m_buffer[offset])) = val;
#endif
    }

#ifdef CPU_ARM32

#define defineUnalignedStore(TYPE)                                                              \
    template <typename T = TYPE>                                                                \
    void store(ExecutionState& state, uint32_t offset, const TYPE& val) const                   \
    {                                                                                           \
        checkAccess(state, offset, sizeof(TYPE));                                               \
        memcpyEndianAware(m_buffer, &val, m_sizeInByte, sizeof(TYPE), offset, 0, sizeof(TYPE)); \
    }
    defineUnalignedStore(uint64_t);
    defineUnalignedStore(int64_t);
    defineUnalignedStore(double);
#undef defineUnalignedStore

#endif

    void init(ExecutionState& state, DataSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize);
    void copy(ExecutionState& state, uint32_t dstStart, uint32_t srcStart, uint32_t size);
    void fill(ExecutionState& state, uint32_t start, uint8_t value, uint32_t size);

    inline void push(TargetBuffer* targetBuffer)
    {
        targetBuffer->prev = m_targetBuffers;
        targetBuffer->sizeInByte = sizeInByte();
        targetBuffer->buffer = buffer();
        m_targetBuffers = targetBuffer;
    }

    inline void pop(TargetBuffer* targetBuffer)
    {
        m_targetBuffers = targetBuffer->prev;
    }

    inline bool checkAccess(uint32_t offset, uint32_t size, uint32_t addend = 0) const
    {
        return !UNLIKELY(!((uint64_t)offset + (uint64_t)addend + (uint64_t)size <= m_sizeInByte));
    }

    void initMemory(DataSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize);
    void copyMemory(uint32_t dstStart, uint32_t srcStart, uint32_t size);
    void fillMemory(uint32_t start, uint8_t value, uint32_t size);

private:
    Memory(uint64_t initialSizeInByte, uint64_t maximumSizeInByte);

    void throwException(ExecutionState& state, uint32_t offset, uint32_t addend, uint32_t size) const;
    inline void checkAccess(ExecutionState& state, uint32_t offset, uint32_t size, uint32_t addend = 0) const
    {
        if (!this->checkAccess(offset, size, addend)) {
            throwException(state, offset, addend, size);
        }
    }

    uint64_t m_sizeInByte;
    uint64_t m_reservedSizeInByte;
    uint64_t m_maximumSizeInByte;
    uint8_t* m_buffer;
    TargetBuffer* m_targetBuffers;
};

} // namespace Walrus

#endif // __WalrusMemory__
