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

#ifndef __WalrusTrap__
#define __WalrusTrap__

#include "runtime/Value.h"
#include "runtime/Object.h"
#include "runtime/Exception.h"
#include "runtime/ExecutionState.h"

namespace Walrus {

class Exception;
class Module;
class Tag;

class Trap : public Object {
public:
    struct TrapResult {
        std::unique_ptr<Exception> exception;

        TrapResult()
        {
        }
    };

    virtual Object::Kind kind() const override
    {
        return Object::TrapKind;
    }

    virtual bool isTrap() const override
    {
        return true;
    }

    TrapResult run(void (*runner)(ExecutionState&, void*), void* data);
    static void throwException(ExecutionState& state, const char* message, size_t len);
    template <const size_t srcLen>
    static void throwException(ExecutionState& state, const char (&src)[srcLen])
    {
        throwException(state, src, srcLen - 1);
    }
    static void throwException(ExecutionState& state, String* message);
    static void throwException(ExecutionState& state, const std::string& src)
    {
        throwException(state, src.data(), src.length());
    }
    static void throwException(ExecutionState& state, Tag* tag, Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>>&& userExceptionData);
    static void throwException(ExecutionState& state, std::unique_ptr<Exception>&& e);

private:
};

} // namespace Walrus

#endif // __WalrusTrap__
