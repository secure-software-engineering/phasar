#!/bin/bash

set -e

num_cores=1
build_dir=./
re_number="^[0-9]+$"

if [ "$#" -ne 3 ] || ! [[ "${1}" =~ ${re_number} ]] || ! [ -d "${2}" ]; then
	echo "usage: ${0} <# cores> <build directory> <install directory>" >&2
	exit 1
fi

num_cores=${1}
build_dir="${2}"
dest_dir="${3}"

if [ -x ${dest_dir}/bin/llvm-config ]; then
   version=`${dest_dir}/bin/llvm-config --version`
   echo "Found LLVM ${version} already installed at ${dest_dir}."
   exit 0;
fi

echo "Getting the LLVM source code..."
if [ ! -d "${build_dir}/llvm-project" ]; then
	git clone --branch llvmorg-8.0.0 https://github.com/llvm/llvm-project.git ${build_dir}/llvm-project
fi
echo "Building LLVM..."
mkdir -p ${build_dir}/llvm-project/build
cd ${build_dir}/llvm-project/build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;libcxx;libcxxabi;libunwind;lld;lldb;compiler-rt;lld;polly;debuginfo-tests;openmp;parallel-libs' -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DBUILD_SHARED_LIBS=ON -DLLVM_BUILD_LLVM_DYLIB=ON ../llvm

make -j${num_cores}
# echo "Run all tests"
# make -j3 check-all
echo "Installing LLVM..."
sudo cmake -DCMAKE_INSTALL_PREFIX=${dest_dir} -P cmake_install.cmake 
echo "${dest_dir}/lib" | sudo tee /etc/ld.so.conf.d/llvm-8.conf > /dev/null
sudo ldconfig
echo "Installed LLVM successfully."
