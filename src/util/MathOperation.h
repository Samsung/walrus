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

#ifndef __WalrusMathOperation__
#define __WalrusMathOperation__

#include "util/BitOperation.h"
#include "runtime/ExecutionState.h"

namespace Walrus {

template <typename T, typename std::enable_if<!std::is_floating_point<T>::value, int>::type = 0>
bool isNaN(T val)
{
    return false;
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
bool isNaN(T val)
{
    return std::isnan(val);
}

template <typename T, typename std::enable_if<!std::is_floating_point<T>::value, int>::type = 0>
T canonNaN(T val)
{
    return val;
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
T canonNaN(T val)
{
    if (UNLIKELY(std::isnan(val))) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    return val;
}

// This is a wrapping absolute value function, so a negative number that is not
// representable as a positive number will be unchanged (e.g. abs(-128) = 128).
//
// Note that std::abs() does not have this behavior (e.g. abs(-128) is UB).
// Similarly, using unary minus is also UB.
template <typename T>
T intAbs(T val)
{
    static_assert(std::is_unsigned<T>::value, "T must be unsigned.");
    const auto signbit = T(-1) << (sizeof(T) * 8 - 1);
    return (val & signbit) ? ~val + 1 : val;
}

// Because of the integer promotion rules [1], any value of a type T which is
// smaller than `int` will be converted to an `int`, as long as `int` can hold
// any value of type T.
//
// So type `uint16_t` will be promoted to `int`, since all values can be stored in
// an int. Unfortunately, the product of two `uint16_t` values cannot always be
// stored in an `int` (e.g. 65535 * 65535). This triggers an error in UBSan.
//
// As a result, we make sure to promote the type ahead of time for `uint16_t`. Note
// that this isn't a problem for any other unsigned types.
//
// [1]; https://en.cppreference.com/w/cpp/language/implicit_conversion#Integral_promotion
template <typename T>
struct PromoteMul {
    using type = T;
};
template <>
struct PromoteMul<uint16_t> {
    using type = uint32_t;
};

template <typename T>
T mul(ExecutionState& state, T lhs, T rhs)
{
    using U = typename PromoteMul<T>::type;
    return canonNaN(U(lhs) * U(rhs));
}

template <typename T>
struct Mask {
    using Type = T;
};
template <>
struct Mask<float> {
    using Type = uint32_t;
};
template <>
struct Mask<double> {
    using Type = uint64_t;
};

template <typename T>
typename Mask<T>::Type eqMask(ExecutionState& state, T lhs, T rhs) { return lhs == rhs ? -1 : 0; }
template <typename T>
typename Mask<T>::Type neMask(ExecutionState& state, T lhs, T rhs) { return lhs != rhs ? -1 : 0; }
template <typename T>
typename Mask<T>::Type ltMask(ExecutionState& state, T lhs, T rhs) { return lhs < rhs ? -1 : 0; }
template <typename T>
typename Mask<T>::Type leMask(ExecutionState& state, T lhs, T rhs) { return lhs <= rhs ? -1 : 0; }
template <typename T>
typename Mask<T>::Type gtMask(ExecutionState& state, T lhs, T rhs) { return lhs > rhs ? -1 : 0; }
template <typename T>
typename Mask<T>::Type geMask(ExecutionState& state, T lhs, T rhs) { return lhs >= rhs ? -1 : 0; }

template <typename T>
T intRotl(ExecutionState& state, T lhs, T rhs)
{
    return (lhs << shiftMask(rhs)) | (lhs >> shiftMask<T>(0 - rhs));
}

template <typename T>
T intRotr(ExecutionState& state, T lhs, T rhs)
{
    return (lhs >> shiftMask(rhs)) | (lhs << shiftMask<T>(0 - rhs));
}

// i{32,64}.{div,rem}_s are special-cased because they trap when dividing the
// max signed value by -1. The modulo operation on x86 uses the same
// instruction to generate the quotient and the remainder.
template <typename T,
          typename std::enable_if<std::is_signed<T>::value, int>::type = 0>
bool isNormalDivRem(T lhs, T rhs)
{
    return !(lhs == std::numeric_limits<T>::min() && rhs == -1);
}

template <typename T,
          typename std::enable_if<!std::is_signed<T>::value, int>::type = 0>
bool isNormalDivRem(T lhs, T rhs)
{
    return true;
}

// using ALWAYS_INLINE needs by regard to return some nan values (like nan:0x0f1e2) is converted other value
#if defined(COMPILER_MSVC)
template <typename T>
ALWAYS_INLINE T floatAbs(T val);
template <typename T>
ALWAYS_INLINE T floatCopysign(T lhs, T rhs);

// Don't use std::{abs,copysign} directly on MSVC, since that seems to lose
// the NaN tag.
template <>
ALWAYS_INLINE float floatAbs(float val)
{
    return _mm_cvtss_float(_mm_and_ps(
        _mm_set1_ps(val), _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))));
}

template <>
ALWAYS_INLINE double floatAbs(double val)
{
    return _mm_cvtsd_double(
        _mm_and_pd(_mm_set1_pd(val),
                   _mm_castsi128_pd(_mm_set1_epi64x(0x7fffffffffffffffull))));
}

template <>
ALWAYS_INLINE float floatCopysign(float lhs, float rhs)
{
    return _mm_cvtss_float(
        _mm_or_ps(_mm_and_ps(_mm_set1_ps(lhs),
                             _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))),
                  _mm_and_ps(_mm_set1_ps(rhs),
                             _mm_castsi128_ps(_mm_set1_epi32(0x80000000)))));
}

