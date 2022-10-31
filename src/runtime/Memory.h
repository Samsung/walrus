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

namespace Walrus {

class Memory : public gc {
public:
    Memory(size_t initialSizeInByte, size_t maximumSizeInByte = std::numeric_limits<size_t>::max());
    static const size_t s_memoryPageSize = 1024 * 64;
    uint8_t* buffer() const
    {
        return m_buffer;
    }

    size_t sizeInByte() const
    {
        return m_sizeInByte;
    }

    size_t sizeInPageSize() const
    {
        return sizeInByte() / s_memoryPageSize;
    }

    size_t maximumSizeInByte() const
    {
        return m_maximumSizeInByte;
    }

    bool grow(size_t growSizeInByte);

private:
    size_t m_sizeInByte;
    size_t m_maximumSizeInByte;
    uint8_t* m_buffer;
};

} // namespace Walrus

#endif // __WalrusInstance__
