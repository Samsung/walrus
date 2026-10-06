cmake_minimum_required(VERSION 2.8.12...4.0)
project(uvwasi C)

set(UVWASI_DIR ${PROJECT_SOURCE_DIR}/uvwasi)

# This can be a commit hash or tag
set(LIBUV_VERSION v1.47.0)

include(CMakeDependentOption)
cmake_dependent_option(UVWASI_BUILD_TESTS
  "Build the unit tests when uvwasi is the root project" ON
  "CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)


if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  list(APPEND uvwasi_cflags -fvisibility=hidden --std=gnu89)
  list(APPEND uvwasi_cflags -Wall -Wsign-compare -Wextra -Wstrict-prototypes)
  list(APPEND uvwasi_cflags -Wno-unused-parameter)

  if (UVWASI_BUILD_X86)
    # set x86 build flag
    list(APPEND uvwasi_cflags -m32)
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND uvwasi_defines _GNU_SOURCE _POSIX_C_SOURCE=200112)
endif()

if (CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    list(APPEND uvwasi_cflags "$<$<CONFIG:Debug>:-O0>" "$<$<CONFIG:Debug>:-g>")

    if (UVWASI_DEBUG_LOG)
        list(APPEND uvwasi_defines UVWASI_DEBUG_LOG)
    endif ()
endif ()

# Resolve pkg-config libraries to full paths instead of modifying directory
# search paths. A legacy x86 override on a 64-bit toolchain must build libuv
# for x86 too; it cannot use the host's system library.
set(UVWASI_USE_SYSTEM_LIBUV ON)
if (UVWASI_BUILD_X86 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(UVWASI_USE_SYSTEM_LIBUV OFF)
endif ()
if (CMAKE_CROSSCOMPILING AND "$ENV{PKG_CONFIG_LIBDIR}" STREQUAL ""
    AND "$ENV{PKG_CONFIG_SYSROOT_DIR}" STREQUAL "")
  # Host pkg-config defaults are unsuitable for a cross-compilation target.
  set(UVWASI_USE_SYSTEM_LIBUV OFF)
endif ()
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND AND UVWASI_USE_SYSTEM_LIBUV)
  pkg_check_modules(UVWASI_SYSTEM_LIBUV QUIET libuv>=1.34)
endif ()
if (UVWASI_USE_SYSTEM_LIBUV AND UVWASI_SYSTEM_LIBUV_FOUND)
  set(UVWASI_LIBUV_INCLUDE_DIRS ${UVWASI_SYSTEM_LIBUV_INCLUDE_DIRS})
  set(UVWASI_LIBUV_LIBRARIES)
  foreach(LIBRARY ${UVWASI_SYSTEM_LIBUV_LIBRARIES})
    find_library(UVWASI_LIBUV_${LIBRARY} NAMES ${LIBRARY}
                 HINTS ${UVWASI_SYSTEM_LIBUV_LIBRARY_DIRS})
    if (NOT UVWASI_LIBUV_${LIBRARY})
      message(FATAL_ERROR "Cannot find libuv dependency: ${LIBRARY}")
    endif ()
    list(APPEND UVWASI_LIBUV_LIBRARIES ${UVWASI_LIBUV_${LIBRARY}})
  endforeach()
  list(APPEND uvwasi_cflags ${UVWASI_SYSTEM_LIBUV_CFLAGS_OTHER})
else ()
  if (CMAKE_VERSION VERSION_LESS 3.14)
    message(FATAL_ERROR "WASI on CMake older than 3.14 requires system libuv for the target architecture (pkg-config libuv), or WALRUS_WASI=OFF")
  endif ()
  include(FetchContent)
  FetchContent_Declare(libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG ${LIBUV_VERSION})
  FetchContent_MakeAvailable(libuv)
  set_target_properties(uv_a PROPERTIES POSITION_INDEPENDENT_CODE ON)
  if (UVWASI_BUILD_X86)
    set_property(TARGET uv_a APPEND PROPERTY COMPILE_OPTIONS -m32)
  endif ()
  set(UVWASI_LIBUV_INCLUDE_DIRS ${libuv_SOURCE_DIR}/include)
  set(UVWASI_LIBUV_LIBRARIES uv_a)
endif ()

## uvwasi source code files.
set(uvwasi_sources
    ${UVWASI_DIR}/src/clocks.c
    ${UVWASI_DIR}/src/fd_table.c
    ${UVWASI_DIR}/src/path_resolver.c
    ${UVWASI_DIR}/src/poll_oneoff.c
    ${UVWASI_DIR}/src/sync_helpers.c
    ${UVWASI_DIR}/src/uv_mapping.c
    ${UVWASI_DIR}/src/uvwasi.c
    ${UVWASI_DIR}/src/wasi_rights.c
    ${UVWASI_DIR}/src/wasi_serdes.c
)

# Keep ASAN as a read-only legacy alias; only the prefixed option is cached.
set(_uvwasi_asan_default OFF)
if (DEFINED ASAN)
  set(_uvwasi_asan_default ${ASAN})
elseif (DEFINED WALRUS_ASAN)
  set(_uvwasi_asan_default ${WALRUS_ASAN})
endif ()
if (DEFINED UVWASI_ASAN)
  set(UVWASI_ASAN "${UVWASI_ASAN}" CACHE BOOL "Enable uvwasi AddressSanitizer")
endif ()
option(UVWASI_ASAN "Enable uvwasi AddressSanitizer" ${_uvwasi_asan_default})
if (UVWASI_ASAN AND CMAKE_C_COMPILER_ID MATCHES "AppleClang|GNU|Clang")
  list(APPEND uvwasi_cflags -fno-omit-frame-pointer -fsanitize=address)
endif ()

## Static library target.
add_library(uvwasi_a STATIC ${uvwasi_sources})
set_target_properties(uvwasi_a PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(uvwasi_a PRIVATE ${uvwasi_defines})
target_compile_options(uvwasi_a PRIVATE ${uvwasi_cflags})
target_include_directories(uvwasi_a PUBLIC ${UVWASI_DIR}/include ${UVWASI_LIBUV_INCLUDE_DIRS})
target_link_libraries(uvwasi_a PRIVATE ${UVWASI_LIBUV_LIBRARIES})
if (UVWASI_ASAN AND CMAKE_C_COMPILER_ID MATCHES "AppleClang|GNU|Clang")
  target_link_libraries(uvwasi_a PRIVATE -fsanitize=address)
endif ()

message(STATUS "summary of uvwasi build options:

    Install prefix:  ${CMAKE_INSTALL_PREFIX}
    Target system:   ${CMAKE_SYSTEM_NAME}
    Compiler:
      C compiler:    ${CMAKE_C_COMPILER}
      CFLAGS:        ${CMAKE_C_FLAGS_${_build_type}} ${CMAKE_C_FLAGS}

    LibUV libraries: ${UVWASI_LIBUV_LIBRARIES}
    LibUV includes:  ${UVWASI_LIBUV_INCLUDE_DIRS}
    Debug logging:   ${UVWASI_DEBUG_LOG}
    Code coverage:   ${CODE_COVERAGE}
    ASAN:            ${UVWASI_ASAN}
    Build tests:     ${UVWASI_BUILD_TESTS}
")
