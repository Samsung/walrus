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

#ifndef __WalrusVector__
#define __WalrusVector__

namespace Walrus {

template <typename T, bool isFundamental = std::is_fundamental<T>::value>
struct VectorCopier {
};

template <typename T>
struct VectorCopier<T, true> {
    static void copy(T* dst, const T* src, const size_t size)
    {
        memcpy(dst, src, sizeof(T) * size);
    }
};

template <typename T>
struct VectorCopier<T, false> {
    static void copy(T* dst, const T* src, const size_t size)
    {
        for (size_t i = 0; i < size; i++) {
            dst[i] = src[i];
        }
    }
};

template <size_t glowFactor = 100>
struct ComputeReservedCapacityFunctionWithLog2 {
    size_t operator()(size_t newSize)
    {
        if (newSize == 0) {
            return 0;
        }
        size_t base = FAST_LOG2_UINT(newSize);
        size_t capacity = 1 << (base + 1);
        if (glowFactor > 100) {
            return capacity * glowFactor / 100.f;
        }
        return capacity;
    }
};

template <size_t glowFactor = 125>
struct ComputeReservedCapacityFunctionWithPercent {
    size_t operator()(size_t newSize) { return newSize * (glowFactor / 100.f); }
};

template <size_t glowFactor = 125, size_t maxGap = 512>
struct ComputeReservedCapacityFunctionWithPercentAndGap {
    size_t operator()(size_t newSize)
    {
        size_t newCapacity = newSize * (glowFactor / 100.f);
        if ((newCapacity - newSize) > maxGap) {
            newCapacity = newSize + maxGap;
        }
        return newCapacity;
    }
};

template <size_t glowFactor = 125>
struct ComputeReservedCapacityFunctionWithLog2AndPercent {
    size_t operator()(size_t newSize)
    {
        if (newSize == 0) {
            return 0;
        }

        if (newSize <= 32) {
            ComputeReservedCapacityFunctionWithLog2<> f1;
            return f1(newSize);
        }

        ComputeReservedCapacityFunctionWithLog2<> f1;
        ComputeReservedCapacityFunctionWithPercentAndGap<> f2;
        return std::min(f1(newSize), f2(newSize));
    }
};

using VectorDefaultComputeReservedCapacityFunction = ComputeReservedCapacityFunctionWithPercent<>;

template <typename T,
          typename Allocator>
class VectorWithFixedSize;

template <typename T,
          typename Allocator = std::allocator<T>,
          typename ComputeReservedCapacityFunction = VectorDefaultComputeReservedCapacityFunction>
class Vector {
    friend class VectorWithFixedSize<T, Allocator>;

public:
    Vector()
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
    }

