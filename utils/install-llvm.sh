#!/bin/bash

set -e

num_cores=1
target_dir=./
re_number="^[0-9]+$"
re_llvm_release="^llvmorg-[0-9]+\.[0-9]+\.[0-9]+$"

num_cores=${1}
readonly build_dir="${2}"
readonly dest_dir="${3}"
readonly llvm_release="${4}"

if [ "$#" -ne 4 ] || ! [[ "$num_cores" =~ ${re_number} ]] || ! [ -d "${build_dir}" ] || ! [[ "${llvm_release}" =~ ${re_llvm_release} ]]; then
	echo "usage: <prog> <# cores> <build dir> <install dir> <LLVM release (e.g. 'llvmorg-9.0.0')>" >&2
	exit 1
fi

readonly llvm_version=${llvm_release##*-} # i.e. 10.0.0, if llvm_release is "llvmorg-10.0.0"
readonly llvm_major_rev=${llvm_version%%.*}  # i.e. 10, if llvm_release is "llvmorg-10.0.0"

function addLibraryPath {
   #libclang.so.<major rev> has been part of LLVM for a while, and we expect it to stick around -- so this should work for checking to make sure the library is available.
   if ! ldconfig -p |grep -q libclang.so.${llvm_major_rev}; then 
       echo "libLLVM-${llvm_major_rev}.so not found in ldconfig. Trying to add it."; 
       echo "${dest_dir}/lib" | sudo tee /etc/ld.so.conf.d/llvm-${llvm_version}.conf > /dev/null
       sudo ldconfig
       ldconfig -p |grep -q libclang.so.${llvm_major_rev} && echo "done." || echo "WARNING: Failed to add LLVM library path"
   fi
}


if [ -x ${dest_dir}/bin/llvm-config ]; then
   version=`${dest_dir}/bin/llvm-config --version`
   echo "Found LLVM ${version} already installed at ${dest_dir}."
   addLibraryPath
   exit 0;
fi

echo "Getting the LLVM source code..."
if [ ! -d "${build_dir}/llvm-project" ]; then
    echo "Getting the complete LLVM source code"
	git clone https://github.com/llvm/llvm-project.git ${build_dir}/llvm-project
fi
echo "Building LLVM..."
cd ${build_dir}/llvm-project/
echo "Build the LLVM project"
git checkout ${llvm_release}
mkdir -p build
cd build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;libcxx;libcxxabi;libunwind;lld;lldb;compiler-rt;lld;polly;debuginfo-tests;openmp;parallel-libs' -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DBUILD_SHARED_LIBS=ON -DLLVM_BUILD_EXAMPLES=Off -DLLVM_INCLUDE_EXAMPLES=Off -DLLVM_BUILD_TESTS=Off -DLLVM_INCLUDE_TESTS=Off ../llvm
make -j${num_cores}
# echo "Run all tests"
# make -j3 check-all
echo "Installing LLVM to ${dest_dir}"
sudo cmake -DCMAKE_INSTALL_PREFIX=${dest_dir} -P cmake_install.cmake

addLibraryPath
echo "Installed LLVM successfully."
