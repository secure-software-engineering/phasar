FROM ubuntu:22.04
ARG LLVM_INSTALL_DIR="/usr/local/llvm-15"
LABEL Name=phasar Version=2403

RUN apt -y update && apt install bash sudo -y


WORKDIR /usr/src/phasar
RUN mkdir -p /usr/src/phasar/utils

COPY ./utils/InitializeEnvironment.sh /usr/src/phasar/utils/
RUN ./utils/InitializeEnvironment.sh

RUN apt-get -y install --no-install-recommends \
    cmake \
    ninja-build \
    libstdc++6 \
    libboost-graph-dev

COPY ./utils/InstallAptDependencies.sh /usr/src/phasar/utils/
RUN ./utils/InstallAptDependencies.sh

RUN apt-get update && \
    apt-get install -y software-properties-common

RUN apt-key adv --fetch-keys https://apt.llvm.org/llvm-snapshot.gpg.key && \
           add-apt-repository -y 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal-15 main' && \
           apt-get update && \
           apt-get -y install --no-install-recommends \
              clang-15 \
              llvm-15-dev \
              libllvm15 \
              libclang-common-15-dev \
              libclang-15-dev \
              libclang-cpp15-dev \
              clang-tidy-15 \
              libclang-rt-15-dev

RUN pip3 install Pygments pyyaml



# installing wllvm
RUN pip3 install wllvm

ENV CC=/usr/bin/clang-15
ENV CXX=/usr/bin/clang++-15

COPY . /usr/src/phasar

RUN git submodule init
RUN git submodule update
RUN mkdir -p build && cd build && \
          cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DPHASAR_TARGET_ARCH="" \
            -DCMAKE_CXX_COMPILER=$CXX \
            -G Ninja && \
          cmake --build .

ENTRYPOINT [ "./build/tools/phasar-cli/phasar-cli" ]
