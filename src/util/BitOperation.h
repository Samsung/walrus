/*
 * Copyright 2020 WebAssembly Community Group participants
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

#ifndef __WalrusBitOperation__
#define __WalrusBitOperation__

#if defined(COMPILER_MSVC)
#include <intrin.h>
#endif

namespace Walrus {

template <typename T>
T shiftMask(T val) { return val & (sizeof(T) * 8 - 1); }

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)

ALWAYS_INLINE int clz(unsigned v)
{
    return v ? __builtin_clz(v) : sizeof(v) * 8;
}
ALWAYS_INLINE int clz(unsigned long v) { return v ? __builtin_clzl(v) : sizeof(v) * 8; }
ALWAYS_INLINE int clz(unsigned long long v) { return v ? __builtin_clzll(v) : sizeof(v) * 8; }

ALWAYS_INLINE int ctz(unsigned v) { return v ? __builtin_ctz(v) : sizeof(v) * 8; }
ALWAYS_INLINE int ctz(unsigned long v) { return v ? __builtin_ctzl(v) : sizeof(v) * 8; }
ALWAYS_INLINE int ctz(unsigned long long v) { return v ? __builtin_ctzll(v) : sizeof(v) * 8; }

ALWAYS_INLINE int popCount(uint8_t v) { return __builtin_popcount(v); }
ALWAYS_INLINE int popCount(unsigned v) { return __builtin_popcount(v); }
ALWAYS_INLINE int popCount(unsigned long v) { return __builtin_popcountl(v); }
ALWAYS_INLINE int popCount(unsigned long long v) { return __builtin_popcountll(v); }

#elif defined(COMPILER_MSVC)

#if WALRUS_32
inline unsigned long lowDWORD(unsigned __int64 v)
{
    return (unsigned long)v;
}

inline unsigned long highDWORD(unsigned __int64 v)
{
    unsigned long high;
    memcpy(&high, (unsigned char*)&v + sizeof(high), sizeof(high));
    return high;
}
#endif

inline int clz(unsigned long v)
{
    if (v == 0)
        return 32;

    unsigned long index;
    _BitScanReverse(&index, v);
    return sizeof(unsigned long) * 8 - (index + 1);
}

inline int clz(unsigned int v)
{
    return clz((unsigned long)v);
}

inline int clz(unsigned __int64 v)
{
#if WALRUS_32
    int result = Clz(highDWORD(v));
    if (result == 32)
        result += Clz(lowDWORD(v));

    return result;
#else
    if (v == 0)
        return 64;

    unsigned long index;
    _BitScanReverse64(&index, v);
    return sizeof(unsigned __int64) * 8 - (index + 1);
#endif
}

inline int ctz(unsigned long v)
{
    if (v == 0)
        return 32;

    unsigned long index;
    _BitScanForward(&index, v);
    return index;
}

inline int ctz(unsigned int v)
{
    return ctz((unsigned long)v);
}

inline int ctz(unsigned __int64 v)
{
#if WALRUS_32
    int result = ctz(lowDWORD(v));
    if (result == 32)
        result += ctz(highDWORD(v));

    return result;
#else
    if (v == 0)
        return 64;

    unsigned long index;
    _BitScanForward64(&index, v);
    return index;
#endif
}

inline int popCount(uint8_t value)
{
    return __popcnt(value);
}

inline int popCount(unsigned long value)
{
    return __popcnt(value);
}

inline int popCount(unsigned int value)
{
    return popCount((unsigned long)value);
}

inline int popCount(unsigned __int64 value)
{
#if WALRUS_32
    return popCount(highDWORD(value)) + popcount(lowDWORD(value));
#else
    return __popcnt64(value);
#endif
}

#else

#error unknown compiler

#endif

} // namespace Walrus

#endif
