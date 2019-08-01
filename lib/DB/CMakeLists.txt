file(GLOB_RECURSE DB_SRC *.h *.cpp)

include_directories(
  ${SQLITE3_INCLUDE_DIR}
)

add_phasar_library(phasar_db
  SHARED
  ${DB_SRC}
)

target_link_libraries(phasar_db
  LINK_PUBLIC
  phasar_pointer
  phasar_passes
  phasar_utils

  LLVMSupport
  LLVMCore
  LLVMVectorize
  LLVMScalarOpts
  LLVMInstrumentation
  LLVMObjCARCOpts
  LLVMTransformUtils
  LLVMCodeGen
  LLVMAnalysis
  LLVMipo
  LLVMIRReader
  LLVMInstCombine
  LLVMLinker

  ${Boost_LIBRARIES}
  ${SQLITE3_LIBRARY}
)

set_target_properties(phasar_db
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
