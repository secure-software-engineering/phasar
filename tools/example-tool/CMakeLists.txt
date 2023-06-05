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
  LINK_PUBLIC
  phasar
  LINK_PRIVATE
  ${PHASAR_STD_FILESYSTEM}
)

install(TARGETS myphasartool
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
