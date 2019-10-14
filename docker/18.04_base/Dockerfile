# Note: This creates an image with a user "phasar" with password also "phasar"
#
# Generally the first thing you'll want to do after is run the bootstrap.sh
# script to compile phasar from the src directory to compile phasar.
#
# This dockerfile is intended to setup a minimal environment for successfully 
# compiling on a Ubuntu 18.04 system.  It does not initiate a compile 
# (which is a long process, mostly due to the need to compile LLVM), because
# we want that to run as a normal user, however the bootstrap script also
# requires sudo access.


FROM ubuntu:18.04


RUN apt-get update

# We need an updated version of cmake, other than git & sudo these dependencies are for installing that repository
# We also need wget for other things, that one overlaps.
RUN apt-get install git sudo apt-utils wget apt-transport-https ca-certificates gnupg software-properties-common -y
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt-get update
RUN apt-get upgrade -y

RUN useradd -ms /bin/bash phasar -p "$(openssl passwd -1 phasar)" -G sudo

USER phasar
WORKDIR /home/phasar

RUN mkdir -p src
# ******* NOTE *******  This should switch back to the regular repo and master
RUN git clone --branch buildCleanup https://github.com/ddiepo-pjr/phasar.git /home/phasar/src



USER phasar
WORKDIR /home/phasar
CMD ["/bin/bash"]
