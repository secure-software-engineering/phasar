#!/bin/bash
set -e

sudo apt-get update
sudo apt-get install git -y
sudo apt-get install zlib1g-dev sqlite3 libsqlite3-dev python3 doxygen python3-pip g++ ninja-build cmake -y