template <>
ALWAYS_INLINE double floatCopysign(double lhs, double rhs)
{
    return _mm_cvtsd_double(_mm_or_pd(
        _mm_and_pd(_mm_set1_pd(lhs),
                   _mm_castsi128_pd(_mm_set1_epi64x(0x7fffffffffffffffull))),
        _mm_and_pd(_mm_set1_pd(rhs),
                   _mm_castsi128_pd(_mm_set1_epi64x(0x8000000000000000ull)))));
}

#else
template <typename T>
ALWAYS_INLINE T floatAbs(T val)
{
#ifndef NDEBUG
    // GCC does not inline std::abs in debug mode
    if (std::is_same<T, float>::value) {
        int32_t& i = reinterpret_cast<int32_t&>(val);
        i &= 0x7FFFFFFF;
        return val;
    } else if (std::is_same<T, double>::value) {
        int64_t& i = reinterpret_cast<int64_t&>(val);
        i &= 0x7FFFFFFFFFFFFFFFLL;
        return val;
    }
    return val < 0 ? -val : val;
#else
    return std::abs(val);
#endif
}

template <typename T>
ALWAYS_INLINE T floatCopysign(ExecutionState& state, T lhs, T rhs)
{
#ifndef NDEBUG
    // GCC does not inline std::copysign in debug mode
    if (std::is_same<T, float>::value) {
        int32_t& i = reinterpret_cast<int32_t&>(lhs);
        int32_t& j = reinterpret_cast<int32_t&>(rhs);
        i &= 0x7FFFFFFF;
        bool sign = j & 0X80000000;
        if (sign) {
            i |= 0X80000000;
        }
        return lhs;
    } else if (std::is_same<T, double>::value) {
        int64_t& i = reinterpret_cast<int64_t&>(lhs);
        int64_t& j = reinterpret_cast<int64_t&>(rhs);
        i &= 0x7FFFFFFFFFFFFFFFLL;
        bool sign = j & 0X8000000000000000LL;
        if (sign) {
            i |= 0X8000000000000000LL;
        }
        return lhs;
    }
    return std::copysign(lhs, rhs);
#else
    return std::copysign(lhs, rhs);
#endif
}
#endif

