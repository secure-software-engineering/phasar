file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

if(PHASAR_ENABLE_PAMM STREQUAL "Off" AND NOT PHASAR_BUILD_UNITTESTS)
	message("Not compiling PAMM.cpp since PAMM and Unittests are disabled.")
	get_filename_component(pamm_src PAMM.cpp ABSOLUTE)
	list(REMOVE_ITEM UTILS_SRC ${pamm_src})
endif()

# Compile phasar_utils once into an object
add_library(phasar_utils_o OBJECT
  ${UTILS_SRC}
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_utils
    SHARED
    $<TARGET_OBJECTS:phasar_utils_o>
  )
  add_phasar_library(phasar_utils_without
    SHARED
    $<TARGET_OBJECTS:phasar_utils_o>
  )
else()
  add_phasar_library(phasar_utils
    STATIC
    $<TARGET_OBJECTS:phasar_utils_o>
  )
  add_phasar_library(phasar_utils_without
    STATIC
    $<TARGET_OBJECTS:phasar_utils_o>
  )
endif()

set(phasar_utils_dep
  phasar_config
)

target_link_libraries(phasar_utils
	LLVMCore
	LLVMSupport
  LLVMBitWriter
  ${phasar_utils_dep}
)

set_target_properties(phasar_utils
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)

target_link_libraries(phasar_utils_without
  ${phasar_utils_dep}
)

set_target_properties(phasar_utils_without
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
