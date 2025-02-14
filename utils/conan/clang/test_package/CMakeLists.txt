cmake_minimum_required(VERSION 3.15)

project(test_package LANGUAGES CXX)

#if (NOT LLVM_HAS_RTTI)
#    if (MSVC)
#        string(APPEND CMAKE_CXX_FLAGS " -fno-rtti")
#    else()
#endif()
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")

find_package(Clang REQUIRED COMPONENTS clangTooling)

add_executable(${PROJECT_NAME} test_package.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE clangTooling)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
