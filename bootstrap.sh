#!/bin/bash

set -eo pipefail

source ./utils/safeCommandsSet.sh

readonly PHASAR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
PHASAR_INSTALL_DIR="/usr/local/phasar"
LLVM_INSTALL_DIR="/usr/local/llvm-16"

NUM_THREADS=$(nproc)
LLVM_RELEASE=llvmorg-16.0.6
DO_UNIT_TEST=true
DO_INSTALL=false
BUILD_TYPE=Release


function usage {
    echo "USAGE: ./bootstrap.sh [options]"
    echo ""
    echo "OPTIONS:"
    echo -e "\t--jobs\t\t-j\t\t- Number of parallel jobs used for compilation (default is nproc -- $(nproc))"
    echo -e "\t--unittest\t-u\t\t- Build and run PhASARs unit-tests (default is true)"
    echo -e "\t--install\t\t\t- Install PhASAR system-wide after building (default is false)"
    echo -e "\t--help\t\t-h\t\t- Display this help message"
    echo -e "\t-DCMAKE_BUILD_TYPE=<string>\t- The build mode for building PhASAR. One of {Debug, RelWithDebInfo, Release} (default is Release)"
    echo -e "\t-DPHASAR_INSTALL_DIR=<path>\t- The folder where to install PhASAR if --install is specified (default is ${PHASAR_INSTALL_DIR})"
    echo -e "\t-DLLVM_INSTALL_DIR=<path>\t- The folder where to install LLVM if --install is specified (default is ${LLVM_INSTALL_DIR})"
}

# Parsing command-line-parameters
# See "https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash" as a reference

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -j|--jobs)
    NUM_THREADS="$2"
    shift # past argument
    shift # past value
    ;;
    -u|--unittest)
    DO_UNIT_TEST=true
    shift # past argument
    ;;
    -DCMAKE_BUILD_TYPE=*)
    BUILD_TYPE="${key#*=}"
    shift # past argument=value
    ;;
    --install)
    DO_INSTALL=true
    shift # past argument
    ;;
    -DPHASAR_INSTALL_DIR)
    PHASAR_INSTALL_DIR="$2"
    shift # past argument
    shift # past value
    ;;
    -DPHASAR_INSTALL_DIR=*)
    PHASAR_INSTALL_DIR="${key#*=}"
    shift # past argument=value
    ;;
    -DLLVM_INSTALL_DIR)
    LLVM_INSTALL_DIR="$2"
    shift # past argument
    shift # past value
    ;;
    -DLLVM_INSTALL_DIR=*)
    LLVM_INSTALL_DIR="${key#*=}"
    shift # past argument=value
    ;;
    -h|--help)
    usage
    exit 0
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters
# End - Parsing command-line-parameters


echo "installing phasar dependencies..."
if [ -x "$(command -v pacman)" ]; then
    yes | sudo pacman -Syu --needed which zlib python3 doxygen gcc ninja cmake
else
    ./utils/InstallAptDependencies.sh
fi

# installing LLVM
tmp_dir=$(mktemp -d "llvm-build.XXXXXXXX" --tmpdir)
./utils/install-llvm.sh "${NUM_THREADS}" "${tmp_dir}" "${LLVM_INSTALL_DIR}" ${LLVM_RELEASE}
rm -rf "${tmp_dir}"

echo "dependencies successfully installed"

# *Always* set the LLVM root to ensure the Phasar script uses the proper toolchain
LLVM_PARAMS=-DLLVM_ROOT="${LLVM_INSTALL_DIR}"

echo "Updating the submodules..."
git submodule update --init
echo "Submodules successfully updated"

echo "Building PhASAR..."
${DO_UNIT_TEST} && echo "with unit tests."

export CC=${LLVM_INSTALL_DIR}/bin/clang
export CXX=${LLVM_INSTALL_DIR}/bin/clang++

mkdir -p "${PHASAR_DIR}"/build
safe_cd "${PHASAR_DIR}"/build
cmake -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DPHASAR_BUILD_UNITTESTS="${DO_UNIT_TEST}" "${LLVM_PARAMS}" "${PHASAR_DIR}"
cmake --build . -j "${NUM_THREADS}"

echo "phasar successfully built"

if ${DO_UNIT_TEST}; then
   echo "Running PhASAR unit tests..."

   NUM_FAILED_TESTS=0

   pushd unittests
   mapfile -t files < <(find . -type f -executable)
   for x in "${files[@]}"; do
       pushd "${x%/*}" && ./"${x##*/}" || { echo "Test ${x} failed."; NUM_FAILED_TESTS=$((NUM_FAILED_TESTS+1)); };
       popd;
       done
   popd

   echo "Finished running PhASAR unittests"
   echo "${NUM_FAILED_TESTS} tests failed"
fi


if ${DO_INSTALL}; then
    echo "install phasar..."
    sudo cmake -DCMAKE_INSTALL_PREFIX="${PHASAR_INSTALL_DIR}" -P cmake_install.cmake
    sudo ldconfig
    safe_cd ..
    echo "phasar successfully installed to ${PHASAR_INSTALL_DIR}"

    echo "Set environment variables"
    ./utils/setEnvironmentVariables.sh "${LLVM_INSTALL_DIR}" "${PHASAR_INSTALL_DIR}"
fi

echo "done."
