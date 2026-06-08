add_custom_target(LLFileGeneration ALL)

# Use phasar's capabilities to automate the LLVM-IR file generation

include(../../../cmake/phasar_macros.cmake)
if (NOT PHASAR_LLVM_VERSION)
  set(PHASAR_LLVM_VERSION 15)
endif()

file(GLOB target_files RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.cpp)

foreach(target_file ${target_files})
  generate_ll_file(FILE ${target_file} DEBUG)
endforeach()
