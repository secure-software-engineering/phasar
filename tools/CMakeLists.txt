
subdirlist(subdirs ${CMAKE_CURRENT_SOURCE_DIR})
if (NOT PHASAR_BUILD_MODULES)
  list(REMOVE_ITEM subdirs hello-modules-tool)
endif()

foreach(tool ${subdirs})
  message(STATUS "Set-up phasar-tool: ${tool}")

  file(GLOB_RECURSE tool_src "${tool}/*.cpp" "${tool}/*.h")
  if(PHASAR_IN_TREE)
    add_phasar_executable(${tool}
      ${tool_src}
    )
  else()
    add_executable(${tool}
      ${tool_src}
    )
  endif()

  set_target_properties(${tool} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${tool}
  )

  target_link_libraries(${tool}
    PRIVATE
      phasar
      ${PHASAR_STD_FILESYSTEM}
  )
endforeach()

set_target_properties(example-tool PROPERTIES
  OUTPUT_NAME "myphasartool"
)

if (NOT PHASAR_IN_TREE)
  install(TARGETS phasar-cli)
endif()
