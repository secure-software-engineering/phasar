file(GLOB_RECURSE PHASARCLANG_SRC *.h *.cpp)

include_directories(${CLANG_INCLUDE_DIRS})

add_phasar_library(phasar_clang
  SHARED
	${PHASARCLANG_SRC}
)

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
)

set_target_properties(phasar_clang
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
