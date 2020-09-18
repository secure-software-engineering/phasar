file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Analysis
  Passes
  # Core
  # # Vectorize
  # # ScalarOpts
  # # Instrumentation
  # # ObjCARCOpts
  # # TransformUtils
  # # CodeGen
  # # Analysis
  # # ipo
  # IRReader
  # # InstCombine
  # # Linker
  # BitWriter
)

# Handle the library files
if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_pointer
    SHARED
    ${POINTER_SRC}
  )
else()
  add_phasar_library(phasar_pointer
    STATIC
    ${POINTER_SRC}
  )
endif()

find_package(Boost COMPONENTS log filesystem graph REQUIRED)
target_link_libraries(phasar_pointer
  LINK_PUBLIC  
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_pointer
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