#if COMPILER_IS_MSVC
#else
#endif

template <typename T>
ALWAYS_INLINE T floatNeg(T val)
{
    return -val;
}
template <typename T>
ALWAYS_INLINE T floatCeil(T val) { return canonNaN(std::ceil(val)); }
template <typename T>
ALWAYS_INLINE T floatFloor(T val) { return canonNaN(std::floor(val)); }
template <typename T>
ALWAYS_INLINE T floatTrunc(T val) { return canonNaN(std::trunc(val)); }
template <typename T>
ALWAYS_INLINE T floatNearest(T val) { return canonNaN(std::nearbyint(val)); }
template <typename T>
ALWAYS_INLINE T floatSqrt(T val) { return canonNaN(std::sqrt(val)); }

template <typename T>
ALWAYS_INLINE T floatDiv(ExecutionState& state, T lhs, T rhs)
{
    // IEE754 specifies what should happen when dividing a float by zero, but
    // C/C++ says it is undefined behavior.
    if (UNLIKELY(rhs == 0)) {
        return std::isnan(lhs) || lhs == 0
            ? std::numeric_limits<T>::quiet_NaN()
            : ((std::signbit(lhs) ^ std::signbit(rhs))
                   ? -std::numeric_limits<T>::infinity()
                   : std::numeric_limits<T>::infinity());
    }
    return canonNaN(lhs / rhs);
}

template <typename T>
ALWAYS_INLINE T floatMin(ExecutionState& state, T lhs, T rhs)
{
    if (UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
        return std::numeric_limits<T>::quiet_NaN();
    } else if (UNLIKELY(lhs == 0 && rhs == 0)) {
        return std::signbit(lhs) ? lhs : rhs;
    } else {
        return std::min(lhs, rhs);
    }
}

template <typename T>
ALWAYS_INLINE T floatPMin(ExecutionState& state, T lhs, T rhs)
{
    return std::min(lhs, rhs);
}

template <typename T>
ALWAYS_INLINE T floatMax(ExecutionState& state, T lhs, T rhs)
{
    if (UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
        return std::numeric_limits<T>::quiet_NaN();
    } else if (UNLIKELY(lhs == 0 && rhs == 0)) {
        return std::signbit(lhs) ? rhs : lhs;
    } else {
        return std::max(lhs, rhs);
    }
}

template <typename T>
ALWAYS_INLINE T floatPMax(ExecutionState& state, T lhs, T rhs)
{
    return std::max(lhs, rhs);
}

template <typename R, typename T>
bool canConvert(T val) { return true; }
template <>
inline bool canConvert<int32_t, float>(float val) { return val >= -2147483648.f && val < 2147483648.f; }
template <>
inline bool canConvert<int32_t, double>(double val) { return val > -2147483649. && val < 2147483648.; }
template <>
inline bool canConvert<uint32_t, float>(float val) { return val > -1.f && val < 4294967296.f; }
template <>
inline bool canConvert<uint32_t, double>(double val) { return val > -1. && val < 4294967296.; }
template <>
inline bool canConvert<int64_t, float>(float val) { return val >= -9223372036854775808.f && val < 9223372036854775808.f; }
template <>
inline bool canConvert<int64_t, double>(double val) { return val >= -9223372036854775808. && val < 9223372036854775808.; }
template <>
inline bool canConvert<uint64_t, float>(float val) { return val > -1.f && val < 18446744073709551616.f; }
template <>
inline bool canConvert<uint64_t, double>(double val) { return val > -1. && val < 18446744073709551616.; }

template <typename R, typename T>
R convert(T val)
{
    ASSERT((canConvert<R, T>(val)));
    return static_cast<R>(val);
}

