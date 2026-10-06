#######################################################
# CONFIGURATION
#######################################################

#######################################################
# PATH
#######################################################
SET (WALRUS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
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

# CMake already consumes CXXFLAGS and LDFLAGS when initializing its cache.
# Keep only the legacy explicit flag lists here to avoid applying environment
# flags twice or overriding configuration-specific optimization flags.
SET (CXXFLAGS_FROM_ENV ${WALRUS_CXXFLAGS_FROM_EXTERNAL})
SET (LDFLAGS_FROM_ENV ${WALRUS_LDFLAGS_FROM_EXTERNAL})

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
