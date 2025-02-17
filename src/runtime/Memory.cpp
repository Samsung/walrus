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
#include "Walrus.h"

#include "Memory.h"
#include "Store.h"
#include "runtime/Trap.h"
#include "runtime/Instance.h"
#include "runtime/Module.h"

#if defined(OS_POSIX)
#define WALRUS_USE_MMAP
#include <sys/mman.h>
#endif

namespace Walrus {

Memory* Memory::createMemory(Store* store, uint64_t initialSizeInByte, uint64_t maximumSizeInByte, bool isShared)
{
    Memory* mem = new Memory(initialSizeInByte, maximumSizeInByte, isShared);
    store->appendExtern(mem);
    return mem;
}

Memory::Memory(uint64_t initialSizeInByte, uint64_t maximumSizeInByte, bool isShared)
    : m_sizeInByte(initialSizeInByte)
    , m_reservedSizeInByte(0)
    , m_maximumSizeInByte(maximumSizeInByte)
    , m_buffer(nullptr)
    , m_targetBuffers(nullptr)
    , m_isShared(isShared)
{
    RELEASE_ASSERT(initialSizeInByte <= std::numeric_limits<size_t>::max());
#if defined(WALRUS_USE_MMAP)
    if (m_maximumSizeInByte) {
#ifndef WALRUS_32_MEMORY_INITIAL_MMAP_RESERVED_ADDRESS_SIZE
#define WALRUS_32_MEMORY_INITIAL_MMAP_RESERVED_ADDRESS_SIZE (1024 * 1024 * 64)
#endif
#ifndef WALRUS_64_MEMORY_INITIAL_MMAP_RESERVED_ADDRESS_SIZE
#define WALRUS_64_MEMORY_INITIAL_MMAP_RESERVED_ADDRESS_SIZE (1024 * 1024 * 512)
#endif
        uint64_t initialReservedSize =
#if defined(WALRUS_32)
            WALRUS_32_MEMORY_INITIAL_MMAP_RESERVED_ADDRESS_SIZE;
#else
            WALRUS_64_MEMORY_INITIAL_MMAP_RESERVED_ADDRESS_SIZE;
#endif
        m_reservedSizeInByte = std::min(std::max(initialReservedSize, initialSizeInByte), m_maximumSizeInByte);
        m_buffer = reinterpret_cast<uint8_t*>(mmap(NULL, m_reservedSizeInByte, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        RELEASE_ASSERT(MAP_FAILED != m_buffer);
        mprotect(m_buffer, initialSizeInByte, (PROT_READ | PROT_WRITE));
    } else {
        m_reservedSizeInByte = 0;
        m_buffer = nullptr;
    }
#else
    m_buffer = reinterpret_cast<uint8_t*>(calloc(1, initialSizeInByte));
    m_reservedSizeInByte = initialSizeInByte;
    RELEASE_ASSERT(m_buffer);
#endif
}

Memory::~Memory()
{
#if defined(WALRUS_USE_MMAP)
    if (m_buffer) {
        munmap(m_buffer, m_reservedSizeInByte);
    }
#else
    ASSERT(!!m_buffer);
    free(m_buffer);
#endif
}

bool Memory::grow(uint64_t growSizeInByte)
{
    uint64_t newSizeInByte = growSizeInByte + m_sizeInByte;
    if (newSizeInByte > m_sizeInByte && newSizeInByte <= m_maximumSizeInByte) {
#if defined(WALRUS_USE_MMAP)
        if (newSizeInByte <= m_reservedSizeInByte) {
            mprotect(m_buffer + m_sizeInByte, growSizeInByte, (PROT_READ | PROT_WRITE));
            m_sizeInByte = newSizeInByte;
        } else {
            auto newReservedSizeInByte = std::min(newSizeInByte * 2, m_maximumSizeInByte);
            auto newBuffer = reinterpret_cast<uint8_t*>(mmap(NULL, newReservedSizeInByte, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
            if (MAP_FAILED == newBuffer) {
                return false;
            }
            mprotect(newBuffer, newSizeInByte, (PROT_READ | PROT_WRITE));
            memcpy(newBuffer, m_buffer, m_sizeInByte);
            munmap(m_buffer, m_reservedSizeInByte);
            m_buffer = newBuffer;
            m_sizeInByte = newSizeInByte;
            m_reservedSizeInByte = newReservedSizeInByte;
        }
#else
        uint8_t* newBuffer = reinterpret_cast<uint8_t*>(calloc(1, newSizeInByte));
        if (newBuffer == nullptr || newSizeInByte >= std::numeric_limits<size_t>::max()) {
            return false;
        }
        m_reservedSizeInByte = newSizeInByte;
        memcpy(newBuffer, m_buffer, m_sizeInByte);
        free(m_buffer);
        m_buffer = newBuffer;
        m_sizeInByte = newSizeInByte;
#endif

        TargetBuffer* targetBuffer = m_targetBuffers;

        while (targetBuffer != nullptr) {
            targetBuffer->sizeInByte = sizeInByte();
            targetBuffer->buffer = buffer();
            targetBuffer = targetBuffer->next;
        }
        return true;
    } else if (newSizeInByte == m_sizeInByte) {
        return true;
    }
    return false;
}

void Memory::throwRangeException(ExecutionState& state, uint32_t offset, uint32_t addend, uint32_t size) const
{
    std::string str = "out of bounds memory access: access at ";
    str += std::to_string(offset + addend);
    str += "+";
    str += std::to_string(size);
    Trap::throwException(state, str);
}

template <class T>
class ReverseArrayIterator {
public:
    ReverseArrayIterator(T* ptr)
        : _ptr(ptr)
    {
    }
    operator T*()
    {
        return _ptr;
    }
    void operator++()
    {
        --_ptr;
    }
    T operator*()
    {
        return *_ptr;
    }
    bool operator!=(ReverseArrayIterator& rhs)
    {
        return _ptr != rhs._ptr;
    }

private:
    T* _ptr;
};

void Memory::init(ExecutionState& state, DataSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    checkAccess(state, dstStart, srcSize);

    if (srcStart >= source->sizeInByte() || srcStart + srcSize > source->sizeInByte()) {
        throwRangeException(state, srcStart, srcStart + srcSize, srcSize);
    }

    this->initMemory(source, dstStart, srcStart, srcSize);
}

void Memory::copy(ExecutionState& state, uint32_t dstStart, uint32_t srcStart, uint32_t size, Memory* dstMem)
{
    checkAccess(state, srcStart, size);
    checkAccess(state, dstStart, size, 0, dstMem);

    if (dstMem == nullptr) {
        this->copyMemory(dstStart, srcStart, size);
    } else {
        this->copyMemoryMulti(dstMem, dstStart, srcStart, size);
    }
}

void Memory::fill(ExecutionState& state, uint32_t start, uint8_t value, uint32_t size)
{
    checkAccess(state, start, size);

    this->fillMemory(start, value, size);
}

void Memory::initMemory(DataSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    auto data = source->data()->initData();
    std::copy(source->data()->initData().begin() + srcStart, source->data()->initData().begin() + srcStart + srcSize,
#if defined(WALRUS_BIG_ENDIAN)
        ReverseArrayIterator(m_buffer + m_sizeInByte - 1)
#else
              m_buffer + dstStart);
#endif
}

void Memory::copyMemory(uint32_t dstStart, uint32_t srcStart, uint32_t size)
{
#if defined(WALRUS_BIG_ENDIAN)
    auto srcBegin = m_buffer + m_sizeInByte + srcStart - size;
    auto dstBegin = m_buffer + m_sizeInByte + dstStart - size;
#else
    auto srcBegin = m_buffer + srcStart;
    auto dstBegin = m_buffer + dstStart;
#endif
    auto srcEnd = srcBegin + size;
    auto dstEnd = dstBegin + size;
    if (srcBegin < dstBegin) {
        std::move_backward(srcBegin, srcEnd, dstEnd);
    } else {
        std::move(srcBegin, srcEnd, dstBegin);
    }
}

void Memory::copyMemoryMulti(Memory* dstMemory, uint32_t dstStart, uint32_t srcStart, uint32_t size)
{
#if defined(WALRUS_BIG_ENDIAN)
    auto srcBegin = m_buffer + m_sizeInByte + srcStart - size;
    auto dstBegin = dstMemory->m_buffer + dstMemory->m_sizeInByte + dstStart - size;
#else
    auto srcBegin = m_buffer + srcStart;
    auto dstBegin = dstMemory->m_buffer + dstStart;
#endif
    auto srcEnd = srcBegin + size;
    auto dstEnd = dstBegin + size;
    if (srcBegin < dstBegin) {
        std::move_backward(srcBegin, srcEnd, dstEnd);
    } else {
        std::move(srcBegin, srcEnd, dstBegin);
    }
}

void Memory::fillMemory(uint32_t start, uint8_t value, uint32_t size)
{
#if defined(WALRUS_BIG_ENDIAN)
    std::fill(m_buffer + m_sizeInByte - start - size, m_buffer + m_sizeInByte - start, value);
#else
    std::fill(m_buffer + start, m_buffer + start + size, value);
#endif
}

void Memory::TargetBuffer::enque(Memory* memory)
{
    next = memory->m_targetBuffers;
    buffer = memory->buffer();
    sizeInByte = memory->sizeInByte();

    memory->m_targetBuffers = this;
}

void Memory::TargetBuffer::deque(Memory* memory)
{
    // Cache is not initialized.
    if (sizeInByte == ~(uint64_t)0) {
        return;
    }

    TargetBuffer* current = memory->m_targetBuffers;

    if (current == this) {
        memory->m_targetBuffers = next;
        return;
    }

    while (true) {
        ASSERT(current != nullptr && current->next != nullptr);

        if (current->next == this) {
            current->next = next;
            return;
        }

        current = current->next;
    }
}

void Memory::checkAtomicAccess(ExecutionState& state, uint32_t offset, uint32_t size, uint32_t addend) const
{
    checkAccess(state, offset, size, addend);
    if (UNLIKELY((offset + addend) % size != 0)) {
        Trap::throwException(state, "unaligned atomic");
    }
}

void Memory::throwUnsharedMemoryException(ExecutionState& state) const
{
    Trap::throwException(state, "expected shared memory");
}
} // namespace Walrus
