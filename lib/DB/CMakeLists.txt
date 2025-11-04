file(GLOB_RECURSE DB_SRC *.h *.cpp)
if(PHASAR_BUILD_MODULES OR NOT "${DB_SRC}" STREQUAL "")
  add_phasar_library(phasar_db
    ${DB_SRC}

    LINKS phasar_passes
      phasar_utils

    LLVM_LINK_COMPONENTS
      Support

    MODULE_FILES
      DB.cppm
  )
endif()
