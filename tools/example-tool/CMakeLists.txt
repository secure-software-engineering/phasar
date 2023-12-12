# Build a stand-alone executable
if(PHASAR_IN_TREE)
  # Build a small test tool to show how phasar may be used
  add_phasar_executable(myphasartool
    myphasartool.cpp
  )
else()
  # Build a small test tool to show how phasar may be used
  add_executable(myphasartool
    myphasartool.cpp
  )
endif()

target_link_libraries(myphasartool
  PRIVATE
    phasar
    ${PHASAR_STD_FILESYSTEM}
)

if(USE_LLVM_FAT_LIB)
  llvm_config(myphasartool USE_SHARED ${LLVM_LINK_COMPONENTS})
else()
  llvm_config(myphasartool ${LLVM_LINK_COMPONENTS})
endif()

if(BUILD_PHASAR_CLANG)
  target_link_libraries(myphasartool PRIVATE ${CLANG_LIBRARY})
endif()

install(TARGETS myphasartool
  RUNTIME DESTINATION bin
)
