add_subdirectory(AnalysisStrategy)
add_subdirectory(PhasarLLVM)
add_subdirectory(PhasarPass)
add_subdirectory(DB)
add_subdirectory(Config)
add_subdirectory(Utils)
add_subdirectory(Controller)
add_subdirectory(Pointer)
add_subdirectory(ControlFlow)
if(BUILD_PHASAR_CLANG)
    add_subdirectory(PhasarClang)
endif()

# The fat lib relies on transitive dependencies...
set(PHASAR_DEBUG_LIBDEPS_SAVE ${PHASAR_DEBUG_LIBDEPS})
set(PHASAR_DEBUG_LIBDEPS OFF)

if(PHASAR_BUILD_DYNLIB)
    set(PHASAR_DYNLIB_KIND SHARED)
endif()

add_phasar_library(phasar ${PHASAR_DYNLIB_KIND}
    FILES
        LibPhasar.cpp
    LINKS
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
    LINK_PUBLIC
        ${Boost_LIBRARIES}
    LLVM_LINK_COMPONENTS
        Core
        Support
        BitWriter
        Analysis
        Passes
        Demangle
)

set(PHASAR_DEBUG_LIBDEPS ${PHASAR_DEBUG_LIBDEPS_SAVE})
