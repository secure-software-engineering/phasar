file(GLOB_RECURSE DB_SRC *.h *.cpp)

include_directories(
  ${SQLITE3_INCLUDE_DIR}
  )

add_phasar_library(phasar_db
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
  clangFrontend
  clangBasic
  clangCodeGen
  phasar_lib
  ${SQLITE3_LIBRARY}
  mysqlcppconn
  phasar_VTABLE
  )