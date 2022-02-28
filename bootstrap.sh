#!/bin/bash
set -e

source ./utils/safeCommandsSet.sh

readonly PHASAR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
readonly PHASAR_INSTALL_DIR="/usr/local/phasar"
readonly LLVM_INSTALL_DIR="/usr/local/llvm-13"

NUM_THREADS=$(nproc)
LLVM_RELEASE=llvmorg-13.0.0
DO_UNIT_TEST=true


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
    yes | sudo pacman -Syu --needed which zlib sqlite3 ncurses make python3 doxygen libxml2 swig gcc cmake z3 libedit graphviz python-sphinx openmp curl python-pip
    ./utils/installBuildEAR.sh
else
    ./utils/InstallAptDependencies.sh
fi
sudo pip3 install Pygments pyyaml

if [ ! -z "${DESIRED_BOOST_DIR}" ]; then
    BOOST_PARAMS="-DBOOST_ROOT=${DESIRED_BOOST_DIR}"
else
# New way of installing boost:
# Check whether we have the required boost packages installed
    BOOST_VERSION=$(echo -e '#include <boost/version.hpp>\nBOOST_LIB_VERSION' | gcc -s -x c++ -E - 2>/dev/null| grep "^[^#;]" | tr -d '\"')

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
                    "libboost-log" "libboost-thread")
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
                sudo apt install "${additional_boost_libs[@]}" -y
            fi
        fi
	fi
fi

# installing LLVM
tmp_dir=$(mktemp -d "llvm-10_build.XXXXXXXX" --tmpdir)
./utils/install-llvm.sh "${NUM_THREADS}" "${tmp_dir}" ${LLVM_INSTALL_DIR} ${LLVM_RELEASE}
rm -rf "${tmp_dir}"
sudo pip3 install wllvm
echo "dependencies successfully installed"


echo "Building PhASAR..."
${DO_UNIT_TEST} && echo "with unit tests."
git submodule init
git submodule update

export CC=${LLVM_INSTALL_DIR}/bin/clang
export CXX=${LLVM_INSTALL_DIR}/bin/clang++

mkdir -p "${PHASAR_DIR}"/build
safe_cd "${PHASAR_DIR}"/build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "${BOOST_PARAMS}" -DPHASAR_BUILD_UNITTESTS=${DO_UNIT_TEST} "${PHASAR_DIR}"
cmake --build .

if ${DO_UNIT_TEST}; then
   echo "Running PhASAR unit tests..."
   pushd unittests
   for x in $(find . -type f -executable -print); do
       pushd "${x%/*}" && ./"${x##*/}" && popd || { echo "Test ${x} failed, aborting."; exit 1; };
       done
   popd
fi

echo "phasar successfully built"
echo "install phasar..."
sudo cmake -DCMAKE_INSTALL_PREFIX=${PHASAR_INSTALL_DIR} -P cmake_install.cmake
sudo ldconfig
safe_cd ..
echo "phasar successfully installed to ${PHASAR_INSTALL_DIR}"


echo "Set environment variables"
./utils/setEnvironmentVariables.sh ${LLVM_INSTALL_DIR} ${PHASAR_INSTALL_DIR}
