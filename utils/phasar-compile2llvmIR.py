#!/usr/bin/env python3

# author: Philipp D. Schubert

import sys
import json
import re
import ntpath
import os
import subprocess

def main():
    if len(sys.argv) < 2:
        print("usage: <prog> <C/C++ project containing 'compile_commands.json'>")
        sys.exit(1)
    project_dir = sys.argv[1]
    compile_db = project_dir + "compile_commands.json"
    llvm_ir_dir = project_dir + "llvm_ir"
    c_compiler = "clang"
    cpp_compiler = "clang++"
    print(llvm_ir_dir)
    if not os.path.exists(llvm_ir_dir):
        os.makedirs(llvm_ir_dir)
    with open(compile_db) as json_db:
        json_data = json.load(json_db)
        for compilation_unit in json_data:
            arguments = compilation_unit["arguments"]
            # adjust the arguments to our needs
            arguments = prepareArguments(arguments)
            src_file = compilation_unit["directory"] + "/" + compilation_unit["file"]
            compiler = c_compiler if src_file.endswith(".c") else cpp_compiler
            arguments.insert(0, compiler)
            arguments.append(src_file)
            # prepare the output
            arguments.append("-o")
            arguments.append(llvm_ir_dir+"/"+src_file.replace("/", ".").lstrip(".")+".ll")
            print(arguments)
            output = subprocess.check_output(' '.join(arguments), shell=True, stderr=subprocess.STDOUT, cwd=compilation_unit["directory"])
            if output:
              print(output.decode("utf-8"))
    sys.exit(0)

def prepareArguments(args):
    # arguments that can be stripped
    toDelete = ["cc", "-c"]
    a = [i for i in args if i not in toDelete ]
    # delete the standard output file
    a = a[:len(a)-3]
    a.insert(0, "-S")
    a.insert(0, "-emit-llvm")
    return a

main()
