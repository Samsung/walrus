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

#include "util/BitOperation.h"
#include "runtime/ExecutionState.h"
#include "runtime/Object.h"
#include "runtime/Store.h"
#include <atomic>

namespace Walrus {

class Store;
class DataSegment;

class Memory : public Extern {
    friend class JITCompiler;

public:
    static const uint32_t s_memoryPageSize = 1024 * 64;
    static const uint64_t s_maxMemory64Grow = ~static_cast<uint64_t>(0) / s_memoryPageSize;

    // Caching memory target for fast access.
    struct TargetBuffer {
        TargetBuffer()
        {
            setUninitialized();
        }

        void setUninitialized()
        {
            sizeInByte = ~(uint64_t)0;
        }

        void enque(Memory* memory);
        void deque(Memory* memory);

        TargetBuffer* next;
        uint8_t* buffer;
        uint64_t sizeInByte;
    };

    static Memory* createMemory(Store* store, uint64_t initialSizeInByte, uint64_t maximumSizeInByte,
                                bool isShared, bool is64);

    ~Memory();

    uint8_t* buffer() const
    {
        return m_buffer;
    }

    uint64_t sizeInByte() const
    {
        return m_sizeInByte;
    }

    uint64_t sizeInPageSize() const
    {
        return sizeInByte() / s_memoryPageSize;
    }

    uint64_t maximumSizeInByte() const
    {
        return m_maximumSizeInByte;
    }

    uint64_t maximumSizeInPageSize() const
    {
        return m_maximumSizeInByte / s_memoryPageSize;
    }

    bool isShared() const
    {
        return m_isShared;
    }

    bool is64() const
    {
        return m_is64;
    }

    bool grow(uint64_t growSizeInByte);

    template <typename T>
    void load(ExecutionState& state, uint32_t offset, uint32_t addend, T* out) const
    {
        checkAccess(state, offset, sizeof(T), addend);

        memcpyEndianAware(out, m_buffer, sizeof(T), m_sizeInByte, 0, offset + addend, sizeof(T));
    }

    template <typename T>
    void loadM64(ExecutionState& state, uint64_t offset, uint64_t addend, T* out) const
    {
        checkAccessM64(state, offset, sizeof(T), addend);

        memcpyEndianAware(out, m_buffer, sizeof(T), m_sizeInByte, 0, static_cast<size_t>(offset + addend), sizeof(T));
    }

    template <typename T>
    void load(ExecutionState& state, uint32_t offset, T* out) const
    {
        checkAccess(state, offset, sizeof(T));
#if defined(WALRUS_BIG_ENDIAN)
        *out = *(reinterpret_cast<T*>(&m_buffer[m_sizeInByte - sizeof(T) - offset]));
#else
        *out = *(reinterpret_cast<T*>(&m_buffer[offset]));
#endif
    }

    template <typename T>
    void loadM64(ExecutionState& state, uint64_t offset, T* out) const
    {
        checkAccessM64(state, offset, sizeof(T));
#if defined(WALRUS_BIG_ENDIAN)
        *out = *(reinterpret_cast<T*>(&m_buffer[m_sizeInByte - sizeof(T) - offset]));
#else
        *out = *(reinterpret_cast<T*>(&m_buffer[offset]));
#endif
    }

#ifdef CPU_ARM32

#define defineUnalignedLoad(TYPE)                                                                                   \
    template <typename T = TYPE>                                                                                    \
    void load(ExecutionState& state, uint32_t offset, TYPE* out) const                                              \
    {                                                                                                               \
        checkAccess(state, offset, sizeof(TYPE));                                                                   \
        memcpyEndianAware(out, m_buffer, sizeof(TYPE), m_sizeInByte, 0, offset, sizeof(TYPE));                      \
    }                                                                                                               \
    template <typename T = TYPE>                                                                                    \
    void loadM64(ExecutionState& state, uint64_t offset, TYPE* out) const                                           \
    {                                                                                                               \
        checkAccessM64(state, offset, sizeof(TYPE));                                                                \
        memcpyEndianAware(out, m_buffer, sizeof(TYPE), m_sizeInByte, 0, static_cast<size_t>(offset), sizeof(TYPE)); \
    }
    defineUnalignedLoad(uint64_t);
    defineUnalignedLoad(int64_t);
    defineUnalignedLoad(double);
#undef defineUnalignedLoad

#endif

