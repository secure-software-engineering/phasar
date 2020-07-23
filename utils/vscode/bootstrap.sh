#!/bin/bash

readonly PHASAR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../../" >/dev/null 2>&1 && pwd )"

cd ${PHASAR_DIR}

mkdir -p .vscode
cp -u utils/vscode/launch.json .vscode/launch.json
cp -u utils/vscode/tasks.json .vscode/tasks.json
cp -u utils/vscode/main.cpp main.cpp

chmod -R 777 .vscode
chmod 777 main.cpp
chmod -R 777 build
cd build/
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPHASAR_BUILD_UNITTESTS=false ${PHASAR_DIR}
cd /
chmod -R 777 /usr/local/phasar

echo "/usr/local/phasar/lib" > /etc/ld.so.conf.d/phasar.conf
echo "/usr/local/llvm-10/lib" > /etc/ld.so.conf.d/llvm-10.conf
sudo ldconfig