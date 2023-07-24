
# target should always be there
# but there shouldn't be always a library
if (NOT PHASAR_BUILD_DYNLIB)
    set(dynlib_header_only ON)
elseif (PHASAR_BUILD_DYNLIB AND BUILD_SHARED_LIBS)
    message(FATAL_ERROR "PHASAR_BUILD_DYNLIB is incompatible with BUILD_SHARED_LIBS") # could be header only
else(PHASAR_BUILD_DYNLIB AND NOT BUILD_SHARED_LIBS)
    set(dynlib_header_only OFF)
endif()

set(exclude)
if (dynlib_header_only)
    set(exclude "^.*/src/.*$")
endif()
just_add_library(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK
    phasar-llvm
    phasar-clang # TODO: auto add all phasar libs?
    phasar-analysisstrategy
    phasar-config
    
    phasar-dataflow
    phasar-db
    phasar-domain
    phasar-pointer
    phasar-typehierarchy
    phasar-utils
    phasar-controller
    EXCLUDE ${exclude}
)
