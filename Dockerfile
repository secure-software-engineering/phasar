FROM ubuntu:latest

LABEL Name=phasar Version=1.0.0

RUN apt-get -y update && apt-get install -y

SHELL ["/bin/bash", "-c"] 
RUN apt-get install bash sudo -y
ADD ./InitializeEnvironment.sh /usr/src/phasar/
ADD ./InstallAptDependencies.sh /usr/src/phasar/
RUN sudo apt-get install libz3-4 z3 -y

WORKDIR /usr/src/phasar
RUN ./InitializeEnvironment.sh
RUN ./InstallAptDependencies.sh
RUN sudo apt-get install libboost-all-dev -y

COPY . /usr/src/phasar

RUN ./bootstrap_docker.sh 

ENTRYPOINT [ "./build/phasar" ]
