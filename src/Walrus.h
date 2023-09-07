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

#ifndef __Walrus__
#define __Walrus__

/* COMPILER() - the compiler being used to build the project */
// Outdated syntax: use defined()
// #define COMPILER(FEATURE) (defined COMPILER_##FEATURE && COMPILER_##FEATURE)

#if defined(__clang__)
#define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif (__GNUC__)
#define COMPILER_GCC 1
#else
#error "Compiler dectection failed"
#endif

#if defined(COMPILER_CLANG)
/* Check clang version */
#if __clang_major__ < 6
#error "Clang version is low (require clang version 6+)"
#endif
/* Keep strong enums turned off when building with clang-cl: We cannot yet build
 * all of Blink without fallback to cl.exe, and strong enums are exposed at ABI
 * boundaries. */
#undef COMPILER_SUPPORTS_CXX_STRONG_ENUMS
#else
#define COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define COMPILER_QUIRK_FINAL_IS_CALLED_SEALED 1
#endif

/* ALWAYS_INLINE */
#ifndef ALWAYS_INLINE
#if (defined(COMPILER_GCC) || defined(COMPILER_CLANG)) && !defined(COMPILER_MINGW)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(COMPILER_MSVC) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif

/* NEVER_INLINE */
#ifndef NEVER_INLINE
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define NEVER_INLINE __attribute__((__noinline__))
#else
#define NEVER_INLINE
#endif
#endif

/* UNLIKELY */
#ifndef UNLIKELY
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define UNLIKELY(x) (x)
#endif
#endif

/* LIKELY */
#ifndef LIKELY
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define LIKELY(x) __builtin_expect((x), 1)
#else
#define LIKELY(x) (x)
#endif
#endif

/* NO_RETURN */
#ifndef NO_RETURN
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define NO_RETURN __attribute((__noreturn__))
#elif defined(COMPILER_MSVC)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN
#endif
#endif

/* EXPORT */
#ifndef EXPORT
#if defined(COMPILER_MSVC)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#endif

/* PREFETCH_READ */
#ifndef PREFETCH_READ
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define PREFETCH_READ(x) __builtin_prefetch((x), 0, 0)
#else
#define PREFETCH_READ(x)
#endif
#endif

/* LOG2 */
#ifndef FAST_LOG2_UINT
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define FAST_LOG2_UINT(x) \
    ((unsigned)(8 * sizeof(unsigned long long) - __builtin_clzll((x)) - 1))
#else
#define FAST_LOG2_UINT(x) log2l(x)
#endif
#endif

#if defined(COMPILER_MSVC)
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#ifndef ATTRIBUTE_NO_SANITIZE_ADDRESS
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif
#endif

// #define OS(NAME) (defined OS_##NAME && OS_##NAME)

#ifdef _WIN32
#define OS_WINDOWS 1
#elif _WIN64
#define OS_WINDOWS 1
#elif __APPLE__
#define OS_DARWIN 1
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
#define OS_POSIX 1
#elif TARGET_OS_IPHONE
#define OS_POSIX 1
#elif TARGET_OS_MAC
#define OS_POSIX 1
#else
#error "Unknown Apple platform"
#endif
#elif __linux__
#define OS_POSIX 1
#elif __unix__ // all unices not caught above
#define OS_POSIX 1
#elif defined(_POSIX_VERSION)
#define OS_POSIX 1
#else
#error "failed to detect target OS"
#endif

#if defined(OS_WINDOWS)
#define NOMINMAX
#endif

/*
we need to mark enum as unsigned if needs.
because processing enum in msvc is little different

ex) enum Type { A, B };
struct Foo { Type type: 1; };
Foo f; f.type = 1;
if (f.type == Type::B) { puts("failed in msvc."); }
*/
#if defined(COMPILER_MSVC)
#define ENSURE_ENUM_UNSIGNED : unsigned int
#else
#define ENSURE_ENUM_UNSIGNED
#endif

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define HAVE_BUILTIN_ATOMIC_FUNCTIONS
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define CPU_X86_64

#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__IA32__) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__) || defined(__386)
#define CPU_X86

#elif defined(__arm__) || defined(__thumb__) || defined(_ARM) || defined(_M_ARM) || defined(_M_ARMT) || defined(__arm) || defined(__arm)
#define CPU_ARM32

#elif defined(__aarch64__)
#define CPU_ARM64

#else
#error "Could't find cpu arch."
#endif

#include <algorithm>
#include <cassert>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <vector>

#if defined(COMPILER_MSVC)
#include <stddef.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define WALRUS_LOG_INFO(...) fprintf(stdout, __VA_ARGS__);
#define WALRUS_LOG_ERROR(...) fprintf(stderr, __VA_ARGS__);

