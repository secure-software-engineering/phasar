#!/bin/bash

# author: Philipp D. Schubert
#
# This script reads translates all .cpp files in the current directory
# in llvm IR. For every .cpp file a corresponding .ll files containt the
# llvm code will be generated.
# If this script is used with the '-mem2reg' parameter, the memory to
# register pass will be applied on the llvm ir after its creation.

for file in *.{cpp,c}; do
		clang -emit-llvm -S ${file} -o ${file%.*}.ll
done
