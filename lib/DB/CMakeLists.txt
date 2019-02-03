file(GLOB_RECURSE DB_SRC *.h *.cpp)

include_directories(
  ${SQLITE3_INCLUDE_DIR}
)

# Compile phasar_db once into an object
add_library(phasar_db_o OBJECT
  ${DB_SRC}
)

add_phasar_library(phasar_db
  # TODO fix circular dependency of DB
  # Do not link phasar_db into a shared object library
  # unitl the circular dependency is fixed.
  # SHARED
  $<TARGET_OBJECTS:phasar_db_o>
)

set(phasar_db_public_dep
  ${Boost_LIBRARIES}
  phasar_passes
  ${SQLITE3_LIBRARY}
  mysqlcppconn
)

target_link_libraries(phasar_db
  LINK_PUBLIC
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
  ${phasar_db_public_dep}
  phasar_utils
)

add_phasar_library(phasar_db_without
  # TODO fix circular dependency of DB
  # Do not link phasar_db into a shared object library
  # unitl the circular dependency is fixed.
  # SHARED
  $<TARGET_OBJECTS:phasar_db_o>
)

target_link_libraries(phasar_db_without
  LINK_PUBLIC
  ${phasar_db_public_dep}
  phasar_utils_without
)

set_target_properties(phasar_db
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
