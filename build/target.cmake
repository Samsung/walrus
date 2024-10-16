# default set of each flag
SET (WALRUS_CXXFLAGS)
SET (WALRUS_CXXFLAGS_DEBUG)
SET (WALRUS_CXXFLAGS_RELEASE)
SET (WALRUS_LDFLAGS)
SET (WALRUS_DEFINITIONS)
SET (WALRUS_THIRDPARTY_CFLAGS)
SET (WALRUS_CXXFLAGS_SHAREDLIB)
SET (WALRUS_LDFLAGS_SHAREDLIB)
SET (WALRUS_CXXFLAGS_STATICLIB)
SET (WALRUS_CXXFLAGS_SHELL)

SET (WALRUS_BUILD_32BIT OFF)
SET (WALRUS_BUILD_64BIT OFF)

# clang-cl defines ${CMAKE_CXX_COMPILER_ID} "Clang" and ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT} "MSVC"
SET (COMPILER_CLANG_CL OFF)
IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    IF (DEFINED CMAKE_CXX_COMPILER_FRONTEND_VARIANT)
        IF ("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
            SET (COMPILER_CLANG_CL ON)
        ENDIF()
    ENDIF()
ENDIF()

# Default options per compiler
IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR ${COMPILER_CLANG_CL})
    SET (WALRUS_CXXFLAGS /std:c++14 /fp:strict /Zc:__cplusplus /EHs /source-charset:utf-8 /D_CRT_SECURE_NO_WARNINGS /D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING /wd4244 /wd4267 /wd4805 /wd4018 /wd4172)
    SET (WALRUS_CXXFLAGS_RELEASE /O2 /Oy-)
    SET (WALRUS_THIRDPARTY_CFLAGS /D_CRT_SECURE_NO_WARNINGS /Oy- /wd4146 /EHs)
    IF (${COMPILER_CLANG_CL})
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} /EHs -Wno-invalid-offsetof -Wno-inline-new-delete -fintegrated-cc1)
    ENDIF()

    IF (WALRUS_SMALL_CONFIG)
        SET (WALRUS_CXXFLAGS_RELEASE ${WALRUS_CXXFLAGS_RELEASE} /Os)
    ENDIF()
    IF (WALRUS_DEBUG_INFO)
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} /DEBUG:FULL)
    ENDIF()
    SET (WALRUS_CXXFLAGS_SHAREDLIB)
    SET (WALRUS_LDFLAGS_SHAREDLIB)
    SET (WALRUS_CXXFLAGS_STATICLIB /DWASM_API_EXTERN)
    SET (WALRUS_CXXFLAGS_SHELL /DWASM_API_EXTERN /std:c++14)
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    SET (WALRUS_CXXFLAGS
        ${WALRUS_CXXFLAGS}
        -std=c++11 -g3
        -fno-rtti
        -fno-math-errno
        -fdata-sections -ffunction-sections
        -fno-omit-frame-pointer
        -fvisibility=hidden
        -frounding-math -fsignaling-nans
        -Wno-unused-parameter
        -Wno-type-limits -Wno-unused-result -Wno-unused-variable -Wno-invalid-offsetof
        -Wno-unused-but-set-variable -Wno-unused-but-set-parameter
        -Wno-deprecated-declarations -Wno-unused-function
    )
    IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9)
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -Wno-attributes -Wno-class-memaccess -Wno-deprecated-copy -Wno-cast-function-type -Wno-stringop-truncation -Wno-pessimizing-move -Wno-mismatched-new-delete -Wno-overloaded-virtual -Wno-dangling-pointer)
    endif()
    SET (WALRUS_CXXFLAGS_DEBUG -O0 -Wall -Wextra -Werror)
    SET (WALRUS_CXXFLAGS_RELEASE -O2 -fno-stack-protector -fno-omit-frame-pointer)
    SET (WALRUS_THIRDPARTY_CFLAGS -w -g3 -fdata-sections -ffunction-sections -fno-omit-frame-pointer -fvisibility=hidden)

    IF (WALRUS_SMALL_CONFIG)
        SET (WALRUS_CXXFLAGS_RELEASE ${WALRUS_CXXFLAGS_RELEASE} -Os)
    ENDIF()
    IF (WALRUS_DEBUG_INFO)
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -g3)
    ENDIF()
    SET (WALRUS_CXXFLAGS_SHAREDLIB -fPIC)
    SET (WALRUS_LDFLAGS_SHAREDLIB -ldl)
    SET (WALRUS_CXXFLAGS_STATICLIB -fPIC -DWASM_API_EXTERN=)
    SET (WALRUS_CXXFLAGS_SHELL -DWASM_API_EXTERN= -frtti -std=c++11)

