FROM ubuntu:22.04
ARG LLVM_INSTALL_DIR="/usr/local/llvm-14"
LABEL Name=phasar Version=1.0.0

RUN apt -y update && apt install bash sudo -y


WORKDIR /usr/src/phasar
RUN mkdir -p /usr/src/phasar/utils

COPY ./utils/InitializeEnvironment.sh /usr/src/phasar/utils/
RUN ./utils/InitializeEnvironment.sh

RUN apt-get -y install --no-install-recommends \
    cmake \
    ninja-build \
    libstdc++6 \
    libboost-all-dev

COPY ./utils/InstallAptDependencies.sh /usr/src/phasar/utils/
RUN ./utils/InstallAptDependencies.sh

RUN apt-get update && \
    apt-get install -y software-properties-common

RUN apt-key adv --fetch-keys https://apt.llvm.org/llvm-snapshot.gpg.key && \
           add-apt-repository -y 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main' && \
           apt-get update && \
           apt-get -y install --no-install-recommends \
              clang-14 \
              llvm-14-dev \
              libllvm14 \
              libclang-common-14-dev \
              libclang-14-dev \
              libclang-cpp14-dev \
              clang-tidy-14 \
              libclang-rt-14-dev

RUN pip3 install Pygments pyyaml



# installing wllvm
RUN pip3 install wllvm

ENV CC=/usr/bin/clang-14
ENV CXX=/usr/bin/clang++-14

COPY . /usr/src/phasar

RUN git submodule init
RUN git submodule update
RUN mkdir -p build && cd build && \
          cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_CXX_COMPILER=$CXX \
            -G Ninja && \
          cmake --build .

ENTRYPOINT [ "./build/tools/phasar-cli/phasar-cli" ]
