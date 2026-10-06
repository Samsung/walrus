cmake_minimum_required(VERSION 2.8.12...4.0)
project(uvwasi C)

set(UVWASI_DIR ${PROJECT_SOURCE_DIR}/uvwasi)

# Point CMake at any custom modules we may ship
list(APPEND CMAKE_MODULE_PATH "${UVWASI_DIR}/cmake")

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
    add_compile_options( -m32 )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
  endif()
endif()

if(APPLE)
   set(CMAKE_MACOSX_RPATH ON)
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

# Old CMake can use a system libuv; newer CMake can fetch the pinned version.
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(LIBUV QUIET libuv>=1.34)
endif ()
if (LIBUV_FOUND)
  link_directories(${LIBUV_LIBRARY_DIRS})
  list(APPEND uvwasi_cflags ${LIBUV_CFLAGS_OTHER})
else ()
  if (CMAKE_VERSION VERSION_LESS 3.14)
    message(FATAL_ERROR "WASI on CMake older than 3.14 requires system libuv (pkg-config libuv), or WALRUS_WASI=OFF")
  endif ()
  include(FetchContent)
  FetchContent_Declare(libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG ${LIBUV_VERSION})
  FetchContent_MakeAvailable(libuv)
  set(LIBUV_INCLUDE_DIRS ${libuv_SOURCE_DIR}/include)
  set(LIBUV_LIBRARIES uv_a)
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

# ASAN Support
option(ASAN "Enable code asan" OFF)
if(ASAN AND CMAKE_C_COMPILER_ID MATCHES "AppleClang|GNU|Clang")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

## Static library target.
add_library(uvwasi_a STATIC ${uvwasi_sources})
target_compile_definitions(uvwasi_a PRIVATE ${uvwasi_defines})
target_compile_options(uvwasi_a PRIVATE ${uvwasi_cflags})
target_include_directories(uvwasi_a PUBLIC ${UVWASI_DIR}/include ${LIBUV_INCLUDE_DIRS})
target_link_libraries(uvwasi_a PRIVATE ${LIBUV_LIBRARIES})

message(STATUS "summary of uvwasi build options:

    Install prefix:  ${CMAKE_INSTALL_PREFIX}
    Target system:   ${CMAKE_SYSTEM_NAME}
    Compiler:
      C compiler:    ${CMAKE_C_COMPILER}
      CFLAGS:        ${CMAKE_C_FLAGS_${_build_type}} ${CMAKE_C_FLAGS}

    LibUV libraries: ${LIBUV_LIBRARIES}
    LibUV includes:  ${LIBUV_INCLUDE_DIRS}
    Debug logging:   ${UVWASI_DEBUG_LOG}
    Code coverage:   ${CODE_COVERAGE}
    ASAN:            ${ASAN}
    Build tests:     ${UVWASI_BUILD_TESTS}
")