ELSEIF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") # Clang and AppleClang
    SET (WALRUS_CXXFLAGS
        ${WALRUS_CXXFLAGS}
        -std=c++11 -g3
        -fno-rtti
        -fno-math-errno
        -fdata-sections -ffunction-sections
        -fno-omit-frame-pointer
        -fvisibility=hidden
        -fno-fast-math -fno-unsafe-math-optimizations -fdenormal-fp-math=ieee
        -Wno-type-limits -Wno-unused-result -Wno-unused-variable -Wno-invalid-offsetof -Wno-unused-function
        -Wno-deprecated-declarations -Wno-parentheses-equality -Wno-dynamic-class-memaccess -Wno-deprecated-register
        -Wno-expansion-to-defined -Wno-return-type -Wno-overloaded-virtual -Wno-unused-private-field -Wno-deprecated-copy -Wno-atomic-alignment
        -Wno-ambiguous-reversed-operator -Wno-deprecated-enum-enum-conversion -Wno-deprecated-enum-float-conversion -Wno-braced-scalar-init -Wno-unused-parameter
    )
    IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 10)
        # this feature supported after clang version 11
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -Wno-unsupported-floating-point-opt)
    endif()
    SET (WALRUS_CXXFLAGS_DEBUG -O0 -Wall -Wextra -Werror)
    SET (WALRUS_CXXFLAGS_RELEASE -O2 -fno-stack-protector -fno-omit-frame-pointer)
    SET (WALRUS_THIRDPARTY_CFLAGS -w -g3 -fdata-sections -ffunction-sections -fno-omit-frame-pointer -fvisibility=hidden)

    IF (WALRUS_SMALL_CONFIG)
        SET (WALRUS_CXXFLAGS_RELEASE ${WALRUS_CXXFLAGS_RELEASE} -Os)
    ENDIF()
    IF (WALRUS_DEBUG_INFO)
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -g3)
    ENDIF()
    SET (WALRUS_CXXFLAGS_SHAREDLIB -fPIC)
    SET (WALRUS_LDFLAGS_SHAREDLIB -ldl)
    SET (WALRUS_CXXFLAGS_STATICLIB -fPIC -DWASM_API_EXTERN=)
    SET (WALRUS_CXXFLAGS_SHELL -DWASM_API_EXTERN= -frtti -std=c++11)
ELSE()
    MESSAGE (FATAL_ERROR ${CMAKE_CXX_COMPILER_ID} " is Unsupported Compiler")
ENDIF()

# Default options per host
IF (${WALRUS_HOST} STREQUAL "linux")
    FIND_PACKAGE (PkgConfig REQUIRED)
    # default set of LDFLAGS
    SET (WALRUS_LDFLAGS -lpthread -lrt -Wl,--gc-sections)
    IF ((${WALRUS_ARCH} STREQUAL "x64") OR (${WALRUS_ARCH} STREQUAL "x86_64"))
        SET (WALRUS_BUILD_64BIT ON)
    ELSEIF ((${WALRUS_ARCH} STREQUAL "x86") OR (${WALRUS_ARCH} STREQUAL "i686"))
        SET (WALRUS_BUILD_32BIT ON)
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -m32 -mfpmath=sse -msse -msse2)
        SET (WALRUS_LDFLAGS ${WALRUS_LDFLAGS} -m32)
        SET (WALRUS_THIRDPARTY_CFLAGS ${WALRUS_THIRDPARTY_CFLAGS} -m32)
    ELSEIF (${WALRUS_ARCH} STREQUAL "arm")
        SET (WALRUS_BUILD_32BIT ON)
    ELSEIF (${WALRUS_ARCH} STREQUAL "aarch64")
        SET (WALRUS_BUILD_64BIT ON)
    ELSEIF (${WALRUS_ARCH} STREQUAL "riscv64")
        SET (WALRUS_BUILD_64BIT ON)
    ELSE()
        MESSAGE (FATAL_ERROR ${WALRUS_ARCH} " is unsupported")
    ENDIF()
ELSEIF (${WALRUS_HOST} STREQUAL "tizen" OR ${WALRUS_HOST} STREQUAL "tizen_obs")
    FIND_PACKAGE (PkgConfig REQUIRED)
    # default set of LDFLAGS
    SET (WALRUS_LDFLAGS -lpthread -lrt -Wl,--gc-sections)
    SET (WALRUS_DEFINITIONS -DWALRUS_TIZEN)
    IF ((${WALRUS_ARCH} STREQUAL "x64") OR (${WALRUS_ARCH} STREQUAL "x86_64"))
        SET (WALRUS_BUILD_64BIT ON)
    ELSEIF ((${WALRUS_ARCH} STREQUAL "x86") OR (${WALRUS_ARCH} STREQUAL "i686"))
        SET (WALRUS_BUILD_32BIT ON)
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -m32 -mfpmath=sse -msse -msse2)
        SET (WALRUS_LDFLAGS ${WALRUS_LDFLAGS} -m32)
        SET (WALRUS_THIRDPARTY_CFLAGS ${WALRUS_THIRDPARTY_CFLAGS} -m32)
    ELSEIF (${WALRUS_ARCH} STREQUAL "arm")
        SET (WALRUS_BUILD_32BIT ON)
        SET (WALRUS_CXXFLAGS_DEBUG -O1)
        SET (WALRUS_CXXFLAGS_RELEASE -O2)
    ELSEIF (${WALRUS_ARCH} STREQUAL "aarch64")
        SET (WALRUS_BUILD_64BIT ON)
    ELSEIF (${WALRUS_ARCH} STREQUAL "riscv64")
        SET (WALRUS_BUILD_64BIT ON)
    ELSE()
        MESSAGE (FATAL_ERROR ${WALRUS_ARCH} " is unsupported")
    ENDIF()