#ifndef CRASH
#define CRASH ASSERT_NOT_REACHED
#endif

#if defined(NDEBUG)
#define ASSERT(assertion) ((void)0)
#define ASSERT_NOT_REACHED() ((void)0)
#define ASSERT_STATIC(assertion, reason)
#else
#define ASSERT(assertion) assert(assertion);
#define ASSERT_NOT_REACHED() \
    do {                     \
        assert(false);       \
    } while (0)
#define ASSERT_STATIC(assertion, reason) static_assert(assertion, reason)
#endif

/* COMPILE_ASSERT */
#ifndef COMPILE_ASSERT
#define COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif

#define RELEASE_ASSERT(assertion)                                                \
    do {                                                                         \
        if (!(assertion)) {                                                      \
            WALRUS_LOG_ERROR("RELEASE_ASSERT at %s (%d)\n", __FILE__, __LINE__); \
            abort();                                                             \
        }                                                                        \
    } while (0);
#define RELEASE_ASSERT_NOT_REACHED()                                          \
    do {                                                                      \
        WALRUS_LOG_ERROR("RELEASE_ASSERT_NOT_REACHED at %s (%d)\n", __FILE__, \
                         __LINE__);                                           \
        abort();                                                              \
    } while (0)

#if !defined(WARN_UNUSED_RETURN) && (defined(COMPILER_GCC) || defined(COMPILER_CLANG))
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

/* UNUSED_PARAMETER */

#if !defined(UNUSED_PARAMETER) && defined(COMPILER_MSVC)
#define UNUSED_PARAMETER(variable) (void)&variable
#endif

#if !defined(UNUSED_PARAMETER)
#define UNUSED_PARAMETER(variable) (void)variable
#endif

/* UNUSED_VARIABLE */

#if !defined(UNUSED_VARIABLE)
#define UNUSED_VARIABLE(variable) UNUSED_PARAMETER(variable)
#endif

#if !defined(FALLTHROUGH) && defined(COMPILER_GCC)
#if __GNUC__ >= 7
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH /* fall through */
#endif
#elif !defined(FALLTHROUGH) && defined(COMPILER_CLANG)
#define FALLTHROUGH /* fall through */
#else
#define FALLTHROUGH
#endif

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER == __LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__) || defined(__i386) || defined(_M_IX86) || defined(__ia64) || defined(__ia64__) || defined(_M_IA64) || defined(_M_IX86) || defined(_M_X64) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(ANDROID)
#define WALRUS_LITTLE_ENDIAN
// #pragma message "little endian"
#elif (defined(__BYTE_ORDER__) && __BYTE_ORDER == __BIG_ENDIAN) || defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define WALRUS_BIG_ENDIAN
// #pragma message "big endian"
#else
#error "I don't know what architecture this is!"
#endif

#if defined(COMPILER_MSVC)
#define MAY_THREAD_LOCAL __declspec(thread)
#else
#define MAY_THREAD_LOCAL __thread
#endif

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define WALRUS_ENABLE_COMPUTED_GOTO
// some devices cannot support getting label address from outside well
#if (defined(CPU_ARM64) || (defined(CPU_ARM32) && defined(COMPILER_CLANG))) || defined(OS_DARWIN) || defined(OS_ANDROID) || defined(OS_WINDOWS)
#define WALRUS_COMPUTED_GOTO_INTERPRETER_INIT_WITH_NULL
#endif
#endif

#define MAKE_STACK_ALLOCATED()                    \
    static void* operator new(size_t) = delete;   \
    static void* operator new[](size_t) = delete; \
    static void operator delete(void*) = delete;  \
    static void operator delete[](void*) = delete;

#define ALLOCA(Type, Result, Bytes)                                                             \
    std::unique_ptr<uint8_t[]> Result##HolderWhenUsingMalloc;                                   \
    size_t bytes##Result = (Bytes);                                                             \
    Type* Result;                                                                               \
    if (LIKELY(bytes##Result < 2048)) {                                                         \
        Result = (Type*)alloca(bytes##Result);                                                  \
    } else {                                                                                    \
        Result##HolderWhenUsingMalloc = std::unique_ptr<uint8_t[]>(new uint8_t[bytes##Result]); \
        Result = (Type*)Result##HolderWhenUsingMalloc.get();                                    \
    }

#if !defined(STACK_GROWS_DOWN) && !defined(STACK_GROWS_UP)
#define STACK_GROWS_DOWN
#endif

#ifndef STACK_LIMIT_FROM_BASE
// FIXME reduce stack limit to 3MB
#define STACK_LIMIT_FROM_BASE (1024 * 1024 * 5) // 5MB
#endif

#include "util/Optional.h"
namespace Walrus {
typedef uint16_t ByteCodeStackOffset;
}

#endif
