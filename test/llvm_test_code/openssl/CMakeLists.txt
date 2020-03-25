
# We need OpenSSL's header files to generate the LLVM IR

if(PHASAR_BUILD_OPENSSL_TS_UNITTESTS)
  find_package(OpenSSL REQUIRED)
  message("${OPENSSL_INCLUDE_DIR}")
  include_directories(${OPENSSL_INCLUDE_DIR})

  add_subdirectory(key_derivation)
  add_subdirectory(secure_memory)
  add_subdirectory(secure_heap)
endif(PHASAR_BUILD_OPENSSL_TS_UNITTESTS)
