# This can be a commit hash or tag
set(LIBUV_VERSION v1.44.2)

include(CMakeDependentOption)

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  list(APPEND uvwasi_cflags -fvisibility=hidden --std=gnu89)
  list(APPEND uvwasi_cflags -Wall -Wsign-compare -Wextra -Wstrict-prototypes)
  list(APPEND uvwasi_cflags -Wno-unused-parameter)

  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

  if(DEFINED WALRUS_ARCH)
    if(${WALRUS_ARCH} STREQUAL "x86")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    endif()
  endif()

  if(DEFINED WALRUS_MODE)
    if(${WALRUS_MODE} STREQUAL "debug")
      set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    endif()
  endif()
endif()

if(APPLE)
   set(CMAKE_MACOSX_RPATH ON)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND uvwasi_defines _GNU_SOURCE _POSIX_C_SOURCE=200112)
endif()

set(CMAKE_MESSAGE_LOG_LEVEL "WARNING")

include(FetchContent)
## https://libuv.org
FetchContent_Declare(
        libuv
        GIT_REPOSITORY https://github.com/libuv/libuv.git
        GIT_TAG ${LIBUV_VERSION})
FetchContent_GetProperties(libuv)
if(NOT libuv_POPULATED)
    FetchContent_Populate(libuv)
    include_directories("${libuv_SOURCE_DIR}/include")
    add_subdirectory(${libuv_SOURCE_DIR} ${libuv_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
set(LIBUV_INCLUDE_DIR ${libuv_SOURCE_DIR}/include)
set(LIBUV_LIBRARIES uv_a)

## uvwasi source code files.
set(uvwasi_sources
    third_party/uvwasi/src/clocks.c
    third_party/uvwasi/src/fd_table.c
    third_party/uvwasi/src/path_resolver.c
    third_party/uvwasi/src/poll_oneoff.c
    third_party/uvwasi/src/sync_helpers.c
    third_party/uvwasi/src/uv_mapping.c
    third_party/uvwasi/src/uvwasi.c
    third_party/uvwasi/src/wasi_rights.c
    third_party/uvwasi/src/wasi_serdes.c
)

option(UVWASI_DEBUG_LOG "Enable debug logging" OFF)
if(UVWASI_DEBUG_LOG)
    list(APPEND uvwasi_cflags -DUVWASI_DEBUG_LOG)
endif()

## Static library target.
add_library(uvwasi_a STATIC ${uvwasi_sources})
target_compile_definitions(uvwasi_a PRIVATE ${uvwasi_defines})
target_compile_options(uvwasi_a PRIVATE ${uvwasi_cflags})
target_include_directories(uvwasi_a PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(uvwasi_a PRIVATE ${LIBUV_LIBRARIES})

set(CMAKE_MESSAGE_LOG_LEVEL "DEFAULT")

message(STATUS "summary of uvwasi build options:
    Compiler:
      C compiler:    ${CMAKE_C_COMPILER}
      CFLAGS:        ${CMAKE_C_FLAGS_${_build_type}} ${CMAKE_C_FLAGS}

    LibUV libraries: ${LIBUV_LIBRARIES}
    LibUV includes:  ${LIBUV_INCLUDE_DIR}
    Debug logging:   ${UVWASI_DEBUG_LOG}
")
