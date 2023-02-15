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

#ifndef __WalrusJITAllocator__
#define __WalrusJITAllocator__

#include "wabt/common.h"
#include "runtime/Value.h"

namespace Walrus {

// Bitset which describes type related data.
typedef uint8_t ValueInfo;

typedef wabt::Index Index;

// Describes the location of a value on the stack.
struct LocationInfo {
    // Unspecified or auto-detected value.
    static const ValueInfo kNone = 0;
    // Size bits.
    static const ValueInfo kFourByteSize = 1;
    static const ValueInfo kEightByteSize = 2;
    static const ValueInfo kSixteenByteSize = 3;
    static const ValueInfo kSizeMask = 0x3;
    // Type information bits.
    static const ValueInfo kFloat = 1 << 2;
    static const ValueInfo kReference = 1 << 3;

    // Status bits
    static const uint8_t kIsOffset = 1 << 0;
    static const uint8_t kIsLocal = 1 << 1;
    static const uint8_t kIsRegister = 1 << 2;
    static const uint8_t kIsCallArg = 1 << 3;
    static const uint8_t kIsImmediate = 1 << 4;
    static const uint8_t kIsUnused = 1 << 5;

    LocationInfo(Index value, uint8_t status, ValueInfo valueInfo)
        : value(value)
        , status(status)
        , valueInfo(valueInfo)
    {
    }

    static ValueInfo typeToValueInfo(Value::Type type);

    static Index length(ValueInfo valueInfo)
    {
        return 0x2u << (valueInfo & kSizeMask);
    }

    // Possible values of value:
    //  offset - when status has kIsOffset bit set
    //  non-zero - register index
    //  0 - immediate or unused value
    Index value;
    uint8_t status;
    ValueInfo valueInfo;
};

class StackAllocator {
public:
    static const Index alignment = sizeof(v128) - 1;

    StackAllocator() {}
    StackAllocator(StackAllocator* other, Index end);

    void push(ValueInfo valueInfo);
    void pushLocal(Index local, ValueInfo valueInfo)
    {
        m_values.push_back(LocationInfo(local, LocationInfo::kIsLocal, valueInfo));
    }
    void pushReg(Index reg, ValueInfo valueInfo)
    {
        m_values.push_back(LocationInfo(reg, LocationInfo::kIsRegister, valueInfo));
    }
    void pushCallArg(Index offset, ValueInfo valueInfo)
    {
        m_values.push_back(LocationInfo(offset, LocationInfo::kIsCallArg, valueInfo));
    }
    void pushImmediate(ValueInfo valueInfo)
    {
        m_values.push_back(LocationInfo(0, LocationInfo::kIsImmediate, valueInfo));
    }
    void pushUnused(ValueInfo valueInfo)
    {
        m_values.push_back(LocationInfo(0, LocationInfo::kIsUnused, valueInfo));
    }
    void pop();

    const LocationInfo& get(size_t index) { return m_values[index]; }
    const LocationInfo& last() { return m_values.back(); }
    std::vector<LocationInfo>& values() { return m_values; }
    bool empty() { return m_values.empty(); }
    Index size() { return m_size; }

    // Also resets the allocator.
    void skipRange(Index start, Index end);

    static Index alignedSize(Index size)
    {
        return (size + (alignment - 1)) & ~(alignment - 1);
    }

private:
    std::vector<LocationInfo> m_values;
    Index m_size = 0;
    // Values cannot be allocated in the skipped region
    Index m_skipStart = 0;
    Index m_skipEnd = 0;
};

class LocalsAllocator {
public:
    static const Index alignment = StackAllocator::alignment;

    LocalsAllocator(Index start)
        : m_size(start)
    {
    }

    void allocate(ValueInfo valueInfo);

    const LocationInfo& get(size_t index) { return m_values[index]; }
    const LocationInfo& last() { return m_values.back(); }
    Index size() { return m_size; }

private:
    std::vector<LocationInfo> m_values;
    Index m_size;
    // Due to the allocation algorithm, at most one 4
    // and one 8 byte space can be free at any time.
    Index m_unusedFourByteEnd = 0;
    Index m_unusedEightByteEnd = 0;
};

} // namespace Walrus

#endif // __WalrusJITAllocator__
