#!/bin/bash

num_cores=1
target_dir=./
re_number="^[0-9]+$"

if [ "$#" -ne 2 ] || ! [[ "$1" =~ ${re_number} ]] || ! [ -d "$2" ]; then
	echo "usage: <prog> <# cores> <directory>" >&2
	exit 1
fi

num_cores=$1
target_dir=$2

echo "Getting the complete LLVM source code"
echo "Get llvm"
svn co http://llvm.org/svn/llvm-project/llvm/tags/RELEASE_501/final/ "${target_dir}/llvm-5.0.1"
cd ${target_dir}/llvm-5.0.1/tools
echo "Get clang"
svn co http://llvm.org/svn/llvm-project/cfe/tags/RELEASE_501/final/ clang
cd clang/tools
echo "Get clang-tools-extra"
svn co http://llvm.org/svn/llvm-project/clang-tools-extra/tags/RELEASE_501/final/ extra
cd ../..
echo "Get lld"
svn co http://llvm.org/svn/llvm-project/lld/tags/RELEASE_501/final/ lld
echo "Get polly"
svn co http://llvm.org/svn/llvm-project/polly/tags/RELEASE_501/final/ polly
cd ../projects
echo "Get compiler-rt"
svn co http://llvm.org/svn/llvm-project/compiler-rt/tags/RELEASE_501/final compiler-rt
echo "Get openmp"
svn co http://llvm.org/svn/llvm-project/openmp/tags/RELEASE_501/final openmp
echo "Get libcxx"
svn co http://llvm.org/svn/llvm-project/libcxx/tags/RELEASE_501/final libcxx
echo "Get libcxxabi"
svn co http://llvm.org/svn/llvm-project/libcxxabi/tags/RELEASE_501/final libcxxabi
echo "Get test-suite"
svn co http://llvm.org/svn/llvm-project/test-suite/tags/RELEASE_501/final test-suite
cd ..
echo "Get new-ld with plugin support"
git clone --depth 1 git://sourceware.org/git/binutils-gdb.git binutils
cd binutils
mkdir build
cd build
echo "build binutils"
../configure --disable-werror
make -j${num_cores} all-ld
cd ../..
echo "LLVM source code and plugins are set up"
echo "Build the LLVM project"
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_BINUTILS_INCDIR=$(pwd)/../binutils/include ..
make -j${num_cores}
echo "Run all tests"
# make -j3 check-all
echo "Installing LLVM"
sudo make install
echo "Successfully installed LLVM"
