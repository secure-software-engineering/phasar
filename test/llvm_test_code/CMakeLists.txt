add_custom_target(LLFileGeneration ALL)

subdirlist(subdirs ${CMAKE_CURRENT_SOURCE_DIR})
foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach(subdir)


if (PHASAR_ENABLE_INTEGRATIONTESTS)
  include(AddLLVM)

  find_program(LLVM_EXTERNAL_LIT_DEFAULT
    NAMES lit-22 lit-21 lit-20 lit-16 llvm-lit lit lit.py
    HINTS "${LLVM_TOOLS_BINARY_DIR}" "${LLVM_INSTALL_PREFIX}/build/utils/lit"
    REQUIRED
  )

  set(LLVM_EXTERNAL_LIT "${LLVM_EXTERNAL_LIT_DEFAULT}"
    CACHE STRING "Command used to spawn lit"
    FORCE
  )

  get_filename_component(PSR_LIT_REALPATH ${LLVM_EXTERNAL_LIT} REALPATH)
  get_filename_component(PSR_LIT_DIR ${PSR_LIT_REALPATH} DIRECTORY)
  find_program(PSR_FILECHECK
    NAMES FileCheck-22 FileCheck-21 FileCheck-20 FileCheck
    HINTS "${PSR_LIT_DIR}" "${LLVM_TOOLS_BINARY_DIR}"
    REQUIRED
  )

  # Note: PSR_FILECHECK_DIR configured into lit.site.cfg.py
  get_filename_component(PSR_FILECHECK_REALPATH ${PSR_FILECHECK} REALPATH)
  get_filename_component(PSR_FILECHECK_DIR ${PSR_FILECHECK_REALPATH} DIRECTORY)

  if(NOT clang OR NOT clangcpp)
    message(FATAL_ERROR "Require clang and clang++")
  endif()
  get_filename_component(PSR_CLANG_REALPATH ${clang} REALPATH)
  get_filename_component(PSR_CLANGPP_REALPATH ${clangcpp} REALPATH)

  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
    MAIN_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  )

  add_lit_testsuite(check-phasar-cli "Running the phasar-cli lit test suite"
    # Note: Currently, we only have LIT-tests for taint_analysis and xtaint
    ${CMAKE_CURRENT_BINARY_DIR}/taint_analysis
    ${CMAKE_CURRENT_BINARY_DIR}/xtaint
    DEPENDS phasar-cli LLFileGeneration
  )
endif(PHASAR_ENABLE_INTEGRATIONTESTS)
