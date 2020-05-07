#!/usr/bin/env python3
#
# author: Philipp Schubert
#
# This script reads all C++ related source files from the project
# and formats them using the clang-format tool according to C++ style scheme
# defined in the .clang-format files of the parent directories.
#

import os
import sys

SCRIPT_PATH = os.path.dirname(os.path.realpath(sys.path[0]))

SRC_DIRS = (SCRIPT_PATH + "/include/phasar",
            SCRIPT_PATH + "/lib/",
            SCRIPT_PATH + "/unittests/"
            )

cpp_extensions = (".cpp",
                  ".cxx",
                  ".c++",
                  ".cc",
                  ".cp",
                  ".c",
                  ".i",
                  ".ii",
                  ".h",
                  ".h++",
                  ".hpp",
                  ".hxx",
                  ".hh",
                  ".inl",
                  ".inc",
                  ".ipp",
                  ".ixx",
                  ".txx",
                  ".tpp",
                  ".tcc",
                  ".tpl")

for SRC_DIR in SRC_DIRS:
    for root, dir, files in os.walk(SRC_DIR):
        for file in files:
            if file.endswith(cpp_extensions):
                print("clang-format -i -style=file " + root + "/" + file)
                os.system("clang-format -i -style=file " + root + "/" + file)

sys.exit(0)
