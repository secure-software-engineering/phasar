file(GLOB_RECURSE CONFIG_SRC *.h *.cpp)

set(LLVM_LINK_COMPONENTS
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_config
    SHARED
    ${CONFIG_SRC}
  )
else()
  add_phasar_library(phasar_config
    STATIC
    ${CONFIG_SRC}
  )
endif()

target_link_libraries(phasar_config
  LINK_PUBLIC
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_config
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
