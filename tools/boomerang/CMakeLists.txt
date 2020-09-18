# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(boomerang
    boomerang.cpp
  )
else()
  add_executable(boomerang
    boomerang.cpp
  )
endif()

find_package(Boost COMPONENTS log filesystem program_options graph ${BOOST_THREAD} REQUIRED)
target_link_libraries(boomerang
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_db
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_wpds
  phasar_syncpds
  ${PHASAR_PLUGINS_LIB}
  wali
  phasar_passes
  phasar_pointer
  phasar_typehierarchy
  phasar_phasarllvm_utils
  phasar_utils
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
)

if(USE_LLVM_FAT_LIB)
  llvm_config(boomerang USE_SHARED ${LLVM_LINK_COMPONENTS})
else()
  llvm_config(boomerang ${LLVM_LINK_COMPONENTS})
endif()

set(LLVM_LINK_COMPONENTS
)