    template <typename T>
    void store(ExecutionState& state, uint32_t offset, uint32_t addend, const T& val) const
    {
        checkAccess(state, offset, sizeof(T), addend);

        memcpyEndianAware(m_buffer, &val, m_sizeInByte, sizeof(T), offset + addend, 0, sizeof(T));
    }

    template <typename T>
    void storeM64(ExecutionState& state, uint64_t offset, uint64_t addend, const T& val) const
    {
        checkAccessM64(state, offset, sizeof(T), addend);

        memcpyEndianAware(m_buffer, &val, m_sizeInByte, sizeof(T), static_cast<size_t>(offset + addend), 0, sizeof(T));
    }

    template <typename T>
    void store(ExecutionState& state, uint32_t offset, const T& val) const
    {
        checkAccess(state, offset, sizeof(T));
#if defined(WALRUS_BIG_ENDIAN)
        *(reinterpret_cast<T*>(&m_buffer[m_sizeInByte - sizeof(T) - offset])) = val;
#else
        *(reinterpret_cast<T*>(&m_buffer[offset])) = val;
#endif
    }

    template <typename T>
    void storeM64(ExecutionState& state, uint64_t offset, const T& val) const
    {
        checkAccessM64(state, offset, sizeof(T));
#if defined(WALRUS_BIG_ENDIAN)
        *(reinterpret_cast<T*>(&m_buffer[m_sizeInByte - sizeof(T) - offset])) = val;
#else
        *(reinterpret_cast<T*>(&m_buffer[offset])) = val;
#endif
    }

    enum AtomicRmwOp {
        Add,
        Sub,
        And,
        Or,
        Xor,
        Xchg
    };

