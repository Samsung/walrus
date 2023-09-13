#
# Copyright 2016 WebAssembly Community Group participants
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.1.0)
project(WABT LANGUAGES C CXX VERSION 1.0.30)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check if wabt is being used directly or via add_subdirectory, FetchContent, etc
set(WABT_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(WABT_MASTER_PROJECT ON)
endif()

# By default use the project version as the version string
set(WABT_VERSION_STRING "${PROJECT_VERSION}")

option(USE_ASAN "Use address sanitizer" OFF)
option(USE_MSAN "Use memory sanitizer" OFF)
option(USE_LSAN "Use leak sanitizer" OFF)
option(USE_UBSAN "Use undefined behavior sanitizer" OFF)
option(WERROR "Build with warnings as errors" OFF)
# WASI support is still a work in progress.
# Only a handful of syscalls are supported at this point.
option(WITH_WASI "Build WASI support via uvwasi" OFF)

if (MSVC)
  set(COMPILER_IS_CLANG 0)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 1)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(COMPILER_IS_CLANG 1)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 0)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(COMPILER_IS_CLANG 0)
  set(COMPILER_IS_GNU 1)
  set(COMPILER_IS_MSVC 0)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(COMPILER_IS_CLANG 1)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 0)
else ()
  set(COMPILER_IS_CLANG 0)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 0)
endif ()

include(CheckIncludeFile)
include(CheckSymbolExists)

