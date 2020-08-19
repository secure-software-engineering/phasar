#!/bin/bash

set -e

num_cores=1
target_dir=./
re_number="^[0-9]+$"
re_llvm_release="^llvmorg-[0-9]+\.[0-9]+\.[0-9]+$"

num_cores=${1}
dest_dir="${2}"
llvm_release="${3}"

if [ "$#" -ne 3 ] || ! [[ "$num_cores" =~ ${re_number} ]] || ! [[ "${llvm_release}" =~ ${re_llvm_release} ]]; then
    echo "usage: <prog> <# cores> <install dir> <LLVM release (e.g. 'llvmorg-9.0.0')>" >&2
	exit 1
fi

if [ -x ${dest_dir}/bin/llvm-config ]; then
   version=`${dest_dir}/bin/llvm-config --version`
   echo "Found LLVM ${version} already installed at ${dest_dir}."
   addLibraryPath
   exit 0;
fi

echo "Getting the LLVM source code..."
tmp_dir=$(mktemp -d -t phasar-llvm-${llvm_release}-XXXXXXXXXX)
if [ ! -d "${tmp_dir}/llvm-project" ]; then
    echo "Getting the complete LLVM source code"
	git clone https://github.com/llvm/llvm-project.git ${tmp_dir}/llvm-project
fi

echo "Building LLVM..."
cd ${tmp_dir}/llvm-project/
echo "Build the LLVM project"
git checkout ${llvm_release}
mkdir -p build
cd build
cmake -G "Ninja" -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;libcxx;libcxxabi;libunwind;lld;compiler-rt;debuginfo-tests' -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DBUILD_SHARED_LIBS=ON -DLLVM_BUILD_EXAMPLES=Off -DLLVM_INCLUDE_EXAMPLES=Off -DLLVM_BUILD_TESTS=Off -DLLVM_INCLUDE_TESTS=Off -DPYTHON_EXECUTABLE=`which python3` ../llvm
cmake --build . -j${num_cores}

echo "Installing LLVM to ${dest_dir}"
sudo cmake -DCMAKE_INSTALL_PREFIX=${dest_dir} -P cmake_install.cmake
sudo ldconfig
rm -rf ${tmp_dir}
echo "Installed LLVM successfully."
