#!/bin/bash

set -e

num_cores=1
target_dir=./
re_number="^[0-9]+$"
re_llvm_release="^llvmorg-[0-9]+\.[0-9]+\.[0-9]+$"

if [ "$#" -ne 3 ] || ! [[ "$1" =~ ${re_number} ]] || ! [ -d "$2" ] || ! [[ "$3" =~ ${re_llvm_release} ]]; then
	echo "usage: <prog> <# cores> <directory> <LLVM release (e.g. 'llvmorg-9.0.0')>" >&2
	exit 1
fi

num_cores=$1
target_dir=$2
llvm_release=$3

echo "Getting the LLVM source code..."
if [ ! -d "${target_dir}/llvm-project" ]; then
    echo "Getting the complete LLVM source code"
	git clone https://github.com/llvm/llvm-project.git ${target_dir}/llvm-project
fi
echo "Building LLVM..."
cd ${target_dir}/llvm-project/
echo "Build the LLVM project"
git checkout ${llvm_release}
mkdir -p build
cd build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;libcxx;libcxxabi;libunwind;lld;lldb;compiler-rt;lld;polly;debuginfo-tests;openmp;parallel-libs' -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DBUILD_SHARED_LIBS=ON ../llvm
make -j${num_cores}
# echo "Run all tests"
# make -j3 check-all
echo "Installing LLVM..."
sudo make install
sudo ldconfig
echo "Installed LLVM successfully."
