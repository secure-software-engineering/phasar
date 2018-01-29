# Build using
# $> docker build -f docker/Dockerfile -t <lovely-name> .
# in the root path of the repository.
#

FROM ubuntu:16.04
MAINTAINER Ben Hermann <Ben.Hermann@uni-paderborn.de>

ENV LANG=en_US.UTF-8

RUN apt-get update

# general dependencies
RUN apt-get -y install zlib1g-dev libncurses5-dev sqlite3 libsqlite3-dev bear python3 doxygen graphviz

# boost dependencies
RUN apt-get -y install wget gcc g++

# build and install boost
RUN wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz && \
	tar xvf boost_1_64_0.tar.gz && \
	cd boost_1_64_0/ && \
	./bootstrap.sh && \
	./b2 install

# llvm dependencies
RUN apt-get -y install software-properties-common python-software-properties

# build and install llvm
RUN add-apt-repository 'deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-3.9 main' && \
    wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    apt-get update && \
    apt-get -y install clang-3.9 clang-3.9-doc libclang-common-3.9-dev libclang-3.9-dev libclang1-3.9 libclang1-3.9-dbg libllvm-3.9-ocaml-dev libllvm3.9 libllvm3.9-dbg lldb-3.9 llvm-3.9 llvm-3.9-dev llvm-3.9-doc llvm-3.9-examples llvm-3.9-runtime clang-format-3.9 python-clang-3.9 libfuzzer-3.9-dev libmysqlcppconn-dev && \
    echo "export PATH="/usr/lib/llvm-3.9/bin:$PATH"" >> /root/.bashrc

# framework dependencies
RUN apt-get -y install make libcurl4-gnutls-dev git

# copy source code
ADD . /opt/framework

# build framework
RUN cd /opt/framework && \
    export PATH="/usr/lib/llvm-3.9/bin:$PATH" && \
    echo $PATH && \
    sed -i "s/\/home\/philipp\/GIT-Repos\/phasar/\/opt\/framework/g" /opt/framework/src/config/Configuration.cpp && \
    make clean && \
    make

RUN cd /opt/framework/misc && \
    ./make_config.sh
