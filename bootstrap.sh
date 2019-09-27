#!/bin/bash
# Run with "-b <boost directory>" to use a pre-existing boost install.
# Run with "-u" to build & run unit tests
set -e

# Get the directory where this script lives.
readonly PHASAR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
readonly LLVM_INSTALL="/usr/local/llvm-8"


BOOST_INSTALL=""
DO_UNIT_TEST=false

function install_boost()
{
   BOOST_INSTALL="/usr/local/boost_1_66_0"
   if [ -d ${BOOST_INSTALL} ]; then
       echo "Found boost already installed at: ${BOOST_INSTALL}"
       return 0;
   fi
   
   echo "Installing Boost..."
   if [ ! -f boost_1_66_0.tar.gz ]
   then
	  wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
   fi
   if [ ! -d boost_1_66_0/ ]
   then
	  tar xvf boost_1_66_0.tar.gz
   fi
   pushd boost_1_66_0/
      ./bootstrap.sh
      sudo ./b2 -j $(nproc) install --prefix=${BOOST_INSTALL}
   popd
}


while getopts “b:u” opt; do
  case $opt in
    b) BOOST_INSTALL=$OPTARG ;;
    u) DO_UNIT_TEST=true
  esac
done

echo "We need sudo to do various install activities.  We only ask for it once!"
sudo -v || exit 1
while true; do
  sudo -nv; sleep 30  # keep sudo from expiring
  kill -0 "$$" 2>/dev/null || exit  # this process should quit once the parent quits
done &  # run subscript in background

echo "Installing PhASAR dependencies..."
echo "Installing dependencies from sources..."
PACKAGES="zlib1g zlib1g-dev sqlite3 libsqlite3-dev python3 doxygen graphviz python python-dev python3-pip python-pip libxml2 libxml2-dev libncurses5-dev libncursesw5-dev swig build-essential g++ cmake libedit-dev python-sphinx libcurl4-openssl-dev"
dpkg-query -l ${PACKAGES} > /dev/null || {
   sudo apt-get update;
   sudo apt-get install -y zlib1g zlib1g-dev sqlite3 libsqlite3-dev python3 doxygen graphviz python python-dev python3-pip python-pip libxml2 libxml2-dev libncurses5-dev libncursesw5-dev swig build-essential g++ cmake libedit-dev python-sphinx libcurl4-openssl-dev;
};

#What are the python packages used for?  Can we install the ubuntu package version?
#sudo pip install Pygments
#sudo pip install pyyaml

if [[ -z ${BOOST_INSTALL} ]]; then
    echo "Installing Boost..."
    install_boost
else
    echo "Using user supplied boost at: ${BOOST_INSTALL}"
fi
echo "Using boost from: ${BOOST_INSTALL}"

echo "Installing LLVM..."
tmp_dir=`mktemp -d "llvm-8_build.XXXXXXXX" --tmpdir`
./utils/install-llvm-8.0.0.sh $(nproc) "${tmp_dir}" "${LLVM_INSTALL}"
rm -rf ${tmp_dir}

echo "Installing WLLVM..."
sudo pip3 install wllvm

echo "Dependencies installed successfully."

echo "Building PhASAR..."
${DO_UNIT_TESTS} && echo "with unit tests."
git submodule init
git submodule update

export CC=${LLVM_INSTALL}/bin/clang
export CXX=${LLVM_INSTALL}/bin/clang++

mkdir -p ${PHASAR_DIR}/build
cd ${PHASAR_DIR}/build
cmake -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=${BOOST_INSTALL} -DPHASAR_BUILD_UNITTESTS=${DO_UNIT_TEST} ${PHASAR_DIR}

make -j $(nproc)
echo "PhASAR built successfully."

if ${DO_UNIT_TEST}; then
   echo "Running PhASAR unit tests..."
   pushd unittests
   for x in `find . -type f -executable -print`; do 
       pushd ${x%/*} && ./${x##*/} && popd || { echo "Test ${x} failed, aborting."; exit 1; };
       done
   popd
fi

echo "Installing PhASAR..."
PHASAR_INSTALL_DIR="/usr/local/phasar"
sudo cmake -DCMAKE_INSTALL_PREFIX=${PHASAR_INSTALL_DIR} -P cmake_install.cmake
cd ..

echo "${PHASAR_INSTALL_DIR}/lib" | sudo tee /etc/ld.so.conf.d/phasar.conf > /dev/null
sudo ldconfig

echo "PhASAR installed successfully to ${PHASAR_INSTALL_DIR}"

