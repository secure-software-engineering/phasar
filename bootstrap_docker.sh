#!/bin/bash
set -e

NUM_THREADS=$(nproc)


sudo pip install Pygments
sudo pip install pyyaml
# installing boost
#wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
#tar xvf boost_1_66_0.tar.gz
#cd boost_1_66_0/
#./bootstrap.sh
#sudo ./b2 install
#cd ..


# installing LLVM
./utils/install-llvm-8.0.0.sh $NUM_THREADS ./utils/
# installing wllvm
sudo pip3 install wllvm

echo "dependencies successfully installed"
echo "build phasar..."

#git submodule init
#git submodule update

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