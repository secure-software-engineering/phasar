add_custom_target(LLFileGeneration ALL)

subdirlist(subdirs ${CMAKE_CURRENT_SOURCE_DIR})
foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach(subdir)


include(AddLLVM)


find_program(LLVM_EXTERNAL_LIT_DEFAULT NAMES lit-22 lit-21 lit-20 lit-16 llvm-lit lit)
if(NOT LLVM_EXTERNAL_LIT_DEFAULT)
  message(FATAL_ERROR "lit not found")
endif()
set(LLVM_EXTERNAL_LIT "${LLVM_EXTERNAL_LIT_DEFAULT}"
  CACHE STRING "Command used to spawn lit" FORCE)

find_program(PSR_FILECHECK
  NAMES FileCheck-22 FileCheck-21 FileCheck-20 FileCheck
  HINTS "${LLVM_TOOLS_BINARY_DIR}"
  REQUIRED
)
get_filename_component(PSR_LIT_REALPATH ${LLVM_EXTERNAL_LIT} REALPATH)
get_filename_component(PSR_FILECHECK_REALPATH ${PSR_FILECHECK} REALPATH)
get_filename_component(PSR_FILECHECK_DIR ${PSR_FILECHECK_REALPATH} DIRECTORY)
# include(HandleLLVMOptions)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

add_lit_testsuite(check-phasar-cli "Running the phasar-cli lit test suite"
  ${CMAKE_CURRENT_BINARY_DIR}/taint_analysis
  ${CMAKE_CURRENT_BINARY_DIR}/xtaint
  DEPENDS phasar-cli LLFileGeneration
)

# message(FATAL_ERROR "lit path: ${PSR_LIT_REALPATH} filecheck path: ${PSR_FILECHECK_REALPATH}")
