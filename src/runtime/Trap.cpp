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

#include "Trap.h"

namespace Walrus {

Trap::TrapResult Trap::run(void (*runner)(ExecutionState&, void*), void* data)
{
    Trap::TrapResult r;
    try {
        ExecutionState state;
        runner(state, data);
    } catch (std::unique_ptr<Exception>& e) {
        r.exception = std::move(e);
    }

    return r;
}

void Trap::throwException(const char* message, size_t len)
{
    throwException(new String(message, len));
}

void Trap::throwException(String* message)
{
    throw Exception::create(message);
}

void Trap::throwException(Tag* tag, Vector<uint8_t, GCUtil::gc_malloc_allocator<uint8_t>>&& userExceptionData)
{
    throw Exception::create(tag, std::move(userExceptionData));
}

} // namespace Walrus
