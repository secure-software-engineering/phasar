#!/bin/bash
set -e

sudo apt-get update
sudo apt-get install git -y
sudo apt-get install zlib1g-dev python3 g++ ninja-build cmake -y
