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


add_library(phasar STATIC
    LibPhasar.cpp
)
