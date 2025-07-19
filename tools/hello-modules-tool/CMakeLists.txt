# Build a stand-alone executable
if(PHASAR_IN_TREE)
  # Build a small test tool to show how phasar may be used
  add_phasar_executable(hello-modules
    hello_modules.cpp
  )
else()
  # Build a small test tool to show how phasar may be used
  add_executable(hello-modules
    hello_modules.cpp
  )
endif()

target_link_libraries(hello-modules
  PRIVATE
    phasar
    ${PHASAR_STD_FILESYSTEM}
)
