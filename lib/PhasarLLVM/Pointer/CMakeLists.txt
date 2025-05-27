file(GLOB POINTER_SRC *.cpp)

add_phasar_library(phasar_llvm_pointer
  ${POINTER_SRC}

  LINKS
    phasar_utils
    phasar_pointer
    phasar_llvm_utils
    phasar_llvm_db

  LLVM_LINK_COMPONENTS
    Core
    Support
    Analysis
    Passes
    Demangle

  LINK_PRIVATE
    ${Boost_LIBRARIES}

  MODULE_FILES
    Pointer.cppm
)

add_subdirectory(external)

if(PHASAR_USE_SVF)
  add_subdirectory(SVF)
  target_link_libraries(phasar_llvm_pointer PRIVATE phasar_llvm_pointer_svf)
endif()
