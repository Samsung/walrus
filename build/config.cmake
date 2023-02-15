#######################################################
# CONFIGURATION
#######################################################

#######################################################
# PATH
#######################################################
SET (WALRUS_ROOT ${PROJECT_SOURCE_DIR})
SET (WALRUS_THIRD_PARTY_ROOT ${WALRUS_ROOT}/third_party)

#######################################################
# FLAGS FOR TARGET
#######################################################
INCLUDE (${WALRUS_ROOT}/build/target.cmake)

#######################################################
# FLAGS FOR COMMON
#######################################################
# WALRUS COMMON CXXFLAGS
SET (WALRUS_DEFINITIONS
    ${WALRUS_DEFINITIONS}
    -DWALRUS
)

SET (CXXFLAGS_FROM_ENV $ENV{CXXFLAGS})
SEPARATE_ARGUMENTS(CXXFLAGS_FROM_ENV)
SET (CFLAGS_FROM_ENV $ENV{CFLAGS})
SEPARATE_ARGUMENTS(CFLAGS_FROM_ENV)

SET (WALRUS_CXXFLAGS
    ${CXXFLAGS_FROM_ENV}
    ${WALRUS_CXXFLAGS}
    -std=c++11
    -fno-math-errno
    -fdata-sections -ffunction-sections
    -fno-omit-frame-pointer
    -fvisibility=hidden
    -Wno-unused-parameter
    -Wno-type-limits -Wno-unused-result -Wno-unused-variable -Wno-invalid-offsetof
    -Wno-deprecated-declarations
)

IF (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-mismatched-new-delete -Wno-attributes)
ELSEIF (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    SET (WALRUS_CXXFLAGS ${WALRUS_CXXFLAGS} -fno-fast-math -fno-unsafe-math-optimizations -fdenormal-fp-math=ieee -Wno-unsupported-floating-point-opt -Wno-parentheses-equality -Wno-dynamic-class-memaccess -Wno-deprecated-register -Wno-expansion-to-defined -Wno-return-type -Wno-overloaded-virtual -Wno-unused-private-field -Wno-deprecated-copy -Wno-atomic-alignment)
ELSE()
    MESSAGE (FATAL_ERROR ${CMAKE_CXX_COMPILER_ID} " is Unsupported Compiler")
ENDIF()

SET (LDFLAGS_FROM_ENV $ENV{LDFLAGS})
SEPARATE_ARGUMENTS(LDFLAGS_FROM_ENV)

# WALRUS COMMON LDFLAGS
SET (WALRUS_LDFLAGS ${WALRUS_LDFLAGS} -fvisibility=hidden)

# bdwgc
IF (${WALRUS_MODE} STREQUAL "debug")
    SET (WALRUS_DEFINITIONS_COMMON ${WALRUS_DEFINITIONS_COMMON} -DGC_DEBUG)
ENDIF()

IF (${WALRUS_OUTPUT} STREQUAL "shared_lib" AND ${WALRUS_HOST} STREQUAL "android")
    SET (WALRUS_LDFLAGS ${WALRUS_LDFLAGS} -shared)
ENDIF()

#######################################################
# FLAGS FOR ADDITIONAL FUNCTION
#######################################################
SET (WALRUS_LIBRARIES)
SET (WALRUS_INCDIRS)
FIND_PACKAGE (PkgConfig REQUIRED)

#######################################################
# FLAGS FOR $(MODE) : debug/release
#######################################################
# DEBUG FLAGS
SET (WALRUS_CXXFLAGS_DEBUG -O0 -g3 -Wall -Wextra -Werror ${WALRUS_CXXFLAGS_DEBUG})
SET (WALRUS_DEFINITIONS_DEBUG -D_GLIBCXX_DEBUG -DGC_DEBUG)

# RELEASE FLAGS
SET (WALRUS_CXXFLAGS_RELEASE -O2 -fno-stack-protector ${WALRUS_CXXFLAGS_RELEASE})
SET (WALRUS_DEFINITIONS_RELEASE -DNDEBUG)

# SHARED_LIB FLAGS
SET (WALRUS_CXXFLAGS_SHAREDLIB -fPIC)
SET (WALRUS_LDFLAGS_SHAREDLIB -ldl)

# STATIC_LIB FLAGS
SET (WALRUS_CXXFLAGS_STATICLIB -fPIC -DWALRUS_EXPORT=)

# SHELL FLAGS
SET (WALRUS_CXXFLAGS_SHELL -DWALRUS_EXPORT= -frtti -std=c++17)

#######################################################
# FLAGS FOR TEST
#######################################################
SET (WALRUS_DEFINITIONS_TEST -DWALRUS_ENABLE_TEST)

#######################################################
# FLAGS FOR MEMORY PROFILING
#######################################################
SET (PROFILER_FLAGS)

IF (WALRUS_PROFILE_BDWGC)
    SET (PROFILER_FLAGS ${PROFILER_FLAGS} -DPROFILE_BDWGC)
ENDIF()

IF (WALRUS_MEM_STATS)
    SET (PROFILER_FLAGS ${PROFILER_FLAGS} -DWALRUS_MEM_STATS)
ENDIF()

IF (WALRUS_VALGRIND)
    SET (PROFILER_FLAGS ${PROFILER_FLAGS} -DWALRUS_VALGRIND)
ENDIF()
