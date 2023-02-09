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

#include "jit/Allocator.h"

namespace Walrus {

ValueInfo LocationInfo::typeToValueInfo(Value::Type type)
{
    switch (type) {
    case Value::Type::I32:
        return kFourByteSize;
    case Value::Type::I64:
        return kEightByteSize;
    case Value::Type::F32:
        return kFloat | kFourByteSize;
    case Value::Type::F64:
        return kFloat | kEightByteSize;
    case Value::Type::V128:
        return kSixteenByteSize;
    case Value::Type::FuncRef:
    case Value::Type::ExternRef:
        return kReference | (sizeof(void*) == 8 ? kEightByteSize : kFourByteSize);
    default:
        WABT_UNREACHABLE;
    }
    return 0;
}

StackAllocator::StackAllocator(StackAllocator* other, Index end)
    : m_values(other->m_values.begin(), other->m_values.begin() + end)
    , m_skipStart(other->m_skipStart)
    , m_skipEnd(other->m_skipEnd)
{
    while (end > 0) {
        --end;
        LocationInfo& info = m_values[end];
        if (info.status & LocationInfo::kIsOffset) {
            m_size = info.value + LocationInfo::length(info.valueInfo);
            return;
        }
    }
}

void StackAllocator::push(ValueInfo valueInfo)
{
    size_t mask;

    switch (valueInfo & LocationInfo::kSizeMask) {
    case LocationInfo::kFourByteSize:
        mask = sizeof(uint32_t) - 1;
        break;
    case LocationInfo::kEightByteSize:
        mask = sizeof(uint64_t) - 1;
        break;
    default:
        assert((valueInfo & LocationInfo::kSizeMask) == LocationInfo::kSixteenByteSize);
        mask = sizeof(v128) - 1;
        break;
    }

    m_size = (m_size + mask) & ~mask;

    if (m_size >= m_skipStart && m_size < m_skipEnd) {
        m_size = (m_skipEnd + mask) & ~mask;
    }

    m_values.push_back(LocationInfo(m_size, LocationInfo::kIsOffset, valueInfo));
    m_size += (mask + 1);
}

void StackAllocator::pop()
{
    assert(m_values.size() > 0);

    uint8_t status = m_values.back().status;

    m_values.pop_back();

    if (!(status & LocationInfo::kIsOffset)) {
        return;
    }

    for (size_t i = m_values.size(); i > 0; --i) {
        LocationInfo& location = m_values[i - 1];

        if (location.status & LocationInfo::kIsOffset) {
            m_size = location.value + LocationInfo::length(location.valueInfo);
            return;
        }
    }

    m_size = 0;
}

void StackAllocator::skipRange(Index start, Index end)
{
    assert(m_skipStart == 0 && m_skipEnd == 0);
    assert(start <= end && m_size <= start);

    if (start < end) {
        m_skipStart = start;
        m_skipEnd = end;
    }

    m_size = 0;
    m_values.clear();
}

void LocalsAllocator::allocate(ValueInfo valueInfo)
{
    Index offset;

    // Allocate value at the end unless there is a free
    // space for the value. Allocating large values might
    // create free space for smaller values.
    switch (valueInfo & LocationInfo::kSizeMask) {
    case LocationInfo::kFourByteSize:
        if (m_unusedFourByteEnd != 0) {
            offset = m_unusedFourByteEnd - sizeof(uint32_t);
            m_unusedFourByteEnd = 0;
            break;
        }

        if (m_unusedEightByteEnd != 0) {
            offset = m_unusedEightByteEnd - sizeof(uint64_t);
            m_unusedFourByteEnd = m_unusedEightByteEnd;
            m_unusedEightByteEnd = 0;
            break;
        }

        offset = m_size;
        m_size += sizeof(uint32_t);
        break;
    case LocationInfo::kEightByteSize:
        if (m_unusedEightByteEnd != 0) {
            offset = m_unusedEightByteEnd - sizeof(uint64_t);
            m_unusedEightByteEnd = 0;
            break;
        }

        if (m_size & sizeof(uint32_t)) {
            // When a four byte space is allocated at the end,
            // it means there was no unused 4 byte available.
            assert(m_unusedFourByteEnd == 0);
            m_size += sizeof(uint32_t);
            m_unusedFourByteEnd = m_size;
        }

        offset = m_size;
        m_size += sizeof(uint64_t);
        break;
    default:
        assert((valueInfo & LocationInfo::kSizeMask) == LocationInfo::kSixteenByteSize);
        if (m_size & (sizeof(v128) - 1)) {
            // Four byte alignment checked first
            // to ensure eight byte alignment.
            if (m_size & sizeof(uint32_t)) {
                assert(m_unusedFourByteEnd == 0);
                m_size += sizeof(uint32_t);
                m_unusedFourByteEnd = m_size;
            }

            if (m_size & sizeof(uint64_t)) {
                assert(m_unusedEightByteEnd == 0);
                m_size += sizeof(uint64_t);
                m_unusedEightByteEnd = m_size;
            }
        }

        offset = m_size;
        m_size += sizeof(v128);
        break;
    }

    m_values.push_back(LocationInfo(offset, LocationInfo::kIsOffset, valueInfo));
}

} // namespace Walrus
