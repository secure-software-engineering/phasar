FROM ubuntu:latest
ARG LLVM_INSTALL_DIR="/usr/local/llvm-10"
LABEL Name=phasar Version=1.0.0

RUN apt -y update && apt install bash sudo -y

WORKDIR /usr/src/phasar
RUN mkdir -p /usr/src/phasar/utils

COPY ./utils/InitializeEnvironment.sh /usr/src/phasar/utils/
RUN ./utils/InitializeEnvironment.sh

COPY ./utils/InstallAptDependencies.sh /usr/src/phasar/utils/
RUN ./utils/InstallAptDependencies.sh

RUN pip3 install Pygments pyyaml

# installing boost
RUN apt install libboost-all-dev -y

# installing LLVM
COPY utils/install-llvm.sh /usr/src/phasar/utils/install-llvm.sh
RUN ./utils/install-llvm.sh $(nproc) ${LLVM_INSTALL_DIR} "llvmorg-10.0.0"

# installing wllvm
RUN pip3 install wllvm

ENV CC=${LLVM_INSTALL_DIR}/bin/clang
ENV CXX=${LLVM_INSTALL_DIR}/bin/clang++
ENV LD_LIBRARY_PATH=${LLVM_INSTALL_DIR}/lib:$LD_LIBRARY_PATH

COPY . /usr/src/phasar

RUN git submodule init
RUN git submodule update
RUN mkdir -p build &&                       \
    cd build &&                             \
    cmake -DCMAKE_BUILD_TYPE=Release .. &&  \
    make -j $(nproc) &&                     \
    make install &&                         \
    ldconfig

ENTRYPOINT [ "./build/tools/phasar-llvm/phasar-llvm" ]
