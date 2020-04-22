#!/bin/bash
set -e

sudo apt update
sudo apt install git make cmake -y
sudo apt install zlib1g-dev sqlite3 libsqlite3-dev libmysqlcppconn-dev bear python3 doxygen graphviz python3-pip libxml2 libxml2-dev libncurses5-dev libncursesw5-dev swig build-essential g++ cmake libz3-dev libz3-4 z3 libedit-dev python3-sphinx libomp-dev libcurl4-openssl-dev -y
