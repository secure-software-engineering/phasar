macro(add_llvm)

  if (NOT PHASAR_IN_TREE)
    # Only search for LLVM if we build out of tree
    find_package(LLVM 14 REQUIRED CONFIG)
    find_library(LLVM_LIBRARY NAMES LLVM PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)

    if(USE_LLVM_FAT_LIB AND ${LLVM_LIBRARY} STREQUAL "LLVM_LIBRARY-NOTFOUND")
      message(WARNING "Did not find requested libLLVM.so. Link against individual modules instead")
      set(USE_LLVM_FAT_LIB OFF)
    elseif(BUILD_SHARED_LIBS AND NOT ${LLVM_LIBRARY} STREQUAL "LLVM_LIBRARY-NOTFOUND")
      message(STATUS "Found consolidated shared LLVM lib ${LLVM_LIBRARY} that will be linked against.")
      set(USE_LLVM_FAT_LIB ON)
    endif()

    if (NOT USE_LLVM_FAT_LIB)
      message(STATUS "Link against individual LLVM modules")
      set(LLVM_REQUIRED_LIBRARIES
        Core
        Support
        BitWriter
        Analysis
        Passes
        Demangle
        Analysis
        IRReader
        Linker
      )
      foreach(lib ${LLVM_REQUIRED_LIBRARIES})
        find_library(LLVM_SMALL_LIB${lib} NAMES LLVM${lib} PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
        if(LLVM_SMALL_LIB${lib} MATCHES "NOTFOUND$")
          list(APPEND LLVM_SMALL_LIB_NOTFOUND "LLVM${lib}")
        endif()
      endforeach()

      if(DEFINED LLVM_SMALL_LIB_NOTFOUND)
        if(${LLVM_LIBRARY} STREQUAL "LLVM_LIBRARY-NOTFOUND")
          message(FATAL_ERROR "Did not find a complete version of LLVM: Did not find the fat lib libLLVM.so, but also did not find the individual modules ${LLVM_SMALL_LIB_NOTFOUND}.")
        else()
          set(USE_LLVM_FAT_LIB ON)
          list(JOIN LLVM_SMALL_LIB_NOTFOUND ", " LLVM_SMALL_LIB_NOTFOUND_PRETTY)
          message(WARNING "Did not find the LLVM modules ${LLVM_SMALL_LIB_NOTFOUND_PRETTY}. Fallback to link against ${LLVM_LIBRARY}. To silence this warning, set -DUSE_LLVM_FAT_LIB=ON in the cmake invocation.")
        endif()
      endif(DEFINED LLVM_SMALL_LIB_NOTFOUND)
    endif(NOT USE_LLVM_FAT_LIB)
  endif(NOT PHASAR_IN_TREE)

  if(NOT LLVM_ENABLE_RTTI AND NOT PHASAR_IN_TREE)
    message(FATAL_ERROR "PhASAR requires a LLVM version that is built with RTTI")
  endif()

endmacro()

macro(add_clang)
  # The clang-cpp shared library is now the preferred way to link dynamically against libclang if we build out of tree.
  if(NOT PHASAR_IN_TREE)
    find_library(CLANG_LIBRARY NAMES clang-cpp libclang-cpp HINTS ${LLVM_LIBRARY_DIRS})
    if(${CLANG_LIBRARY} STREQUAL "CLANG_LIBRARY-NOTFOUND")
      set(NEED_LIBCLANG_COMPONENT_LIBS ON)
    endif()
  endif()
  # As fallback, look for the small clang libraries
  if(PHASAR_IN_TREE OR NEED_LIBCLANG_COMPONENT_LIBS)
    set(CLANG_LIBRARY
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
      LLVMFrontendOpenMP)
  endif()

  if (PHASAR_IN_TREE)
    # Phasar needs clang headers, specificaly some that are generated by clangs table-gen
    include_directories(
      ${CLANG_INCLUDE_DIR}
      ${PHASAR_SRC_DIR}/../clang/include
      ${PROJECT_BINARY_DIR}/tools/clang/include
    )
  endif()
endmacro(add_clang)
