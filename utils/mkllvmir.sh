#!/bin/bash

# author: Philipp D. Schubert
#
# This script reads translates all .cpp files in the current directory
# in llvm IR. For every .cpp file a corresponding .ll files containt the
# llvm code will be generated.
# If this script is used with the '-mem2reg' parameter, the memory to
# register pass will be applied on the llvm ir after its creation.

if [ "$1" = "-mem2reg" ]; then
	echo "compile to llvm IR and mem2reg-transform"
	for src_file in *.cpp
	do
		bname=$(basename ${src_file} .cpp)
		clang++ -std=c++11 -emit-llvm -S ${src_file} -o ${bname}.ll.pp
		opt -mem2reg -S ${bname}.ll.pp &> ${bname}.ll
		rm ${bname}.ll.pp
	done
elif [ "$1" = "" ]; then
	echo "compile to llvm IR"
	for src_file in *.cpp
	do
		clang++ -std=c++11 -emit-llvm -S ${src_file}
	done
elif [ "$1" = "-clean" ]; then
	rm *.ll
else
	echo "unrecognized command, abort!"
fi
