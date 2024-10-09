set(LLVM_LINK_COMPONENTS
  Analysis
  BitWriter
  Core
  Demangle
  IRReader
  Linker
  Passes
  Support
)

# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(indexed-uninit
  IFDSUninitializedVaraiblesIndexedAnalysis.cpp
  )
else()
  add_executable(indexed-uninit
    IFDSUninitializedVaraiblesIndexedAnalysis.cpp
  )
endif()

target_link_libraries(indexed-uninit
  PRIVATE
    phasar
    ${PHASAR_STD_FILESYSTEM}
)

if (NOT PHASAR_IN_TREE)
  install(TARGETS indexed-uninit)
endif()
