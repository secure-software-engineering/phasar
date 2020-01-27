#!/bin/bash
set -e

NUM_THREADS=$(nproc)
LLVM_RELEASE=llvmorg-9.0.0

./utils/InitializeEnvironment.sh
./utils/InstallAptDependencies.sh

sudo pip install Pygments
sudo pip install pyyaml

# installing boost
sudo apt-get install libboost-all-dev -y

# installing LLVM
./utils/install-llvm.sh $NUM_THREADS . $LLVM_RELEASE
# installing wllvm
sudo pip3 install wllvm

echo "dependencies successfully installed"
echo "build phasar..."

export CC=/usr/local/bin/clang
export CXX=/usr/local/bin/clang++

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $NUM_THREADS
echo "phasar successfully built"
echo "install phasar..."
sudo make install
sudo ldconfig
cd ..
echo "phasar successfully installed"