template <>
inline float convert(double val)
{
    // The WebAssembly rounding mode means that these values (which are > float_MAX)
    // should be rounded to float_MAX and not set to infinity. Unfortunately, UBSAN
    // complains that the value is not representable as a float, so we'll special
    // case them.
    const double kMin = 3.4028234663852886e38;
    const double kMax = 3.4028235677973366e38;
    if (LIKELY(val >= -kMin && val <= kMin)) {
        return val;
    } else if (UNLIKELY(val > kMin && val < kMax)) {
        return std::numeric_limits<float>::max();
    } else if (UNLIKELY(val > -kMax && val < -kMin)) {
        return -std::numeric_limits<float>::max();
    } else if (UNLIKELY(std::isnan(val))) {
        return std::numeric_limits<float>::quiet_NaN();
    } else {
        return std::copysign(std::numeric_limits<float>::infinity(), val);
    }
}

#if defined(COMPILER_MSVC)
double convertUint64ToDouble(uint64_t x);
float convertUint64ToFloat(uint64_t x);
double convertInt64ToDouble(int64_t x);
float convertInt64ToFloat(int64_t x);
#else
inline double convertUint64ToDouble(uint64_t x)
{
    return static_cast<double>(x);
}

inline float convertUint64ToFloat(uint64_t x)
{
    return static_cast<float>(x);
}

inline double convertInt64ToDouble(int64_t x)
{
    return static_cast<double>(x);
}

inline float convertInt64ToFloat(int64_t x)
{
    return static_cast<float>(x);
}
#endif

template <>
inline float convert(uint64_t val)
{
    return convertUint64ToFloat(val);
}

template <>
inline double convert(uint64_t val)
{
    return convertUint64ToDouble(val);
}

template <>
inline float convert(int64_t val)
{
    return convertInt64ToFloat(val);
}

template <>
inline double convert(int64_t val)
{
    return convertInt64ToDouble(val);
}

template <typename T, int N>
T intExtend(ExecutionState& state, T val)
{
    // Hacker's delight 2.6 - sign extension
    auto bit = T{ 1 } << N;
    auto mask = (bit << 1) - 1;
    return ((val & mask) ^ bit) - bit;
}

template <typename R, typename T>
R intTruncSat(ExecutionState& state, T val)
{
    if (UNLIKELY(std::isnan(val))) {
        return 0;
    } else if (UNLIKELY(!canConvert<R>(val))) {
        return std::signbit(val) ? std::numeric_limits<R>::min()
                                 : std::numeric_limits<R>::max();
    } else {
        return static_cast<R>(val);
    }
}

template <typename T>
struct SatPromote;
template <>
struct SatPromote<int8_t> {
    using type = int32_t;
};
template <>
struct SatPromote<int16_t> {
    using type = int32_t;
};
template <>
struct SatPromote<uint8_t> {
    using type = int32_t;
};
template <>
struct SatPromote<uint16_t> {
    using type = int32_t;
};

template <typename R, typename T>
R saturate(T val)
{
    static_assert(sizeof(R) < sizeof(T), "Incorrect types for Saturate");
    const T min = std::numeric_limits<R>::min();
    const T max = std::numeric_limits<R>::max();
    return val > max ? max : val < min ? min : val;
}

template <typename T, typename U = typename SatPromote<T>::type>
T intAddSat(ExecutionState& state, T lhs, T rhs)
{
    return saturate<T, U>(lhs + rhs);
}

template <typename T, typename U = typename SatPromote<T>::type>
T intSubSat(ExecutionState& state, T lhs, T rhs)
{
    return saturate<T, U>(lhs - rhs);
}

template <typename T>
T saturatingRoundingQMul(ExecutionState& state, T lhs, T rhs)
{
    constexpr int size_in_bits = sizeof(T) * 8;
    int round_const = 1 << (size_in_bits - 2);
    int64_t product = lhs * rhs;
    product += round_const;
    product >>= (size_in_bits - 1);
    return saturate<T, int64_t>(product);
}

} // namespace Walrus

#endif
