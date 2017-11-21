#!/usr/bin/env python3

# author: Philipp D. Schubert
#
# This little script reads all C++ related source files from the project
# and formats them using the clang-format tool according to the default 
# 'google' C++ style scheme.
# Caution: it is not intended to call this script directly. It should 
# rather be called by the Makefile.

import os

SRC_DIR = "src/"

cpp_extensions = (".cpp", ".cxx", ".c++", ".cc", ".cp", ".c", ".i", ".ii", ".h", ".h++", ".hpp", ".hxx", ".hh", ".inl", ".inc", ".ipp", ".ixx", ".txx", ".tpp", ".tcc", ".tpl")
for root, dir, files in os.walk(SRC_DIR):
	for file in files:
		if file.endswith(cpp_extensions):
			os.system("clang-format -i " + root + "/" + file)