    template <typename T>
    void atomicLoad(ExecutionState& state, uint32_t offset, uint32_t addend, T* out) const
    {
        checkAtomicAccess(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        *out = shared->load(std::memory_order_relaxed);
    }

    template <typename T>
    void atomicLoadM64(ExecutionState& state, uint64_t offset, uint64_t addend, T* out) const
    {
        checkAtomicAccessM64(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        *out = shared->load(std::memory_order_relaxed);
    }

    template <typename T>
    void atomicStore(ExecutionState& state, uint32_t offset, uint32_t addend, const T& val) const
    {
        checkAtomicAccess(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        shared->store(val);
    }

    template <typename T>
    void atomicStoreM64(ExecutionState& state, uint64_t offset, uint64_t addend, const T& val) const
    {
        checkAtomicAccessM64(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        shared->store(val);
    }

    template <typename T>
    void atomicRmw(ExecutionState& state, uint32_t offset, uint32_t addend, const T& val, T* out, AtomicRmwOp operation) const
    {
        checkAtomicAccess(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        doAtomicRmw(shared, val, out, operation);
    }

    template <typename T>
    void atomicRmwM64(ExecutionState& state, uint64_t offset, uint64_t addend, const T& val, T* out, AtomicRmwOp operation) const
    {
        checkAtomicAccessM64(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        doAtomicRmw(shared, val, out, operation);
    }

    template <typename T>
    void atomicRmwCmpxchg(ExecutionState& state, uint32_t offset, uint32_t addend, T expect, const T& replace, T* out) const
    {
        checkAtomicAccess(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        shared->compare_exchange_weak(expect, replace);
        *out = expect;
    }

    template <typename T>
    void atomicRmwCmpxchgM64(ExecutionState& state, uint64_t offset, uint64_t addend, T expect, const T& replace, T* out) const
    {
        checkAtomicAccessM64(state, offset, sizeof(T), addend);
        std::atomic<T>* shared = reinterpret_cast<std::atomic<T>*>(m_buffer + (offset + addend));
        shared->compare_exchange_weak(expect, replace);
        *out = expect;
    }

    template <typename T>
    void atomicWait(ExecutionState& state, Store* store, uint32_t offset, uint32_t addend, const T& expect, int64_t timeOut, uint32_t* out) const
    {
        checkAtomicAccess(state, offset, sizeof(T), addend);
        if (UNLIKELY(!m_isShared)) {
            throwUnsharedMemoryException(state);
        }

        atomicWait(state, store, m_buffer + (offset + addend), expect, timeOut, out);
    }

    template <typename T>
    void atomicWaitM64(ExecutionState& state, Store* store, uint64_t offset, uint64_t addend, const T& expect, int64_t timeOut, uint32_t* out) const
    {
        checkAtomicAccessM64(state, offset, sizeof(T), addend);
        if (UNLIKELY(!m_isShared)) {
            throwUnsharedMemoryException(state);
        }

        atomicWait(state, store, m_buffer + (offset + addend), expect, timeOut, out);
    }

    template <typename T>
    void atomicWait(ExecutionState& state, Store* store, uint8_t* absoluteAddress, const T& expect, int64_t timeOut, uint32_t* out) const
    {
        T read;
        atomicLoad(state, absoluteAddress - m_buffer, 0, &read);
        if (read != expect) {
            // "not-equal", the loaded value did not match the expected value
            *out = 1;
        } else {
            // wait process
            bool notified = false;
            Waiter* waiter = store->getWaiter(static_cast<void*>(absoluteAddress));

            // lock waiter
            std::unique_lock<std::mutex> lock(waiter->m_mutex);

            Waiter::WaiterItem* item = new Waiter::WaiterItem(waiter);
            waiter->m_waiterItemList.push_back(item);

            // wait
            notified = (waiter->m_condition.wait_for(lock, std::chrono::nanoseconds(timeOut)) == std::cv_status::no_timeout);

            ASSERT(std::find(waiter->m_waiterItemList.begin(), waiter->m_waiterItemList.end(), item) != waiter->m_waiterItemList.end());
            auto iter = waiter->m_waiterItemList.begin();
            while (iter != waiter->m_waiterItemList.end()) {
                if (*iter == item) {
                    waiter->m_waiterItemList.erase(iter);
                    break;
                }
                iter++;
            }
            delete item;

            if (notified) {
                // "ok", woken by another agent in the cluster
                *out = 0;
            } else {
                // "timed-out", not woken before timeout expired
                *out = 2;
            }
        }
    }

    void atomicNotify(ExecutionState& state, Store* store, uint32_t offset, uint32_t addend, const uint32_t& count, uint32_t* out) const
    {
        checkAtomicAccess(state, offset, 4, addend);
        if (UNLIKELY(!m_isShared)) {
            *out = 0;
            return;
        }

        atomicNotify(store, m_buffer + (offset + addend), count, out);
    }

    void atomicNotifyM64(ExecutionState& state, Store* store, uint64_t offset, uint64_t addend, const uint32_t& count, uint32_t* out) const
    {
        checkAtomicAccessM64(state, offset, 4, addend);
        if (UNLIKELY(!m_isShared)) {
            *out = 0;
            return;
        }

        atomicNotify(store, m_buffer + (offset + addend), count, out);
    }

    void atomicNotify(Store* store, uint8_t* absoluteAddress, const uint32_t& count, uint32_t* out) const
    {
        Waiter* waiter = store->getWaiter(static_cast<void*>(absoluteAddress));

        waiter->m_mutex.lock();
        uint32_t realCount = std::min(waiter->m_waiterItemList.size(), (size_t)count);

        for (uint32_t i = 0; i < realCount; i++) {
            waiter->m_waiterItemList[i]->m_waiter->m_condition.notify_one();
        }

        waiter->m_mutex.unlock();
        *out = realCount;
    }

#ifdef CPU_ARM32

#define defineUnalignedStore(TYPE)                                                                                   \
    template <typename T = TYPE>                                                                                     \
    void store(ExecutionState& state, uint32_t offset, const TYPE& val) const                                        \
    {                                                                                                                \
        checkAccess(state, offset, sizeof(TYPE));                                                                    \
        memcpyEndianAware(m_buffer, &val, m_sizeInByte, sizeof(TYPE), offset, 0, sizeof(TYPE));                      \
    }                                                                                                                \
    template <typename T = TYPE>                                                                                     \
    void storeM64(ExecutionState& state, uint64_t offset, const TYPE& val) const                                     \
    {                                                                                                                \
        checkAccessM64(state, offset, sizeof(TYPE));                                                                 \
        memcpyEndianAware(m_buffer, &val, m_sizeInByte, sizeof(TYPE), static_cast<size_t>(offset), 0, sizeof(TYPE)); \
    }
    defineUnalignedStore(uint64_t);
    defineUnalignedStore(int64_t);
    defineUnalignedStore(double);
#undef defineUnalignedStore

#endif

    void init(ExecutionState& state, DataSegment* source, uint64_t dstStart, uint32_t srcStart, uint32_t srcSize);
    void copy(ExecutionState& state, uint64_t dstStart, uint64_t srcStart, uint64_t size, Memory* dstMem = nullptr);
    void fill(ExecutionState& state, uint64_t start, uint8_t value, uint64_t size);

    inline bool checkAccess(uint32_t offset, uint32_t size, uint32_t addend = 0) const
    {
        ASSERT(!is64());
        return !UNLIKELY(!((uint64_t)offset + (uint64_t)addend + (uint64_t)size <= m_sizeInByte));
    }

    inline bool checkAccessM64(uint64_t offset, uint64_t size) const
    {
        ASSERT(is64());
        return !UNLIKELY(offset > m_sizeInByte || size > m_sizeInByte - offset);
    }

    inline bool checkAccessM64(uint64_t offset, uint64_t size, uint64_t addend) const
    {
        ASSERT(is64());
        return !UNLIKELY(offset > m_sizeInByte || addend > m_sizeInByte - offset || size > m_sizeInByte - offset - addend);
    }

    void initMemory(DataSegment* source, size_t dstStart, uint32_t srcStart, uint32_t srcSize);
    void copyMemory(size_t dstStart, size_t srcStart, size_t size);
    void copyMemory(Memory* dstMemory, size_t dstStart, size_t srcStart, size_t size);
    void fillMemory(size_t start, uint8_t value, size_t size);

private:
    Memory(uint64_t initialSizeInByte, uint64_t maximumSizeInByte, bool isShared, bool is64);

    void throwRangeException(ExecutionState& state, uint32_t offset, uint32_t addend, uint32_t size) const;

    inline void checkAccess(ExecutionState& state, uint32_t offset, uint32_t size, uint32_t addend = 0) const
    {
        if (!this->checkAccess(offset, size, addend)) {
            throwRangeException(state, offset, addend, size);
        }
    }

    inline void checkAccessM64(ExecutionState& state, uint64_t offset, uint64_t size, uint64_t addend = 0) const
    {
        if (!this->checkAccessM64(offset, size, addend)) {
            throwRangeException(state, offset, addend, size);
        }
    }

    template <typename T>
    void doAtomicRmw(std::atomic<T>* shared, const T& val, T* out, AtomicRmwOp operation) const
    {
        switch (operation) {
        case Add:
            *out = shared->fetch_add(val);
            break;
        case Sub:
            *out = shared->fetch_sub(val);
            break;
        case And:
            *out = shared->fetch_and(val);
            break;
        case Or:
            *out = shared->fetch_or(val);
            break;
        case Xor:
            *out = shared->fetch_xor(val);
            break;
        case Xchg:
            *out = shared->exchange(val);
            break;
        }
    }

    void checkAtomicAccess(ExecutionState& state, uint32_t offset, uint32_t size, uint32_t addend = 0) const;
    void checkAtomicAccessM64(ExecutionState& state, uint64_t offset, uint64_t size, uint64_t addend = 0) const;
    void throwUnsharedMemoryException(ExecutionState& state) const;

    uint64_t m_sizeInByte;
    uint64_t m_reservedSizeInByte;
    uint64_t m_maximumSizeInByte;
    uint8_t* m_buffer;
    TargetBuffer* m_targetBuffers;
    bool m_isShared;
    bool m_is64;
};

} // namespace Walrus

#endif // __WalrusMemory__
