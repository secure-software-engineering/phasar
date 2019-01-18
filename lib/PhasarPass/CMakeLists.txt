file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

add_phasar_library(phasar_pass
	SHARED
	${PHASARPASS_SRC}
)

# Fix boost_thread dependency for MacOS
if(APPLE)
  set(BOOST_THREAD boost_thread-mt)
else()
  set(BOOST_THREAD boost_thread)
endif()

### Adding external libraries
# Threads
find_package(Threads)

# Boost
find_package(Boost COMPONENTS filesystem graph system program_options log thread REQUIRED)
include_directories(${BOOST_INCLUDE_DIR})
add_definitions(-DBOOST_LOG_DYN_LINK)

# Curl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

# SQL
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

# LLVM
find_package(LLVM REQUIRED CONFIG)
find_library(LLVM_LIBRARY NAMES llvm REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIB_PATH} ${LLVM_LIBRARY_DIRS})

llvm_map_components_to_libnames(llvm_libs
  #coverage
  #coroutines
  #libdriver
  #lto
  #support
  #analysis
  #bitwriter
  core
  #ipo
  #irreader
  #instcombine
  #instrumentation
  #linker
  #objcarcopts
  #scalaropts
  #transformutils
  #codegen
  #vectorize
)
add_definitions(${LLVM_DEFINITIONS})

# Clang
find_library(CLANG_LIBRARY NAMES clang REQUIRED)
link_directories(${CLANG_LIB_PATH})
set(CLANG_LIBRARIES
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

target_link_libraries(phasar_pass
  phasar_config
  phasar_controller
  phasar_db
  phasar_experimental
  phasar_clang
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_passes
  phasar_pointer
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CLANG_LIBRARIES}
  ${llvm_libs}
  curl
#libphasar_ifdsided.a
#libphasar_controlflowd.a
#libphasar_controllerd.a
#libphasar_dbd.a
#libphasar_configd.a
#libphasar_experimentald.a
#libphasar_monod.a
#libphasar_pointerd.a
#libphasar_pluginsd.a
#libphasar_utilsd.a
#libphasar_passesd.a
)

set_target_properties(phasar_pass
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
