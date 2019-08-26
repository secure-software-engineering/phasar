file(GLOB_RECURSE PHASARCLANG_SRC *.h *.cpp)

include_directories(${CLANG_INCLUDE_DIRS})

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_clang
    SHARED
	  ${PHASARCLANG_SRC}
  )
else()
  add_phasar_library(phasar_clang
    STATIC
    ${PHASARCLANG_SRC}
  )
endif()

target_link_libraries(phasar_clang
  clangTooling
  clangFrontendTool
  clangFrontend
  clangDriver
  clangSerialization
  clangCodeGen
  clangParse
  clangSema
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangAnalysis
  clangARCMigrate
  clangRewrite
  clangRewriteFrontend
  clangEdit
  clangAST
  clangASTMatchers
  clangLex
  clangBasic

  LLVMSupport
  LLVMCore

  boost_log
  boost_system
)

set_target_properties(phasar_clang
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
