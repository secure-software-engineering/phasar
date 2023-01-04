file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
  phasar_db
  phasar_pointer
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Analysis
  Passes
  Demangle
)

# Handle the library files
if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_llvm_pointer
    SHARED
    ${POINTER_SRC}
  )
else()
  add_phasar_library(phasar_llvm_pointer
    STATIC
    ${POINTER_SRC}
  )
endif()

find_package(Boost COMPONENTS graph REQUIRED)
target_link_libraries(phasar_llvm_pointer
  LINK_PUBLIC
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_llvm_pointer
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
