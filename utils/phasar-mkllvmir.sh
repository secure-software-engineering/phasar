#!/bin/bash

# author: Philipp D. Schubert, Richard Leer
#
# This script reads translates all .cpp files in the current directory
# in llvm IR. For every .cpp file a corresponding .ll file containing the
# llvm code will be generated.
# If the '-mem2reg' flag is passed, the memory to register pass will be 
# applied on the llvm IR after its creation.

if [[ "$1" = "-mem2reg" ]]; then
  for file in *.{cpp,c}; do
    if [[ $file == *.cpp ]]; then
      bname=$(basename ${file} .cpp)
      clang++ -std=c++14 -emit-llvm -S -Xclang -disable-O0-optnone ${file} -o ${bname}.ll
    else
      bname=$(basename ${file} .c)
      clang -emit-llvm -S -Xclang -disable-O0-optnone ${file} -o ${bname}.ll
    fi
    opt -mem2reg -S ${bname}.ll -o ${bname}.ll
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
