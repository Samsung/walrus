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
#include "runtime/Trap.h"
#include "runtime/Instance.h"
#include "runtime/Module.h"

namespace Walrus {

Memory::Memory(uint32_t initialSizeInByte, uint32_t maximumSizeInByte)
    : m_sizeInByte(initialSizeInByte)
    , m_maximumSizeInByte(maximumSizeInByte)
    , m_buffer(reinterpret_cast<uint8_t*>(calloc(1, initialSizeInByte)))
{
    RELEASE_ASSERT(m_buffer);
    GC_REGISTER_FINALIZER_NO_ORDER(this, [](void* obj, void* cd) {
        free(reinterpret_cast<Memory*>(obj)->m_buffer);
    },
                                   nullptr, nullptr, nullptr);
}

bool Memory::grow(uint64_t growSizeInByte)
{
    uint64_t newSizeInByte = growSizeInByte + m_sizeInByte;
    if (newSizeInByte > m_sizeInByte && newSizeInByte <= m_maximumSizeInByte) {
        uint8_t* newBuffer = reinterpret_cast<uint8_t*>(calloc(1, newSizeInByte));
        if (newBuffer) {
            memcpy(newBuffer, m_buffer, m_sizeInByte);
            free(m_buffer);
            m_buffer = newBuffer;
            m_sizeInByte = newSizeInByte;
            return true;
        }
    } else if (newSizeInByte == m_sizeInByte) {
        return true;
    }
    return false;
}

void Memory::throwException(uint32_t offset, uint32_t addend, uint32_t size) const
{
    std::string str = "out of bounds memory access: access at ";
    str += std::to_string(offset + addend);
    str += "+";
    str += std::to_string(size);
    Trap::throwException(str.data(), str.length());
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

void Memory::init(DataSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    checkAccess(dstStart, 0, srcSize);
    if (srcStart >= source->sizeInByte() || srcStart + srcSize > source->sizeInByte()) {
        throwException(srcStart, srcStart + srcSize, srcSize);
    }

    auto data = source->data()->initData();
    std::copy(source->data()->initData().begin() + srcStart, source->data()->initData().begin() + srcStart + srcSize,
#if defined(WALRUS_BIG_ENDIAN)
        ReverseArrayIterator(m_buffer + m_sizeInByte - 1)
#else
              m_buffer + dstStart);
#endif
}

void Memory::copy(uint32_t dstStart, uint32_t srcStart, uint32_t size)
{
    checkAccess(srcStart, 0, size);
    checkAccess(dstStart, 0, size);

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

void Memory::fill(uint32_t start, uint8_t value, uint32_t size)
{
    checkAccess(start, 0, size);
#if defined(WALRUS_BIG_ENDIAN)
    std::fill(m_buffer + m_sizeInByte - start - size, m_buffer + m_sizeInByte - start, value);
#else
    std::fill(m_buffer + start, m_buffer + start + size, value);
#endif
}

} // namespace Walrus
