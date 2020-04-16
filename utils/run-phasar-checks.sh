#!/bin/sh
#
# author: Philipp Schubert
#
# Ensure the overall code quality and an unified look and feel of the PhASAR
# project by running some very useful clang-tidy checks and clang-format on
# the code base.
#

build_dir=${1}

if [ "$#" -ne 1 ] || ! [ -d "${build_dir}" ]; then
	echo "usage: <prog> <build dir>" >&2
	exit 1
fi


# exclude external projects from clang tidy checks
cp .clang-tidy-ignore external/googletest/.clang-tidy
cp .clang-tidy-ignore external/json/.clang-tidy
cp .clang-tidy-ignore external/WALi-OpenNWA/.clang-tidy

echo "Run clang-tidy ..."
cd ${build_dir}
run-clang-tidy.py -p ./ -header-filter='phasar*.h' -fix
cd -
echo "Run clang-format ..."
./utils/run-clang-format.py

rm external/googletest/.clang-tidy
rm external/json/.clang-tidy
rm external/WALi-OpenNWA/.clang-tidy

exit 0
