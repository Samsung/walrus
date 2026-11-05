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

#include "runtime/ExecutionState.h"
#include "util/Vector.h"

#include <atomic>

namespace Walrus {

class Tag;
#ifdef ENABLE_GC
class GCException;
#endif /* ENABLE_GC */

// The payload of a thrown exception may hold references. It must live where the
// collector scans it, for as long as the exception does.
class ExceptionData {
public:
    ExceptionData()
        : m_data(nullptr)
    {
    }

    ExceptionData(ExceptionData&& other)
        : m_data(other.m_data)
    {
        other.m_data = nullptr;
    }

    ExceptionData(size_t size);
    ~ExceptionData();

    uint8_t* data() const { return m_data; }

private:
    uint8_t* m_data;
};

class Exception {
public:
#ifndef NDEBUG
    // count the total number of created Extern objects
    static std::atomic<size_t> g_exceptionCount;
#endif

    static Exception* create(const std::string& m)
    {
        return new Exception(m);
    }

    static Exception* create(Tag* tag, ExceptionData&& userExceptionData)
    {
        return new Exception(tag, std::move(userExceptionData));
    }

    bool isBuiltinException()
    {
        return !m_message.empty();
    }

    bool isUserException()
    {
        return !!tag();
    }

    void addRef()
    {
        m_refCount++;
    }

    void releaseRef()
    {
        if (--m_refCount == 0) {
#ifndef NDEBUG
            g_exceptionCount--;
#endif
            delete this;
        }
    }

    std::string& message()
    {
        return m_message;
    }

    Optional<Tag*> tag() const
    {
        return m_tag;
    }

    const ExceptionData& userExceptionData() const
    {
        return m_userExceptionData;
    }

private:
    friend class Interpreter;
#ifdef ENABLE_GC
    friend class GCException;
#endif /* ENABLE_GC */

    Exception(const std::string& message)
        : m_refCount(1)
        , m_message(message)
#ifdef ENABLE_GC
        , m_exceptionRef(nullptr)
#endif /* ENABLE_GC */
    {
#ifndef NDEBUG
        g_exceptionCount++;
#endif
    }

    Exception(Tag* tag, ExceptionData&& userExceptionData)
        : m_refCount(1)
        , m_tag(tag)
        , m_userExceptionData(std::move(userExceptionData))
#ifdef ENABLE_GC
        , m_exceptionRef(nullptr)
#endif /* ENABLE_GC */
    {
#ifndef NDEBUG
        g_exceptionCount++;
#endif
    }

    size_t m_refCount;
    std::string m_message;
    Optional<Tag*> m_tag;
    ExceptionData m_userExceptionData;
#ifdef ENABLE_GC
    GCException* m_exceptionRef;
#endif /* ENABLE_GC */
};

} // namespace Walrus

#endif // __WalrusException__
