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

#ifndef __WalrusExecutionState__
#define __WalrusExecutionState__

#include "util/Optional.h"
#include "util/Util.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

class Function;

class ExecutionState {
public:
    friend class Store;
    friend class Exception;
    friend class Trap;
    friend class Interpreter;

    ExecutionState(const ExecutionState&) = delete;
    ExecutionState& operator=(const ExecutionState&) = delete;

    ExecutionState(ExecutionState& parent)
        : m_parent(&parent)
        , m_currentFunction(nullptr)
        , m_stackLimit(parent.m_stackLimit)
        , m_bp(nullptr)
        , m_capacity(0)
        , m_owned(nullptr)
    {
    }

    ExecutionState(ExecutionState& parent, Function* currentFunction)
        : m_parent(&parent)
        , m_currentFunction(currentFunction)
        , m_stackLimit(parent.m_stackLimit)
        , m_bp(nullptr)
        , m_capacity(0)
        , m_owned(nullptr)
    {
    }

    ExecutionState(ExecutionState& parent, Function* currentFunction, uint8_t* bp, size_t capacity, uint8_t* owned)
        : m_parent(&parent)
        , m_currentFunction(currentFunction)
        , m_stackLimit(parent.m_stackLimit)
        , m_bp(bp)
        , m_capacity(capacity)
        , m_owned(owned)
    {
        ASSERT(owned == nullptr || owned == bp);
        if (m_owned != nullptr) {
            m_previousOwnedFrame = s_ownedFrames;
            s_ownedFrames = this;
        }
    }

    ~ExecutionState()
    {
        if (m_owned != nullptr) {
            ASSERT(s_ownedFrames == this);
            s_ownedFrames = m_previousOwnedFrame;
            deallocateBuffer(m_owned);
        }
    }

    Function* currentFunction() const
    {
        return m_currentFunction;
    }

    uint8_t* bp() const
    {
        return m_bp;
    }

    size_t capacity() const
    {
        return m_capacity;
    }

    size_t stackLimit() const
    {
        return m_stackLimit;
    }

    static uint8_t* allocateBuffer(size_t size)
    {
#ifdef ENABLE_GC
        return reinterpret_cast<uint8_t*>(GC_MALLOC_UNCOLLECTABLE(size));
#else
        return reinterpret_cast<uint8_t*>(malloc(size));
#endif
    }

    void replaceBuffer(uint8_t* buffer, size_t capacity)
    {
        ASSERT(capacity > m_capacity);
        if (m_owned != nullptr) {
            deallocateBuffer(m_owned);
        } else {
            m_previousOwnedFrame = s_ownedFrames;
            s_ownedFrames = this;
        }
        m_owned = buffer;
        m_bp = m_owned;
        m_capacity = capacity;
    }

    void replaceCurrentFunction(Function* function)
    {
        m_currentFunction = function;
    }

private:
    friend class ByteCodeTable;
    ExecutionState()
        : m_parent(nullptr)
        , m_currentFunction(nullptr)
        , m_bp(nullptr)
        , m_capacity(0)
        , m_owned(nullptr)
    {
        m_stackLimit = (size_t)currentStackPointer();

        // Keep the limit at least STACK_LIMIT_FROM_END inside the current
        // thread's stack so that the check fires on small-stacked threads.
#ifdef STACK_GROWS_DOWN
        m_stackLimit = std::max(m_stackLimit - STACK_LIMIT_FROM_BASE, threadStackBound());
#else
        m_stackLimit = std::min(m_stackLimit + STACK_LIMIT_FROM_BASE, threadStackBound());
#endif
    }

    // The address STACK_LIMIT_FROM_END inside the end of the current thread's
    // stack, resolved once per thread. An unknown stack yields a bound that
    // never clamps, which keeps the entry-relative limit.
    static size_t threadStackBound()
    {
        if (UNLIKELY(!s_threadStackBound.resolved)) {
            resolveThreadStackBound();
        }
        return s_threadStackBound.value;
    }
    static void resolveThreadStackBound();
    struct ThreadStackBound {
        bool resolved;
        size_t value;
    };
    static MAY_THREAD_LOCAL ThreadStackBound s_threadStackBound;

    static void deallocateBuffer(uint8_t* buffer)
    {
#ifdef ENABLE_GC
        GC_FREE(buffer);
#else
        free(buffer);
#endif
    }

    ExecutionState* m_parent;
    // This chain also crosses roots created by reentrant Trap::run calls.
    static thread_local ExecutionState* s_ownedFrames;
    ExecutionState* m_previousOwnedFrame = nullptr;
    Function* m_currentFunction;
    size_t m_stackLimit;
    // Stack frame related data
    uint8_t* m_bp;
    size_t m_capacity;
    uint8_t* m_owned;
};

} // namespace Walrus

#endif // __WalrusFunction__
