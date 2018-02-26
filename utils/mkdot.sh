#!/bin/bash

# author: Philipp D. Schubert
#
# This script reads all .dot files in the current directory and
# converts them into .png files that can then be displayed.

echo "create png(s) from dot"
for dot_file in *.dot
do
	bname=$(basename ${dot_file} .dot)
	echo "compile ${dot_file} to ${bname}.png"
	dot -Tpng ${dot_file} -o ${bname}.png
done
