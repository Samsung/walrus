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

#ifndef __WalrusString__
#define __WalrusString__

namespace Walrus {

class String : public gc {
public:
    String(const char* buffer, size_t length)
    {
        m_buffer = reinterpret_cast<char*>(GC_MALLOC_ATOMIC(length));
        memcpy(m_buffer, buffer, length);
        m_length = length;
    }

    String(const std::string& src)
        : String(src.data(), src.length())
    {
    }

    const char* buffer() const { return m_buffer; }

    size_t length() const { return m_length; }

    char charAt(size_t index) const { return m_buffer[index]; }

    bool equals(String* src) const
    {
        if (src == this) {
            return true;
        }

        if (src->m_length != m_length) {
            return false;
        }

        return memcmp(m_buffer, src->m_buffer, m_length) == 0;
    }

    template <const size_t srcLen>
    bool equals(const char (&src)[srcLen]) const
    {
        return equals(src, srcLen - 1);
    }

    bool equals(const char* src, size_t srcLen) const
    {
        if (srcLen != m_length) {
            return false;
        }

        return memcmp(m_buffer, src, m_length) == 0;
    }

    void* operator new(size_t size);

private:
    char* m_buffer;
    size_t m_length;
};

} // namespace Walrus

#endif
