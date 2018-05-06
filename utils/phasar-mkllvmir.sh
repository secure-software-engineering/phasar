#!/bin/bash

# author: Philipp D. Schubert, Richard Leer
#
# This script reads translates all .cpp files in the current directory
# in llvm IR. For every .cpp file a corresponding .ll file containing the
# llvm code will be generated.
# If the '-mem2reg' flag is passed, the memory to register pass will be 
# applied on the llvm IR after its creation.
#
# Calling this script with the '-unittest' flag, will automatically
# generate all necessary .ll files needed for the unittests, i.e.
# for all example programs in the test/llvm_test_code/ directory. 
# Additionally, this script will recognize 'mem2reg' annotations
# inside a cpp file, and apply the mem2reg pass accordingly.
# Note that this happens automatically when building Phasar's
# unittests!

if [[ "$1" = "-mem2reg" ]]; then
  for file in *.{cpp,c}; do
    if [[ $file == *.cpp ]]; then
      bname=$(basename ${file} .cpp)
      clang++ -std=c++14 -emit-llvm -S ${file} -o ${bname}.ll.pp
    else
      bname=$(basename ${file} .c)
      clang -emit-llvm -S ${file} -o ${bname}.ll.pp
    fi
    opt -mem2reg -S ${bname}.ll.pp &> ${bname}.ll
    rm ${bname}.ll.pp
  done
elif [[ "$1" = "-unittest" ]]; then
  for file in $( find "../test/llvm_test_code/" -name "*.cpp" -o -name "*.c" ); do
    if grep -q "mem2reg" ${file}; then
      if [[ $file == *.cpp ]]; then
        bname=$(basename ${file} .cpp)
        clang -std=c++14 -emit-llvm -S ${file} -o ${bname}.ll.pp
      else
        bname=$(basename ${file} .c)
        clang -emit-llvm -S ${file} -o ${bname}.ll.pp
      fi
      opt -mem2reg -S ${bname}.ll.pp &> $(dirname ${file})"/${bname}.ll"
      rm ${bname}.ll.pp
    else
      if [[ $file == *.cpp ]]; then
        clang++ -std=c++14 -emit-llvm -S ${file} -o "${file%.*}.ll"
      else
        clang -emit-llvm -S ${file} -o "${file%.*}.ll"
      fi
    fi
  done
elif [[ "$1" = "" ]]; then
  for file in *.{cpp,c}; do
    if [[ $file == *.cpp ]]; then
      clang++ -std=c++14 -emit-llvm -S ${file}
    else
      clang -emit-llvm -S ${file}
    fi
  done
else 
  echo "unrecognized command, abort!"
fi
