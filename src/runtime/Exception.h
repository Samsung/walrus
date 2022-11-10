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

#ifndef __WalrusException__
#define __WalrusException__

namespace Walrus {

class String;
class Tag;

class Exception : public gc {
public:
    // we should use exception value as NoGC since bdwgc cannot find thrown value
    static std::unique_ptr<Exception> create(String* m)
    {
        return std::unique_ptr<Exception>(new (NoGC) Exception(m));
    }

    static std::unique_ptr<Exception> create(Tag* tag, Vector<uint8_t, GCUtil::gc_malloc_allocator<uint8_t>>&& userExceptionData)
    {
        return std::unique_ptr<Exception>(new (NoGC) Exception(tag, std::move(userExceptionData)));
    }

    bool isBuiltinException()
    {
        return !!message();
    }

    bool isUserException()
    {
        return !!tag();
    }

    Optional<String*> message() const
    {
        return m_message;
    }

    Optional<Tag*> tag() const
    {
        return m_tag;
    }

    const Vector<uint8_t, GCUtil::gc_malloc_allocator<uint8_t>>& userExceptionData() const
    {
        return m_userExceptionData;
    }

private:
    Exception(String* message)
        : m_message(message)
    {
    }

    Exception(Tag* tag, Vector<uint8_t, GCUtil::gc_malloc_allocator<uint8_t>>&& userExceptionData)
        : m_tag(tag)
        , m_userExceptionData(std::move(userExceptionData))
    {
    }

    Optional<String*> m_message;
    Optional<Tag*> m_tag;
    Vector<uint8_t, GCUtil::gc_malloc_allocator<uint8_t>> m_userExceptionData;
};

} // namespace Walrus

#endif // __WalrusException__
