#!/bin/bash
#
# author: Philipp Schubert
#
# Ensure the overall code quality and an unified look and feel of the PhASAR
# project by running some very useful clang-tidy checks and clang-format on
# the code base.
#

source ./utils/safeCommandsSet.sh

build_dir=${1}
num_jobs=${2}
integer_re="^[0-9]+$"

if [ "$#" -ne 2 ] || ! [ -d "${build_dir}" ] || ! [[ "${num_jobs}" =~ ${integer_re} ]] ; then
	echo "usage: <prog> <build dir> <# jobs>" >&2
	exit 1
fi

echo "Run clang-tidy ..."
safe_cd "${build_dir}" -
run-clang-tidy.py -j "${num_jobs}" -p ./ -header-filter='phasar*.h' -fix
safe_cd -
echo "Run clang-format ..."
./utils/run-clang-format.py

exit 0
