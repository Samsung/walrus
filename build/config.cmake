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
SET (LDFLAGS_FROM_ENV $ENV{LDFLAGS})
SEPARATE_ARGUMENTS(LDFLAGS_FROM_ENV)

# these flags assigned from external should have the highest priority
SET (CXXFLAGS_FROM_ENV ${CXXFLAGS_FROM_ENV} ${WALRUS_CXXFLAGS_FROM_EXTERNAL})
SET (LDFLAGS_FROM_ENV ${LDFLAGS_FROM_ENV} ${WALRUS_LDFLAGS_FROM_EXTERNAL})

IF (${WALRUS_OUTPUT} STREQUAL "shared_lib" AND ${WALRUS_HOST} STREQUAL "android")
    SET (WALRUS_LDFLAGS ${WALRUS_LDFLAGS} -shared)
ENDIF()

IF (WALRUS_EXTENDED_FEATURES)
    SET (WALRUS_DEFINITIONS ${WALRUS_DEFINITIONS} -DENABLE_EXTENDED_FEATURES)
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
