#######################################################
# CONFIGURATION
#######################################################

#######################################################
# PATH
#######################################################
SET (WALRUS_ROOT ${PROJECT_SOURCE_DIR})
SET (WALRUS_THIRD_PARTY_ROOT ${WALRUS_ROOT}/third_party)
SET (SLJIT_ROOT ${WALRUS_THIRD_PARTY_ROOT}/sljit)

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

IF (${WALRUS_OUTPUT} STREQUAL "shared_lib" AND ${WALRUS_HOST} STREQUAL "android")
    SET (WALRUS_LDFLAGS ${WALRUS_LDFLAGS} -shared)
ENDIF()

#######################################################
# FLAGS FOR ADDITIONAL FUNCTION
#######################################################
SET (WALRUS_LIBRARIES)
SET (WALRUS_INCDIRS)

#######################################################
# FLAGS FOR TEST
#######################################################
SET (WALRUS_DEFINITIONS_TEST -DWALRUS_ENABLE_TEST)

#######################################################
# FLAGS FOR MEMORY PROFILING
#######################################################
SET (PROFILER_FLAGS)

IF (WALRUS_VALGRIND)
    SET (PROFILER_FLAGS ${PROFILER_FLAGS} -DWALRUS_VALGRIND)
ENDIF()
