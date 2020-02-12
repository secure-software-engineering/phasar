FROM ubuntu:latest

LABEL Name=phasar Version=1.0.0

RUN apt-get -y update && apt-get install -y && apt-get install bash sudo -y

SHELL ["/bin/bash", "-c"] 

WORKDIR /usr/src/phasar

COPY . /usr/src/phasar

RUN ./bootstrap_docker.sh

ENTRYPOINT [ "./build/phasar-llvm/phasar-llvm" ]
