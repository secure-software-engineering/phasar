#!/bin/bash

set -eo pipefail

source ./utils/safeCommandsSet.sh

readonly PHASAR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
PHASAR_INSTALL_DIR="/usr/local/phasar"
LLVM_INSTALL_DIR="/usr/local/llvm-15"

NUM_THREADS=$(nproc)
LLVM_RELEASE=llvmorg-15.0.7
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
    echo -e "\t-DBOOST_DIR=<path>\t\t- The directory where boost should be installed (optional)"
    echo -e "\t-DBOOST_VERSION=<string>\t- The desired boost version to install (optional)"
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
    -DBOOST_DIR)
    DESIRED_BOOST_DIR="$2"
    shift # past argument
    shift # past value
    ;;
    -DBOOST_DIR=*)
    DESIRED_BOOST_DIR="${key#*=}"
    shift # past argument=value
    ;;
    -DBOOST_VERSION)
    DESIRED_BOOST_VERSION="$2"
    shift # past argument
    shift # past value
    ;;
    -DBOOST_VERSION=*)
    DESIRED_BOOST_VERSION="${key#*=}"
    shift # past argument=value
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

if [ ! -z "${DESIRED_BOOST_DIR}" ]; then
    BOOST_PARAMS="-DBOOST_ROOT=${DESIRED_BOOST_DIR}"
else
# New way of installing boost:
# Check whether we have the required boost packages installed
    { BOOST_VERSION=$(echo -e '#include <boost/version.hpp>\nBOOST_LIB_VERSION' | gcc -s -x c++ -E - 2>/dev/null| grep "^[^#;]" | tr -d '\"'); } || true

	if [ -z "$BOOST_VERSION" ] ;then
        if [ -x "$(command -v pacman)" ]; then
            yes | sudo pacman -Syu --needed boost-libs boost
        else
            if [ -z "$DESIRED_BOOST_VERSION" ] ;then
                sudo apt-get install libboost-graph-dev -y
            else
                # DESIRED_BOOST_VERSION in form d.d, i.e. 1.65 (this is the latest version I found in the apt repo)
                sudo apt-get install "libboost${DESIRED_BOOST_VERSION}-graph-dev" -y
            fi
            #verify installation
            BOOST_VERSION=$(echo -e '#include <boost/version.hpp>\nBOOST_LIB_VERSION' | gcc -s -x c++ -E - 2>/dev/null| grep "^[^#;]" | tr -d '\"')
            if [ -z "$BOOST_VERSION" ] ;then
                echo "Failed installing boost $DESIRED_BOOST_VERSION"
                exit 1
            else
                echo "Successfully installed boost v${BOOST_VERSION//_/.}"
            fi
        fi
	else
        echo "Already installed boost version ${BOOST_VERSION//_/.}"
        if [ -x "$(command -v apt)" ]; then
            DESIRED_BOOST_VERSION=${BOOST_VERSION//_/.}
            # install missing packages if necessary
            boostlibnames=("libboost-graph")
            additional_boost_libs=()
            for boost_lib in ${boostlibnames[@]}; do
                dpkg -s "$boost_lib${DESIRED_BOOST_VERSION}" >/dev/null 2>&1 ||
                dpkg -s "$boost_lib${DESIRED_BOOST_VERSION}.0" >/dev/null 2>&1 ||
                additional_boost_libs+=("$boost_lib${DESIRED_BOOST_VERSION}") ||
                additional_boost_libs+=("$boost_lib${DESIRED_BOOST_VERSION}.0")
                dpkg -s "${boost_lib}-dev" >/dev/null 2>&1 || additional_boost_libs+=("${boost_lib}-dev")
            done
            if [ ${#additional_boost_libs[@]} -gt 0 ] ;then
                echo "Installing additional ${#additional_boost_libs[@]} boost packages: ${additional_boost_libs[*]}"
                sudo apt-get install "${additional_boost_libs[@]}" -y || true
            fi
        fi
	fi
fi

# installing LLVM
tmp_dir=$(mktemp -d "llvm-build.XXXXXXXX" --tmpdir)
./utils/install-llvm.sh "${NUM_THREADS}" "${tmp_dir}" ${LLVM_INSTALL_DIR} ${LLVM_RELEASE}
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
cmake -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "${BOOST_PARAMS}" -DPHASAR_BUILD_UNITTESTS="${DO_UNIT_TEST}" "${LLVM_PARAMS}" "${PHASAR_DIR}"
cmake --build . -j "${NUM_THREADS}"

echo "phasar successfully built"

if ${DO_UNIT_TEST}; then
   echo "Running PhASAR unit tests..."

   NUM_FAILED_TESTS=0

   pushd unittests
   for x in $(find . -type f -executable -print); do
       pushd "${x%/*}" && ./"${x##*/}" || { echo "Test ${x} failed."; NUM_FAILED_TESTS=$((NUM_FAILED_TESTS+1)); };
       popd;
       done
   popd

   echo "Finished running PhASAR unittests"
   echo "${NUM_FAILED_TESTS} tests failed"
fi


if ${DO_INSTALL}; then
    echo "install phasar..."
    sudo cmake -DCMAKE_INSTALL_PREFIX=${PHASAR_INSTALL_DIR} -P cmake_install.cmake
    sudo ldconfig
    safe_cd ..
    echo "phasar successfully installed to ${PHASAR_INSTALL_DIR}"

    echo "Set environment variables"
    ./utils/setEnvironmentVariables.sh ${LLVM_INSTALL_DIR} ${PHASAR_INSTALL_DIR}
fi

echo "done."
