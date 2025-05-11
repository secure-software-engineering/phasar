file(GLOB_RECURSE TYPEHIERARCHY *.h *.cpp)

# Handle the library files
add_phasar_library(phasar_typehierarchy
  ${TYPEHIERARCHY}

  MODULE_FILES
    TypeHierarchy.cppm
)
