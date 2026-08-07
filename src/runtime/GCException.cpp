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

#include "GCException.h"
#include "runtime/Exception.h"
#include "runtime/Trap.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(exceptionTypeInfo, ExceptionKind);

#ifdef ENABLE_GC
static void GC_CALLBACK exceptionFinalizer(void* ptr, void* /* ignored ptr */)
{
    reinterpret_cast<GCException*>(ptr)->exception() = nullptr;
}
#endif // ENABLE_GC

GCException* GCException::exceptionNew(std::unique_ptr<Exception>& e)
{
#ifdef ENABLE_GC
    // TODO: The object is currently stored on the stack, which is good enough for testing,
    // but several GC related improvements needs to be added to the code later.
    GCException* result = reinterpret_cast<GCException*>(GC_MALLOC(sizeof(GCException)));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    new (result) GCException(e);

    GC_REGISTER_FINALIZER_NO_ORDER(result, exceptionFinalizer, nullptr, nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

GCException::GCException(std::unique_ptr<Exception>& e)
    : GCBase(GET_GLOBAL_TYPE_INFO(exceptionTypeInfo))
    , m_exception(std::move(e))
{
}

void GCException::throwException()
{
    if (m_exception == nullptr) {
        // Currently an engine limitation.
        Trap::throwException("Exception has been thrown");
    }

    throw std::move(m_exception);
}

} // namespace Walrus