ELSEIF (${WALRUS_HOST} STREQUAL "android")
    FIND_PACKAGE (PkgConfig REQUIRED)
    SET (WALRUS_DEFINITIONS -DANDROID=1 -DWALRUS_ANDROID=1)
    SET (WALRUS_THIRDPARTY_CFLAGS ${WALRUS_THIRDPARTY_CFLAGS} -mstackrealign)
    SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -mstackrealign)
    IF (${WALRUS_ARCH} STREQUAL "arm")
        SET (WALRUS_BUILD_32BIT ON)
        SET (WALRUS_LDFLAGS -fPIE -pie -march=armv7-a -Wl,--fix-cortex-a8 -llog -Wl,--gc-sections)
    ELSEIF ((${WALRUS_ARCH} STREQUAL "arm64") OR (${WALRUS_ARCH} STREQUAL "aarch64"))
        SET (WALRUS_BUILD_64BIT ON)
        SET (WALRUS_LDFLAGS -fPIE -pie -llog -Wl,--gc-sections)
    ELSEIF (${WALRUS_ARCH} STREQUAL "x86")
        SET (WALRUS_BUILD_32BIT ON)
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -m32 -mfpmath=sse -msse -msse2  -mstackrealign)
        SET (WALRUS_LDFLAGS -fPIE -pie -llog -Wl,--gc-sections -m32)
    ELSEIF (${WALRUS_ARCH} STREQUAL "x86_64" OR ${WALRUS_ARCH} STREQUAL "x64")
        SET (WALRUS_BUILD_64BIT ON)
        SET (WALRUS_LDFLAGS -fPIE -pie -llog -Wl,--gc-sections)
        # bdwgc android amd64 cannot support keeping back ptrs
        SET (WALRUS_THIRDPARTY_CFLAGS ${WALRUS_THIRDPARTY_CFLAGS} -UKEEP_BACK_PTRS -USAVE_CALL_COUNT -UDBG_HDRS_ALL)
    ENDIF()
ELSEIF (${WALRUS_HOST} STREQUAL "darwin")
    FIND_PACKAGE (PkgConfig REQUIRED)
    IF ((NOT ${WALRUS_ARCH} STREQUAL "x64") AND (NOT ${WALRUS_ARCH} STREQUAL "aarch64"))
        MESSAGE (FATAL_ERROR ${WALRUS_ARCH} " is unsupported")
    ENDIF()
    SET (WALRUS_LDFLAGS -lpthread -Wl,-dead_strip)
    # bdwgc mac cannot support pthread_getattr_np
    SET (WALRUS_THIRDPARTY_CFLAGS ${WALRUS_THIRDPARTY_CFLAGS} -UHAVE_PTHREAD_GETATTR_NP)
    SET (WALRUS_BUILD_64BIT ON)
ELSEIF (${WALRUS_HOST} STREQUAL "windows")
    # in windows, default stack limit is 1MB
    # but expand stack to 8MB when building to exe for running test
    IF (${WALRUS_OUTPUT} STREQUAL "shell")
        # Default limit on windows is 1MB
        # but we needs more stack to pass testcases
        # and we needs more reserved space for process stackoverflow exception
        # msvc process native exception catch on top of stack :(
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} /DSTACK_LIMIT_FROM_BASE=4194304)
        SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:16777216")
    ELSE()
        SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} /DSTACK_LIMIT_FROM_BASE=524288)
    ENDIF()

    IF ((${WALRUS_ARCH} STREQUAL "x64") OR (${WALRUS_ARCH} STREQUAL "x86_64"))
        SET (WALRUS_BUILD_64BIT ON)
    ELSEIF ((${WALRUS_ARCH} STREQUAL "x86") OR (${WALRUS_ARCH} STREQUAL "i686"))
        SET (WALRUS_BUILD_32BIT ON)
    ELSEIF (${WALRUS_ARCH} STREQUAL "arm")
        SET (WALRUS_BUILD_32BIT ON)
    ELSEIF (${WALRUS_ARCH} STREQUAL "aarch64" OR (${WALRUS_ARCH} STREQUAL "arm64"))
        SET (WALRUS_BUILD_64BIT ON)
    ELSE()
        MESSAGE (FATAL_ERROR ${WALRUS_ARCH} " is unsupported")
    ENDIF()

ELSE()
    MESSAGE (FATAL_ERROR ${WALRUS_HOST} " with " ${WALRUS_ARCH} " is unsupported")
ENDIF()

IF (WALRUS_BUILD_32BIT)
    # 32bit build
    SET (WALRUS_DEFINITIONS ${WALRUS_DEFINITIONS} -DWALRUS_32=1)
ELSEIF (WALRUS_BUILD_64BIT)
    # 64bit build
    SET (WALRUS_DEFINITIONS ${WALRUS_DEFINITIONS} -DWALRUS_64=1)
ELSE()
    MESSAGE (FATAL_ERROR "unsupported mode")
ENDIF()
