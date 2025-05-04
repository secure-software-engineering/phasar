# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(typestate
    typestate.cpp
  )
else()
  add_executable(typestate
    typestate.cpp
  )
endif()

# Warning! There is a another listing of libraries inside cmake/phasar_macros.cmake.
# If this list is altered the other one should be altered accordingly.
target_link_libraries(typestate
  PRIVATE
    phasar
)

if (NOT PHASAR_IN_TREE)
  install(TARGETS typestate)
endif()
