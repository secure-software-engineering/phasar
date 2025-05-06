# Build a stand-alone executable
if(PHASAR_IN_TREE)
  # Build a small test tool to show how phasar may be used
  add_phasar_executable(full-phasar-modules
    full-phasar-modules.cpp
  )
else()
  # Build a small test tool to show how phasar may be used
  add_executable(full-phasar-modules
    full-phasar-modules.cpp
  )
endif()

target_link_libraries(full-phasar-modules
  PRIVATE
    phasar
    ${PHASAR_STD_FILESYSTEM}
)