    explicit Vector(size_t siz)
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
        resize(siz);
    }

    Vector(Vector<T, Allocator, ComputeReservedCapacityFunction>&& other)
        : m_buffer(other.m_buffer)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        other.m_buffer = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    Vector(const Vector<T, Allocator, ComputeReservedCapacityFunction>& other)
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
        if (other.size()) {
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_buffer = Allocator().allocate(m_capacity);
            VectorCopier<T>::copy(m_buffer, other.data(), m_size);
        }
    }

    Vector(std::initializer_list<T> list)
        : Vector()
    {
        reserve(list.size());
        for (auto& i : list) {
            push_back(i);
        }
    }

    const Vector<T, Allocator, ComputeReservedCapacityFunction>& operator=(
        const Vector<T, Allocator, ComputeReservedCapacityFunction>& other)
    {
        if (&other == this)
            return *this;

        if (other.size()) {
            m_size = other.size();
            m_capacity = other.m_capacity;
            m_buffer = Allocator().allocate(m_capacity);
            VectorCopier<T>::copy(m_buffer, other.data(), m_size);
        } else {
            clear();
        }
        return *this;
    }

    const Vector<T, Allocator, ComputeReservedCapacityFunction>& operator=(
        Vector<T, Allocator, ComputeReservedCapacityFunction>&& other)
    {
        if (&other == this)
            return *this;

        if (other.size()) {
            m_size = other.size();
            m_buffer = other.m_buffer;
            m_capacity = other.m_capacity;
            other.m_buffer = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        } else {
            clear();
        }
        return *this;
    }

    Vector(const Vector<T, Allocator, ComputeReservedCapacityFunction>& other,
           const T& newItem)
    {
        m_size = other.size() + 1;
        ComputeReservedCapacityFunction f;
        m_capacity = f(m_size);
        m_buffer = Allocator().allocate(m_capacity);
        VectorCopier<T>::copy(m_buffer, other.data(), other.size());

        m_buffer[other.size()] = newItem;
    }

    ~Vector()
    {
        if (m_buffer)
            Allocator().deallocate(m_buffer, m_capacity);
    }

    void assign(const T* start, const T* end)
    {
        clear();

        m_size = std::distance(start, end);

        if (m_size) {
            ComputeReservedCapacityFunction f;
            m_capacity = f(m_size);
            T* newBuffer = Allocator().allocate(m_capacity);
            m_buffer = newBuffer;

            VectorCopier<T>::copy(m_buffer, start, m_size);
        }
    }

    void pushBack(const T& val)
    {
        if (m_capacity <= (m_size + 1)) {
            size_t oldc = m_capacity;
            ComputeReservedCapacityFunction f;
            m_capacity = f(m_size + 1);
            T* newBuffer = Allocator().allocate(m_capacity);

            if (m_buffer) {
                VectorCopier<T>::copy(newBuffer, m_buffer, m_size);
                Allocator().deallocate(m_buffer, oldc);
            }
            m_buffer = newBuffer;
        }
        m_buffer[m_size] = val;
        m_size++;
    }

    void push_back(const T& val) { pushBack(val); }

    void insert(size_t pos, const T& val)
    {
        ASSERT(pos <= m_size);
        if (m_capacity <= (m_size + 1)) {
            size_t oldC = m_capacity;

            ComputeReservedCapacityFunction f;
            m_capacity = f(m_size + 1);
            T* newBuffer = Allocator().allocate(m_capacity);

            VectorCopier<T>::copy(newBuffer, m_buffer, pos);
            newBuffer[pos] = val;
            VectorCopier<T>::copy(&newBuffer[pos + 1], m_buffer + pos, m_size - pos);

            if (m_buffer)
                Allocator().deallocate(m_buffer, oldC);
            m_buffer = newBuffer;
        } else {
            // TODO use memmove
            for (size_t i = m_size; i > pos; i--) {
                m_buffer[i] = m_buffer[i - 1];
            }

            m_buffer[pos] = val;
        }

        m_size++;
    }

    void erase(size_t pos) { erase(pos, pos + 1); }

    void erase(size_t start, size_t end)
    {
        ASSERT(start < end);
        ASSERT(end <= m_size);

        size_t c = end - start;
        if (m_size - c) {
            size_t oldC = m_capacity;
            T* newBuffer = Allocator().allocate(m_size - c);
            VectorCopier<T>::copy(newBuffer, m_buffer, start);
            if (m_size > end) {
                VectorCopier<T>::copy(&newBuffer[end - c], &m_buffer[end],
                                      m_size - end);
            }

            if (m_buffer)
                Allocator().deallocate(m_buffer, oldC);
            m_buffer = newBuffer;
            m_size = m_size - c;
            m_capacity = m_size;
        } else {
            clear();
        }
    }

    size_t size() const { return m_size; }

    size_t capacity() const { return m_capacity; }

    bool empty() const { return m_size == 0; }

    void pop_back() { erase(m_size - 1); }

    T& operator[](const size_t idx)
    {
        ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    const T& operator[](const size_t idx) const
    {
        ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    T& at(const size_t& idx)
    {
        ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    const T& at(const size_t& idx) const
    {
        ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    T* data() { return m_buffer; }

    const T* data() const { return m_buffer; }

    T& back() { return m_buffer[m_size - 1]; }

    using iterator = T*;
    constexpr iterator begin() const { return m_buffer; }
    constexpr iterator end() const { return m_buffer + m_size; }
    void clear()
    {
        if (m_buffer) {
            Allocator().deallocate(m_buffer, m_capacity);
        }
        m_buffer = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    void shrinkToFit()
    {
        ASSERT(m_size <= m_capacity);
        if (m_size != m_capacity) {
            if (m_size) {
                ASSERT(m_buffer);
                T* newBuffer = Allocator().allocate(m_size);
                VectorCopier<T>::copy(newBuffer, m_buffer, m_size);

                Allocator().deallocate(m_buffer, m_capacity);

                m_buffer = newBuffer;
                m_capacity = m_size;
            } else {
                clear();
            }
        }
    }

    bool isShrinked() const
    {
        return m_size == m_capacity;
    }

    void reserve(size_t newCapacity)
    {
        if (m_capacity < newCapacity) {
            T* newBuffer = Allocator().allocate(newCapacity);
            VectorCopier<T>::copy(newBuffer, m_buffer, m_size);

            if (m_buffer) {
                Allocator().deallocate(m_buffer, m_capacity);
            }

            m_buffer = newBuffer;
            m_capacity = newCapacity;
        }
    }

    void resizeWithUninitializedValues(size_t newSize)
    {
        if (newSize) {
            if (m_capacity < newSize) {
                ComputeReservedCapacityFunction f;
                size_t newCapacity = f(newSize);
                T* newBuffer = Allocator().allocate(newCapacity);
                VectorCopier<T>::copy(newBuffer, m_buffer, std::min(m_size, newSize));

                if (m_buffer)
                    Allocator().deallocate(m_buffer, m_capacity);

                m_buffer = newBuffer;
                m_capacity = newCapacity;
            }
            m_size = newSize;
        } else {
            clear();
        }
    }

    void resize(size_t newSize, const T& val = T())
    {
        if (newSize) {
            if (newSize > m_capacity) {
                ComputeReservedCapacityFunction f;
                size_t newCapacity = f(newSize);
                T* newBuffer = Allocator().allocate(newCapacity);
                VectorCopier<T>::copy(newBuffer, m_buffer, std::min(m_size, newSize));

                for (size_t i = m_size; i < newSize; i++) {
                    newBuffer[i] = val;
                }
                if (m_buffer)
                    Allocator().deallocate(m_buffer, m_capacity);
                m_size = newSize;
                m_capacity = newCapacity;
                m_buffer = newBuffer;
            } else {
                for (size_t i = m_size; i < newSize; i++) {
                    m_buffer[i] = val;
                }
                m_size = newSize;
            }
        } else {
            clear();
        }
    }

    T* takeBuffer()
    {
        T* buf = m_buffer;
        m_buffer = nullptr;
        clear();
        return buf;
    }

    void* operator new[](size_t size) = delete;

protected:
    T* m_buffer;
    size_t m_size;
    size_t m_capacity;
};

template <typename T,
          typename Allocator = std::allocator<T>>
class VectorWithFixedSize {
public:
    VectorWithFixedSize()
        : m_buffer(nullptr)
        , m_size(0)
    {
        // uninitialized
    }

    VectorWithFixedSize(Vector<T, Allocator>&& other)
        : m_buffer(nullptr)
        , m_size(other.m_size)
    {
        if (m_size) {
            if (other.m_size == other.m_capacity) {
                m_buffer = other.m_buffer;
                other.m_buffer = nullptr;
                other.m_size = 0;
                other.m_capacity = 0;
            } else {
                m_buffer = Allocator().allocate(m_size);
                VectorCopier<T>::copy(m_buffer, other.m_buffer, m_size);
                other.clear();
            }
        }
    }

    const VectorWithFixedSize<T, Allocator>& operator=(
        const VectorWithFixedSize<T, Allocator>& other)
        = delete;
    VectorWithFixedSize(const VectorWithFixedSize<T, Allocator>& other,
                        const T& newItem)
        = delete;

    ~VectorWithFixedSize()
    {
        clear();
    }

    size_t size() const { return m_size; }

    T& operator[](const size_t idx)
    {
        ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    const T& operator[](const size_t idx) const
    {
        ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    T* data() { return m_buffer; }
    const T* data() const { return m_buffer; }

    using iterator = T*;
    constexpr iterator begin() const { return m_buffer; }
    constexpr iterator end() const { return m_buffer + m_size; }

    void clear()
    {
        if (m_buffer) {
            ASSERT(m_size);
            Allocator().deallocate(m_buffer, m_size);
        }
        m_buffer = nullptr;
        m_size = 0;
    }

    void reserve(size_t newSize)
    {
        // reserve should be called for uninitialized Vector
        ASSERT(!m_buffer && !m_size);
        if (newSize) {
            m_buffer = Allocator().allocate(newSize);
            m_size = newSize;
        }
    }

    void* operator new[](size_t size) = delete;

protected:
    T* m_buffer;
    size_t m_size;
};

// Vector for special purpose
// It has InlineStorage, so push_back operation is fast with InlineStorage
template <unsigned int InlineStorageSize,
          typename T,
          typename ExternalStorageAllocator>
class VectorWithInlineStorage {
public:
    VectorWithInlineStorage()
        : m_useExternalStorage(false)
        , m_size(0)
        , m_inlineStorage{}
    {
    }

    VectorWithInlineStorage(
        const VectorWithInlineStorage<InlineStorageSize,
                                      T,
                                      ExternalStorageAllocator>& src)
    {
        copy(src);
    }

    VectorWithInlineStorage(size_t s)
        : m_useExternalStorage(false)
        , m_size(s)
        , m_inlineStorage{}
    {
        if (UNLIKELY(m_size >= InlineStorageSize)) {
            m_externalStorage.resize(s);
            m_useExternalStorage = true;
        }
    }

    VectorWithInlineStorage(const T* data, size_t s)
        : m_useExternalStorage(false)
        , m_size(s)
        , m_inlineStorage{}
    {
        if (LIKELY(m_size < InlineStorageSize)) {
            for (size_t i = 0; i < s; i++) {
                new (inlineAt(i)) T(data[i]);
            }
        } else {
            m_useExternalStorage = true;
            m_externalStorage.assign(&data[0], &data[s]);
        }
    }

    VectorWithInlineStorage(
        VectorWithInlineStorage<InlineStorageSize, T, ExternalStorageAllocator>&&
            src)
        : m_useExternalStorage(false)
        , m_size(0)
        , m_inlineStorage{}
    {
        move(std::move(src));
    }

    void clear()
    {
        if (m_useExternalStorage) {
            m_externalStorage.clear();
        } else {
            for (size_t i = 0; i < m_size; i++) {
                inlineAt(i)->~T();
            }
        }
        m_size = 0;
        m_useExternalStorage = false;
    }

    ~VectorWithInlineStorage() { clear(); }

    const VectorWithInlineStorage<InlineStorageSize, T, ExternalStorageAllocator>&
    operator=(
        VectorWithInlineStorage<InlineStorageSize, T, ExternalStorageAllocator>&&
            src)
    {
        clear();
        move(std::move(src));
        return *this;
    }

    const VectorWithInlineStorage<InlineStorageSize, T, ExternalStorageAllocator>&
    operator=(const VectorWithInlineStorage<InlineStorageSize,
                                            T,
                                            ExternalStorageAllocator>& src)
    {
        clear();
        copy(src);
        return *this;
    }

    T* data()
    {
        if (LIKELY(!m_useExternalStorage)) {
            return (T*)m_inlineStorage;
        } else {
            return m_externalStorage.data();
        }
    }

    void pushBack(const T& decl) { push_back(decl); }

    void push_back(const T& decl)
    {
        if (LIKELY(!m_useExternalStorage)) {
            if (LIKELY(m_size < InlineStorageSize)) {
                new (inlineAt(m_size)) T(decl);
            } else {
                m_useExternalStorage = true;
                m_externalStorage.reserve(m_size + 1);

                for (size_t i = 0; i < m_size; i++) {
                    m_externalStorage.push_back(std::move(*inlineAt(i)));
                }

                m_externalStorage.push_back(decl);
            }
        } else {
            m_externalStorage.push_back(decl);
        }
        m_size++;
    }

    void pop_back()
    {
        m_size--;
        if (UNLIKELY(m_useExternalStorage)) {
            m_externalStorage.pop_back();
        } else {
            inlineAt(m_size)->~T();
        }
    }

    T& operator[](const size_t& idx)
    {
        if (LIKELY(!m_useExternalStorage)) {
            return *inlineAt(idx);
        } else {
            return m_externalStorage[idx];
        }
    }

    const T& operator[](const size_t& idx) const
    {
        if (LIKELY(!m_useExternalStorage)) {
            return *inlineAt(idx);
        } else {
            return m_externalStorage[idx];
        }
    }

    T& back() { return operator[](m_size - 1); }

    size_t size() const { return m_size; }

    T* begin()
    {
        if (LIKELY(!m_useExternalStorage)) {
            return inlineAt(0);
        } else {
            return m_externalStorage.data();
        }
    }

    T* end()
    {
        if (LIKELY(!m_useExternalStorage)) {
            return inlineAt(m_size);
        } else {
            return m_externalStorage.data() + m_size;
        }
    }

    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;

protected:
    T* inlineAt(size_t idx) { return (T*)(&m_inlineStorage[idx * sizeof(T)]); }

    const T* inlineAt(size_t idx) const
    {
        return (T*)(&m_inlineStorage[idx * sizeof(T)]);
    }

    void copy(const VectorWithInlineStorage<InlineStorageSize,
                                            T,
                                            ExternalStorageAllocator>& src)
    {
        m_size = src.m_size;
        m_useExternalStorage = src.m_useExternalStorage;

        if (src.m_useExternalStorage) {
            m_externalStorage = src.m_externalStorage;
        } else {
            for (size_t i = 0; i < src.m_size; i++) {
                new (inlineAt(i)) T(*src.inlineAt(i));
            }
        }
    }

    void move(
        VectorWithInlineStorage<InlineStorageSize, T, ExternalStorageAllocator>&&
            src)
    {
        m_size = src.m_size;
        if (src.m_useExternalStorage) {
            m_externalStorage = std::move(src.m_externalStorage);
            m_useExternalStorage = true;
        } else {
            for (size_t i = 0; i < m_size; i++) {
                new (inlineAt(i)) T(std::move(*src.inlineAt(i)));
            }
            m_useExternalStorage = false;
        }

        src.clear();
    }

    bool m_useExternalStorage;
    size_t m_size;
    uint8_t m_inlineStorage[InlineStorageSize * sizeof(T)];
    Vector<T, ExternalStorageAllocator> m_externalStorage;
};

class VectorUtil {
public:
    static const size_t invalidIndex;

    template <typename T, typename E>
    static size_t findInVector(const T& vector, const E& target)
    {
        size_t len = vector.size();
        for (size_t i = 0; i < len; i++) {
            if (vector[i] == target) {
                return i;
            }
        }

        return invalidIndex;
    }
};
} // namespace Walrus

#endif
