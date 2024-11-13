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

add_subdirectory(Controller)

target_link_libraries(phasar-cli
  PRIVATE
    phasar
    ${PHASAR_STD_FILESYSTEM}
)

if (NOT PHASAR_IN_TREE)
  install(TARGETS phasar-cli)
endif()
