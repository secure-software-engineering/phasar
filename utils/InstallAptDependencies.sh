#!/bin/bash
set -e

#sudo apt-get update
sudo apt-get install git make cmake -y
#echo "-------------------------------------------"
#git --version
#echo "-------------------------------------------"
sudo apt-get install zlib1g-dev sqlite3 libsqlite3-dev libmysqlcppconn-dev bear python3 doxygen graphviz python python-dev python3-pip python-pip libxml2 libxml2-dev libncurses5-dev libncursesw5-dev swig build-essential g++ cmake libz3-dev libz3-4 z3 libedit-dev python-sphinx libomp-dev libcurl4-openssl-dev -y