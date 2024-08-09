add_subdirectory(AnalysisStrategy)
add_subdirectory(PhasarLLVM)
add_subdirectory(PhasarPass)
add_subdirectory(DB)
add_subdirectory(Config)
add_subdirectory(Utils)
add_subdirectory(Pointer)
add_subdirectory(ControlFlow)
if(BUILD_PHASAR_CLANG)
    if(PHASAR_IN_TREE)
        add_dependencies(phasar_interface intrinsics_gen)
    endif()
    add_subdirectory(PhasarClang)
endif()

if(PHASAR_BUILD_DYNLIB)
    set(PHASAR_DYNLIB_KIND SHARED)
endif()

set(PHASAR_LINK_LIBS
    phasar_utils
    phasar_passes
    phasar_config
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
)
if(SQLite3_FOUND)
    list(APPEND PHASAR_LINK_LIBS phasar_db)
endif()

add_phasar_library(phasar ${PHASAR_DYNLIB_KIND}
    FILES
        LibPhasar.cpp
    LINKS
        ${PHASAR_LINK_LIBS}
    LINK_PRIVATE
        ${Boost_LIBRARIES}
    LLVM_LINK_COMPONENTS
        Core
        Support
        BitWriter
        Analysis
        Passes
        Demangle
)
