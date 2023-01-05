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

#ifndef __WalrusOptional__
#define __WalrusOptional__

namespace Walrus {

template <typename T>
class Optional {
public:
    Optional()
        : m_hasValue(false)
        , m_value()
    {
    }

    Optional(T value)
        : m_hasValue(true)
        , m_value(value)
    {
    }

    Optional(std::nullptr_t value)
        : m_hasValue(false)
        , m_value()
    {
    }

    T& value()
    {
        ASSERT(m_hasValue);
        return m_value;
    }

    const T& value() const
    {
        ASSERT(m_hasValue);
        return m_value;
    }

    bool hasValue() const { return m_hasValue; }

    T unwrap() const { return m_value; }

    operator bool() const { return hasValue(); }

    void reset()
    {
        m_value = T();
        m_hasValue = false;
    }

    bool operator==(const Optional<T>& other) const
    {
        if (m_hasValue != other.hasValue()) {
            return false;
        }
        return m_hasValue ? m_value == other.m_value : true;
    }

    bool operator!=(const Optional<T>& other) const
    {
        return !this->operator==(other);
    }

    bool operator==(const T& other) const
    {
        if (m_hasValue) {
            return value() == other;
        }
        return false;
    }

    bool operator!=(const T& other) const { return !operator==(other); }

protected:
    bool m_hasValue;
    T m_value;
};

template <typename T>
inline bool operator==(const T& a, const Optional<T>& b)
{
    return b == a;
}

template <typename T>
inline bool operator!=(const T& a, const Optional<T>& b)
{
    return b != a;
}

template <typename T>
class Optional<T*> {
public:
    Optional()
        : m_value(nullptr)
    {
    }

    Optional(T* value)
        : m_value(value)
    {
    }

    Optional(std::nullptr_t value)
        : m_value(nullptr)
    {
    }

    T* value()
    {
        ASSERT(hasValue());
        return m_value;
    }

    const T* value() const
    {
        ASSERT(hasValue());
        return m_value;
    }

    bool hasValue() const { return !!m_value; }

    operator bool() const { return hasValue(); }

    void reset() { m_value = nullptr; }

    T* operator->()
    {
        ASSERT(hasValue());
        return m_value;
    }

    const T* operator->() const
    {
        ASSERT(hasValue());
        return m_value;
    }

    T* unwrap() const { return m_value; }

    bool operator==(const Optional<T*>& other) const
    {
        if (hasValue() != other.hasValue()) {
            return false;
        }
        return hasValue() ? m_value == other.m_value : true;
    }

    bool operator!=(const Optional<T*>& other) const
    {
        return !this->operator==(other);
    }

    bool operator==(const T*& other) const
    {
        if (hasValue()) {
            return value() == other;
        }
        return false;
    }

    bool operator!=(const T*& other) const { return !operator==(other); }

protected:
    T* m_value;
};

template <typename T>
inline bool operator==(const T*& a, const Optional<T*>& b)
{
    return b == a;
}

template <typename T>
inline bool operator!=(const T*& a, const Optional<T*>& b)
{
    return b != a;
}
} // namespace Walrus

#endif
