file(GLOB_RECURSE DB_SRC *.h *.cpp)

include_directories(
  ${SQLITE3_INCLUDE_DIR}
)

add_phasar_library(phasar_db
  # TODO fix circular dependency of DB
  # Do not link phasar_db into a shared object library
  # unitl the circular dependency is fixed.
  # SHARED  
  ${DB_SRC}
)

target_link_libraries(phasar_db
  LINK_PUBLIC
  ${Boost_LIBRARIES}
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
  phasar_utils
  phasar_passes
  ${SQLITE3_LIBRARY}
  mysqlcppconn
)

set_target_properties(phasar_db
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