check_include_file("alloca.h" HAVE_ALLOCA_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_symbol_exists(snprintf "stdio.h" HAVE_SNPRINTF)
check_symbol_exists(strcasecmp "strings.h" HAVE_STRCASECMP)

if (WIN32)
  check_symbol_exists(ENABLE_VIRTUAL_TERMINAL_PROCESSING "windows.h" HAVE_WIN32_VT100)
endif ()

include(CheckTypeSize)
check_type_size(ssize_t SSIZE_T)
check_type_size(size_t SIZEOF_SIZE_T)

configure_file(src/config.h.in include/wabt/config.h @ONLY)

include_directories(${WABT_SOURCE_DIR} ${WABT_BINARY_DIR})

if (COMPILER_IS_MSVC)
  if (WERROR)
    add_definitions(-WX)
  endif ()

  # disable warning C4018: signed/unsigned mismatch
  # disable warning C4056, C4756: overflow in floating-point constant arithmetic
  #   seems to not like float compare w/ HUGE_VALF; bug?
  # disable warnings C4267 and C4244: conversion/truncation from larger to smaller type.
  # disable warning C4800: implicit conversion from larger int to bool
  add_definitions(-W3 -wd4018 -wd4056 -wd4756 -wd4267 -wd4244 -wd4800 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)

  # multi-core build.
  add_definitions("/MP")

else ()
  if (WERROR)
    add_definitions(-Werror)
  endif ()

  # disable -Wunused-parameter: this is really common when implementing
  #   interfaces, etc.
  # disable -Wpointer-arith: this is a GCC extension, and doesn't work in MSVC.
  add_definitions(
    -Wall -Wextra -Wno-unused-parameter -Wpointer-arith -Wuninitialized
  )

  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast")

  if (NOT WITH_EXCEPTIONS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  endif ()

  # Need to define __STDC_*_MACROS because C99 specifies that C++ shouldn't
  # define format (e.g. PRIu64) or limit (e.g. UINT32_MAX) macros without the
  # definition, and some libcs (e.g. glibc2.17 and earlier) follow that.
  add_definitions(-D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1)

  if (MINGW OR CYGWIN)
    # On MINGW, _POSIX_C_SOURCE is needed to ensure we use mingw printf
    # instead of the VC runtime one.
    add_definitions(-D_POSIX_C_SOURCE=200809L)
  endif()

  if (COMPILER_IS_GNU)
    # disable -Wclobbered: it seems to be guessing incorrectly about a local
    # variable being clobbered by longjmp.
    add_definitions(-Wno-clobbered)
  endif ()
endif ()

set(USE_SANITIZER FALSE)

function(sanitizer NAME FLAGS)
  if (${NAME})
    if (USE_SANITIZER)
      message(FATAL_ERROR "Only one sanitizer allowed")
    endif ()
    set(USE_SANITIZER TRUE PARENT_SCOPE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}" PARENT_SCOPE)
  endif ()
endfunction()
sanitizer(USE_ASAN "-fsanitize=address")
sanitizer(USE_MSAN "-fsanitize=memory")
sanitizer(USE_LSAN "-fsanitize=leak")

if (USE_UBSAN)
  # -fno-sanitize-recover was deprecated, see if we are compiling with a newer
  # clang that requires -fno-sanitize-recover=all.
  set(UBSAN_BLACKLIST ${WABT_SOURCE_DIR}/ubsan.blacklist)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-fsanitize=undefined -fno-sanitize-recover -Wall -Werror" HAS_UBSAN_RECOVER_BARE)
  if (HAS_UBSAN_RECOVER_BARE)
    sanitizer(USE_UBSAN "-fsanitize=undefined -fno-sanitize-recover -fsanitize-blacklist=${UBSAN_BLACKLIST}")
  endif ()
  check_cxx_compiler_flag("-fsanitize=undefined -fno-sanitize-recover=all -Wall -Werror" HAS_UBSAN_RECOVER_ALL)
  # If we already setup UBSAN recover bare, setting it up again here will be an error.
  if (NOT USE_SANITIZER AND HAS_UBSAN_RECOVER_ALL)
    sanitizer(USE_UBSAN "-fsanitize=undefined -fno-sanitize-recover=all -fsanitize-blacklist=${UBSAN_BLACKLIST}")
  endif ()
  if (NOT USE_SANITIZER)
    message(FATAL_ERROR "UBSAN is not supported")
  endif ()
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${WABT_SOURCE_DIR}/cmake)

add_custom_target(everything)

set(WABT_LIBRARY_SRC
  src/binary-reader-logging.cc
  src/binary-reader.cc
  src/binary.cc
  src/binding-hash.cc
  src/common.cc
  src/config.cc # for window build
  src/expr-visitor.cc
  src/feature.cc
  src/leb128.cc
  src/opcode-code-table.c
  src/opcode.cc
  src/option-parser.cc
  src/shared-validator.cc
  src/stream.cc
  src/type-checker.cc
  src/utf8.cc

  src/walrus/binary-reader-walrus.cc

  # files for testing
  src/binary-reader-ir.cc
  src/binary-writer.cc
  src/ir.cc
  src/lexer-source.cc
  src/literal.cc
  src/resolve-names.cc
  src/token.cc

  src/wast-lexer.cc
  src/wast-parser.cc
)

if (COMPILER_IS_MSVC)
  set(WABT_CXX_FLAGS_INTERNAL ${WABT_CXX_FLAGS_INTERNAL} /fp:strict /Oy-)
else ()
  if (WABT_ARCH STREQUAL "x86")
    # wasm doesn't allow for x87 floating point math
    set(WABT_CXX_FLAGS_INTERNAL ${WABT_CXX_FLAGS_INTERNAL} -msse2 -mfpmath=sse)
  endif ()
endif()

add_compile_options(${WABT_CXX_FLAGS_INTERNAL})
add_compile_options(${WABT_CXX_FLAGS})

set(WABT_CXXFLAGS_FROM_ENV $ENV{CXXFLAGS})
separate_arguments(WABT_CXXFLAGS_FROM_ENV)
add_compile_options(${WABT_CXXFLAGS_FROM_ENV})

add_library(wabt STATIC ${WABT_LIBRARY_SRC})
target_include_directories(wabt PUBLIC ${PROJECT_SOURCE_DIR}/include "$<BUILD_INTERFACE:${WABT_BINARY_DIR}/include>")
TARGET_COMPILE_DEFINITIONS (wabt PRIVATE ${WABT_DEFINITIONS})
