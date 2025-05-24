# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(varalyzer
    varalyzer.cpp
  )
else()
  add_executable(varalyzer
    varalyzer.cpp
  )
endif()

target_link_libraries(varalyzer
  PRIVATE
    phasar
    ${PHASAR_STD_FILESYSTEM}
)

if (NOT PHASAR_IN_TREE)
  install(TARGETS varalyzer)
endif()