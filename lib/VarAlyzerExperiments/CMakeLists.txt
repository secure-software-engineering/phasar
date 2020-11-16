file(GLOB_RECURSE VARALYZER_UTILS_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
)

set(LLVM_LINK_COMPONENTS
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_varalyzer_utils
    SHARED
    ${VARALYZER_UTILS_SRC}
  )
else()
  add_phasar_library(phasar_varalyzer_utils
    STATIC
    ${VARALYZER_UTILS_SRC}
  )
endif()

target_include_directories(phasar_varalyzer_utils PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(phasar_varalyzer_utils PUBLIC -DBOOST_LOG_DYN_LINK)

find_package(Boost COMPONENTS log filesystem REQUIRED)
target_link_libraries(phasar_varalyzer_utils
  LINK_PUBLIC
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
)

set_target_properties(phasar_varalyzer_utils
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
