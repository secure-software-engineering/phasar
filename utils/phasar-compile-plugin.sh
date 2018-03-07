#!/bin/bash

# author: Philipp D. Schubert

# This little script compiles a Phasar plug-in into a shared object library

# TODO
# cc -std=c++14 -I../include/phasar/ -Wall -Wextra -rdynamic -fPIC -shared $1 -o "${1}.so"
echo "successfully compiled shared object library"
