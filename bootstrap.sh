#!/bin/bash
set -e
echo "installing phasar dependencies..."

sudo apt-get update
sudo apt-get install zlib1g-dev sqlite3 libsqlite3-dev python3 doxygen graphviz python python-dev python3-pip libxml2 libxml2-dev libncurses5-dev libncursesw5-dev swig build-essential g++ cmake libz3 libz3-dev libedit-dev python-sphinx libomp-dev libcurl4-openssl-dev
sudo pip install Pygments
sudo pip install pyyaml
# installing boost
wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
tar xvf boost_1_66_0.tar.gz
cd boost_1_66_0/
./bootstrap.sh
sudo ./b2 install
cd ..
# installing LLVM
./utils/install-llvm-8.0.0.sh $(nproc) ./utils/
# installing wllvm
sudo pip3 install wllvm

echo "dependencies successfully installed"
echo "build phasar..."

#git submodule init
#git submodule update

export CC=/usr/local/bin/clang
export CXX=/usr/local/bin/clang++

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc)
echo "phasar successfully built"
echo "install phasar..."
sudo make install
cd ..
echo "phasar successfully installed"