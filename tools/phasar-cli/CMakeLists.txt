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
  add_phasar_executable(phasar-cli
    phasar-cli.cpp
  )
else()
  add_executable(phasar-cli
    phasar-cli.cpp
  )
endif()

# Warning! There is a another listing of libraries inside cmake/phasar_macros.cmake.
# If this list is altered the other one should be altered accordingly.
target_link_libraries(phasar-cli
  PRIVATE
    phasar_config
    phasar_controller
    phasar_llvm_controlflow
    phasar_llvm_utils
    phasar_analysis_strategy
    phasar_llvm_ifdside
    phasar_utils
    phasar_mono
    phasar_llvm_db
    phasar_passes
    phasar_llvm_pointer
    phasar_llvm
    phasar_llvm_typehierarchy
    phasar_taintconfig

    ${PHASAR_STD_FILESYSTEM}
)

if (NOT PHASAR_IN_TREE)
  if(USE_LLVM_FAT_LIB)
    llvm_config(phasar-cli USE_SHARED ${LLVM_LINK_COMPONENTS})
  else()
    llvm_config(phasar-cli ${LLVM_LINK_COMPONENTS})
  endif()

  install(TARGETS phasar-cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()
