#!/bin/bash
set -e

sudo apt update
sudo apt install git -y
sudo apt install zlib1g-dev sqlite3 libsqlite3-dev python3 doxygen graphviz python3-pip libxml2 libxml2-dev libncurses5-dev libncursesw5-dev swig build-essential g++ libedit-dev python3-sphinx libomp-dev ninja-build -y
