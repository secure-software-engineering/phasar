#!/bin/bash
echo "installing phasar dependencies..."

sudo apt-get install zliblg-dev libncurses5-dev sqlite3 libsqlite3-dev libmysqlcppconn-dev bear python3 doxygen graphviz 
# installing boost
wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
tar xvf boost_1_66_0.tar.gz
cd boost_1_66_0/
./bootstrap.sh
sudo ./b2 install
cd ..
# installing LLVM
./install-llvm-8.0.0.sh 4 .
echo "dependencies successfully installed"
echo "build phasar..."
#TODO: Are these paths correct, or are there some env variables for them?
export CC=/usr/local/bin/clang
export CXX=/usr/local/bin/clang++

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc)
echo "phasar successfully built"

#TODO: do we need sudo make install ?