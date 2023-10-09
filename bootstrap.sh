#!/bin/bash

set -eo pipefail

source ./utils/safeCommandsSet.sh

readonly PHASAR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
readonly PHASAR_INSTALL_DIR="/usr/local/phasar"
readonly LLVM_INSTALL_DIR="/usr/local/llvm-14"

NUM_THREADS=$(nproc)
LLVM_RELEASE=llvmorg-14.0.0
DO_UNIT_TEST=true
DO_INSTALL=false
BUILD_TYPE=Release


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
    yes | sudo pacman -Syu --needed which zlib sqlite3 ncurses make python3 doxygen libxml2 swig gcc libedit graphviz python-sphinx openmp python-pip ninja
else
    ./utils/InstallAptDependencies.sh
fi

pip3 install cmake

if [ ! -z "${DESIRED_BOOST_DIR}" ]; then
    BOOST_PARAMS="-DBOOST_ROOT=${DESIRED_BOOST_DIR}"
else
# New way of installing boost:
# Check whether we have the required boost packages installed
    (BOOST_VERSION=$(echo -e '#include <boost/version.hpp>\nBOOST_LIB_VERSION' | gcc -s -x c++ -E - 2>/dev/null| grep "^[^#;]" | tr -d '\"')) || true

	if [ -z "$BOOST_VERSION" ] ;then
        if [ -x "$(command -v pacman)" ]; then
            yes | sudo pacman -Syu --needed boost-libs boost
        else
            if [ -z "$DESIRED_BOOST_VERSION" ] ;then
                sudo apt install libboost-all-dev -y
            else
                # DESIRED_BOOST_VERSION in form d.d, i.e. 1.65 (this is the latest version I found in the apt repo)
                sudo apt install "libboost${DESIRED_BOOST_VERSION}-all-dev" -y
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
            boostlibnames=("libboost-system" "libboost-filesystem"
                    "libboost-graph" "libboost-program-options"
                    "libboost-thread")
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
                sudo apt install "${additional_boost_libs[@]}" -y || true
            fi
        fi
	fi
fi

# installing LLVM
tmp_dir=$(mktemp -d "llvm-build.XXXXXXXX" --tmpdir)
./utils/install-llvm.sh "${NUM_THREADS}" "${tmp_dir}" ${LLVM_INSTALL_DIR} ${LLVM_RELEASE}
rm -rf "${tmp_dir}"
echo "dependencies successfully installed"

echo "Updating the submodules..."
git submodule update --init
echo "Submodules successfully updated"

echo "Building PhASAR..."
${DO_UNIT_TEST} && echo "with unit tests."

export CC=${LLVM_INSTALL_DIR}/bin/clang
export CXX=${LLVM_INSTALL_DIR}/bin/clang++

mkdir -p "${PHASAR_DIR}"/build
safe_cd "${PHASAR_DIR}"/build
cmake -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "${BOOST_PARAMS}" -DPHASAR_BUILD_UNITTESTS="${DO_UNIT_TEST}" "${PHASAR_DIR}"
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
