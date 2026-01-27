add_subdirectory(AnalysisStrategy)
add_subdirectory(Config)
add_subdirectory(ControlFlow)
add_subdirectory(DataFlow)
add_subdirectory(DB)
add_subdirectory(Domain)
add_subdirectory(PhasarLLVM)
add_subdirectory(PhasarPass)
add_subdirectory(Pointer)
add_subdirectory(TypeHierarchy)
add_subdirectory(Utils)

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
    phasar_analysis_strategy
    phasar_config
    phasar_controlflow
    phasar_dataflow
    phasar_dataflow_ifdside
    phasar_dataflow_mono
    phasar_dataflow_pathsensitivity
    phasar_domain

    phasar_llvm
    phasar_llvm_controlflow
    phasar_llvm_ifdside
    phasar_llvm_mono
    phasar_llvm_pathsensitivity
    phasar_llvm_dataflow
    phasar_llvm_db
    phasar_llvm_domain
    phasar_llvm_pointer
    phasar_llvm_typehierarchy
    phasar_llvm_utils

    phasar_pass
    phasar_passes
    phasar_pointer
    phasar_taintconfig
    phasar_typehierarchy
    phasar_utils
)
if(BUILD_PHASAR_CLANG)
    list(APPEND PHASAR_LINK_LIBS phasar_clang)
endif()
if(TARGET phasar_db)
    list(APPEND PHASAR_LINK_LIBS phasar_db)
endif()
if(PHASAR_USE_SVF)
    list(APPEND PHASAR_LINK_LIBS phasar_llvm_pointer_svf)
endif()

add_phasar_library(phasar ${PHASAR_DYNLIB_KIND}
    FILES
        LibPhasar.cpp
    LINKS
        ${PHASAR_LINK_LIBS}
    LLVM_LINK_COMPONENTS
        Core
        Support
        BitWriter
        Analysis
        Passes
        Demangle
)
