add_subdirectory(AnalysisStrategy)
add_subdirectory(PhasarLLVM)
add_subdirectory(PhasarPass)
add_subdirectory(PhasarClang)
add_subdirectory(DB)
add_subdirectory(Config)
add_subdirectory(Utils)
add_subdirectory(Controller)
add_subdirectory(Pointer)
add_subdirectory(ControlFlow)

set(PHASAR_LINK_LIBS
    phasar_utils
    phasar_passes
    phasar_config
    phasar_db
    phasar_pointer
    phasar_controlflow

    phasar_llvm_utils
    phasar_llvm_db
    phasar_llvm_pointer
    phasar_llvm_typehierarchy
    phasar_llvm_controlflow

    phasar_taintconfig
    phasar_mono
    phasar_llvm
    phasar_llvm_ifdside
    phasar_analysis_strategy
    phasar_controller
    ${Boost_LIBRARIES}
)

set(LLVM_LINK_COMPONENTS
    Core
    Support
    BitWriter
    Analysis
    Passes
    Demangle
)

# The fat lib relies on transitive dependencies...
set(PHASAR_DEBUG_LIBDEPS_SAVE ${PHASAR_DEBUG_LIBDEPS})
set(PHASAR_DEBUG_LIBDEPS OFF)

if(BUILD_SHARED_LIBS OR PHASAR_BUILD_DYNLIB)
    add_phasar_library(phasar SHARED
        LibPhasar.cpp
    )
else()
    add_phasar_library(phasar STATIC
        LibPhasar.cpp
    )
endif()

set(PHASAR_DEBUG_LIBDEPS ${PHASAR_DEBUG_LIBDEPS_SAVE})
