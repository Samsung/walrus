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

cmake_minimum_required(VERSION 2.8.12...4.0)
project(WABT C CXX)
set(WABT_VERSION_STRING 1.0.30)
set(WABT_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(WABT_MASTER_PROJECT ON)
endif ()

# Read legacy inputs without adding unprefixed options to the shared cache.
function(wabt_option NAME LEGACY HELP DEFAULT)
  if (DEFINED ${NAME})
    set(${NAME} "${${NAME}}" CACHE BOOL "${HELP}")
  elseif (DEFINED ${LEGACY})
    set(${NAME} "${${LEGACY}}" CACHE BOOL "${HELP}")
  endif ()
  option(${NAME} "${HELP}" ${DEFAULT})
endfunction()
wabt_option(WABT_USE_ASAN USE_ASAN "Use address sanitizer" OFF)
wabt_option(WABT_USE_MSAN USE_MSAN "Use memory sanitizer" OFF)
wabt_option(WABT_USE_LSAN USE_LSAN "Use leak sanitizer" OFF)
wabt_option(WABT_USE_UBSAN USE_UBSAN "Use undefined behavior sanitizer" OFF)
wabt_option(WABT_WERROR WERROR "Build WABT with warnings as errors" OFF)
wabt_option(WABT_WITH_EXCEPTIONS WITH_EXCEPTIONS "Enable C++ exceptions in WABT" OFF)

set(WABT_COMPILE_OPTIONS)
set(WABT_COMPILE_DEFINITIONS ${WABT_DEFINITIONS})
set(WABT_CXX_ONLY_OPTIONS)
if (MSVC)
  set(WABT_COMPILER_IS_CLANG 0)
  set(WABT_COMPILER_IS_GNU 0)
  set(WABT_COMPILER_IS_MSVC 1)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(WABT_COMPILER_IS_CLANG 1)
  set(WABT_COMPILER_IS_GNU 0)
  set(WABT_COMPILER_IS_MSVC 0)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(WABT_COMPILER_IS_CLANG 0)
  set(WABT_COMPILER_IS_GNU 1)
  set(WABT_COMPILER_IS_MSVC 0)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(WABT_COMPILER_IS_CLANG 1)
  set(WABT_COMPILER_IS_GNU 0)
  set(WABT_COMPILER_IS_MSVC 0)
else ()
  set(WABT_COMPILER_IS_CLANG 0)
  set(WABT_COMPILER_IS_GNU 0)
  set(WABT_COMPILER_IS_MSVC 0)
endif ()

# Prefix cached probe results. Generic names needed by CheckTypeSize and
# config.h are confined to this function, so other projects cannot reuse them.
function(wabt_configure_header)
  if (WABT_ARCH STREQUAL "x86" AND (WABT_COMPILER_IS_GNU OR WABT_COMPILER_IS_CLANG))
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -m32")
  endif ()
  include(CheckIncludeFile)
  include(CheckSymbolExists)
  check_include_file("alloca.h" WABT_HAVE_ALLOCA_H)
  check_include_file("unistd.h" WABT_HAVE_UNISTD_H)
  check_symbol_exists(snprintf "stdio.h" WABT_HAVE_SNPRINTF)
  check_symbol_exists(strcasecmp "strings.h" WABT_HAVE_STRCASECMP)
  if (WIN32)
    check_symbol_exists(ENABLE_VIRTUAL_TERMINAL_PROCESSING "windows.h" WABT_HAVE_WIN32_VT100)
  endif ()
  check_include_file("sys/types.h" WABT_HAVE_SYS_TYPES_H)
  check_include_file("stdint.h" WABT_HAVE_STDINT_H)
  check_include_file("stddef.h" WABT_HAVE_STDDEF_H)
  set(HAVE_SYS_TYPES_H ${WABT_HAVE_SYS_TYPES_H})
  set(HAVE_STDINT_H ${WABT_HAVE_STDINT_H})
  set(HAVE_STDDEF_H ${WABT_HAVE_STDDEF_H})
  include(CheckTypeSize)
  check_type_size(ssize_t WABT_SSIZE_T)
  check_type_size(size_t WABT_SIZEOF_SIZE_T)
  foreach(NAME ALLOCA_H UNISTD_H SNPRINTF STRCASECMP WIN32_VT100)
    set(HAVE_${NAME} "${WABT_HAVE_${NAME}}")
  endforeach()
  set(HAVE_SSIZE_T ${HAVE_WABT_SSIZE_T})
  set(SIZEOF_SIZE_T ${WABT_SIZEOF_SIZE_T})
  foreach(NAME CLANG GNU MSVC)
    set(COMPILER_IS_${NAME} ${WABT_COMPILER_IS_${NAME}})
  endforeach()
  set(WITH_EXCEPTIONS ${WABT_WITH_EXCEPTIONS})
  configure_file(${WABT_SOURCE_DIR}/src/config.h.in
                 ${WABT_BINARY_DIR}/include/wabt/config.h @ONLY)
endfunction()
wabt_configure_header()

if (WABT_COMPILER_IS_MSVC)
  if (WABT_WERROR)
    list(APPEND WABT_COMPILE_OPTIONS -WX)
  endif ()

  # disable warning C4018: signed/unsigned mismatch
  # disable warning C4056, C4756: overflow in floating-point constant arithmetic
  #   seems to not like float compare w/ HUGE_VALF; bug?
  # disable warnings C4267 and C4244: conversion/truncation from larger to smaller type.
  # disable warning C4800: implicit conversion from larger int to bool
  list(APPEND WABT_COMPILE_OPTIONS -W3 -wd4018 -wd4056 -wd4267 -wd4244 -wd4800)
  list(APPEND WABT_COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS)

  # multi-core build.
  list(APPEND WABT_COMPILE_OPTIONS /MP)

else ()
  if (WABT_WERROR)
    list(APPEND WABT_COMPILE_OPTIONS -Werror)
  endif ()

  # disable -Wunused-parameter: this is really common when implementing
  #   interfaces, etc.
  # disable -Wpointer-arith: this is a GCC extension, and doesn't work in MSVC.
  list(APPEND WABT_COMPILE_OPTIONS
    -Wall -Wextra -Wno-unused-parameter -Wpointer-arith -Wuninitialized
  )

  list(APPEND WABT_CXX_ONLY_OPTIONS -Wold-style-cast)

  if (NOT WABT_WITH_EXCEPTIONS)
    list(APPEND WABT_CXX_ONLY_OPTIONS -fno-exceptions)
  endif ()

  # Need to define __STDC_*_MACROS because C99 specifies that C++ shouldn't
  # define format (e.g. PRIu64) or limit (e.g. UINT32_MAX) macros without the
  # definition, and some libcs (e.g. glibc2.17 and earlier) follow that.
  list(APPEND WABT_COMPILE_DEFINITIONS __STDC_LIMIT_MACROS=1 __STDC_FORMAT_MACROS=1)

  if (MINGW OR CYGWIN)
    # On MINGW, _POSIX_C_SOURCE is needed to ensure we use mingw printf
    # instead of the VC runtime one.
    list(APPEND WABT_COMPILE_DEFINITIONS _POSIX_C_SOURCE=200809L)
  endif()

  if (WABT_COMPILER_IS_GNU)
    # disable -Wclobbered: it seems to be guessing incorrectly about a local
    # variable being clobbered by longjmp.
    list(APPEND WABT_COMPILE_OPTIONS -Wno-clobbered)
  endif ()
endif ()

set(WABT_SANITIZER_OPTIONS)
macro(wabt_sanitizer NAME)
  if (${NAME})
    if (WABT_SANITIZER_OPTIONS)
      message(FATAL_ERROR "Only one WABT sanitizer is allowed")
    endif ()
    list(APPEND WABT_SANITIZER_OPTIONS ${ARGN})
  endif ()
endmacro()
wabt_sanitizer(WABT_USE_ASAN -fsanitize=address)
wabt_sanitizer(WABT_USE_MSAN -fsanitize=memory)
wabt_sanitizer(WABT_USE_LSAN -fsanitize=leak)
if (WABT_USE_UBSAN)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-fsanitize=undefined -fno-sanitize-recover=all" WABT_HAS_UBSAN_RECOVER_ALL)
  if (WABT_HAS_UBSAN_RECOVER_ALL)
    wabt_sanitizer(WABT_USE_UBSAN -fsanitize=undefined -fno-sanitize-recover=all)
  else ()
    check_cxx_compiler_flag("-fsanitize=undefined -fno-sanitize-recover" WABT_HAS_UBSAN_RECOVER_BARE)
    if (NOT WABT_HAS_UBSAN_RECOVER_BARE)
      message(FATAL_ERROR "UBSAN is not supported")
    endif ()
    wabt_sanitizer(WABT_USE_UBSAN -fsanitize=undefined -fno-sanitize-recover)
  endif ()
endif ()

set(WABT_LIBRARY_SRC
  src/binary-reader-logging.cc
  src/binary-reader.cc
  src/binary.cc
  src/binding-hash.cc
  src/common.cc
  src/component.cc
  src/config.cc # for windows build
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

if (WABT_COMPILER_IS_MSVC)
  set(WABT_CXX_FLAGS_INTERNAL ${WABT_CXX_FLAGS_INTERNAL} /fp:strict /Oy-)
else ()
  if (WABT_ARCH STREQUAL "x86")
    # wasm doesn't allow for x87 floating point math
    set(WABT_CXX_FLAGS_INTERNAL ${WABT_CXX_FLAGS_INTERNAL} -msse2 -mfpmath=sse)
  endif ()
endif()

add_library(wabt STATIC ${WABT_LIBRARY_SRC})
set_target_properties(wabt PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
if (CMAKE_VERSION VERSION_LESS 3.1 AND NOT WABT_COMPILER_IS_MSVC)
  list(APPEND WABT_CXX_ONLY_OPTIONS -std=c++11)
endif ()
if (CMAKE_VERSION VERSION_LESS 3.3)
  # COMPILE_LANGUAGE expressions were added in 3.3. Older CMake needs
  # C++-only options on this target's C++ source files instead.
  string(REPLACE ";" " " WABT_CXX_SOURCE_FLAGS "${WABT_CXX_ONLY_OPTIONS}")
  foreach(SOURCE ${WABT_LIBRARY_SRC})
    if (SOURCE MATCHES "\\.cc$")
      set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "${WABT_CXX_SOURCE_FLAGS}")
    endif ()
  endforeach()
else ()
  foreach(FLAG ${WABT_CXX_ONLY_OPTIONS})
    target_compile_options(wabt PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${FLAG}>")
  endforeach()
endif ()
target_compile_options(wabt PRIVATE ${WABT_COMPILE_OPTIONS}
  ${WABT_CXX_FLAGS_INTERNAL} ${WABT_CXX_FLAGS} ${WABT_SANITIZER_OPTIONS})
target_compile_definitions(wabt PRIVATE ${WABT_COMPILE_DEFINITIONS})
target_include_directories(wabt PRIVATE ${WABT_SOURCE_DIR} ${WABT_BINARY_DIR}
  PUBLIC ${WABT_SOURCE_DIR}/include "$<BUILD_INTERFACE:${WABT_BINARY_DIR}/include>")
# Static-library users need the sanitizer runtime when linking, but unrelated
# targets must not inherit sanitizer compile flags.
target_link_libraries(wabt PRIVATE ${WABT_SANITIZER_OPTIONS})
if (WABT_MASTER_PROJECT)
  add_custom_target(everything DEPENDS wabt)
endif ()
