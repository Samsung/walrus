#######################################################
# CONFIGURATION
#######################################################

#######################################################
# PATH
#######################################################
SET (WALRUS_ROOT ${PROJECT_SOURCE_DIR})
SET (WALRUS_THIRD_PARTY_ROOT ${WALRUS_ROOT}/third_party)
SET (SLJIT_ROOT ${WALRUS_THIRD_PARTY_ROOT}/sljit)
SET (GCUTIL_ROOT ${WALRUS_THIRD_PARTY_ROOT}/GCutil)

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
SET (LDFLAGS_FROM_ENV $ENV{LDFLAGS})
SEPARATE_ARGUMENTS(LDFLAGS_FROM_ENV)

# these flags assigned from external should have the highest priority
SET (CXXFLAGS_FROM_ENV ${CXXFLAGS_FROM_ENV} ${WALRUS_CXXFLAGS_FROM_EXTERNAL})
SET (LDFLAGS_FROM_ENV ${LDFLAGS_FROM_ENV} ${WALRUS_LDFLAGS_FROM_EXTERNAL})

IF (${WALRUS_OUTPUT} STREQUAL "shared_lib" AND ${WALRUS_HOST} STREQUAL "android")
    SET (WALRUS_LDFLAGS ${WALRUS_LDFLAGS} -shared)
ENDIF()

IF (NOT DEFINED WALRUS_JIT)
    SET (WALRUS_JIT ON)
ENDIF()

#######################################################
# FLAGS FOR ADDITIONAL FUNCTION
#######################################################
SET (WALRUS_LIBRARIES)
SET (WALRUS_INCDIRS)

IF (WALRUS_JIT)
    SET (WALRUS_DEFINITIONS ${WALRUS_DEFINITIONS} -DWALRUS_ENABLE_JIT)
ENDIF()

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
