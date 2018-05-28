#!/usr/bin/env python3

# author: Philipp D. Schubert

import sys
import json
import re
import ntpath
import os
import subprocess

# int main(sys.argv) {
if len(sys.argv) < 2:
    print("usage: <prog> <C/C++ project containing 'compile_commands.json'>")
    sys.exit(1)

project_dir = sys.argv[1]
compile_db = project_dir + "compile_commands.json"
llvm_ir_dir = project_dir + "llvm_ir/"
c_compiler = "clang"
cpp_compiler = "clang++"

if os.path.exists(project_dir) and not os.path.exists(llvm_ir_dir):
    os.makedirs(llvm_ir_dir)

processed_commands = 0

with open(compile_db) as json_db:
    json_data = json.load(json_db)
    for compilation_unit in json_data:
        file_name = ntpath.basename(compilation_unit["file"])
        compiler = c_compiler if file_name.endswith(".c") else cpp_compiler # check if a C or C++ compiler has to be used
        new_file_name = compilation_unit["file"].replace("/", ".")
        new_file_name = new_file_name.lstrip(".")
        command = compilation_unit["command"]
        #command = re.sub(r"cc\s", compiler + " -emit-llvm -S ", command)
        command = re.sub(r"c\+\+\s", compiler + " -emit-llvm -S ", command)
        command = re.sub(r"g\+\+\s", compiler + " -emit-llvm -S ", command)
        command = re.sub(r"cc\s", compiler + " -emit-llvm -S ", command)
		# check if compile command does not contain -o
        if command.find("-o") == -1:
          command = re.sub(re.escape(file_name), " -o " + llvm_ir_dir + new_file_name + ".ll " + compilation_unit["file"], command)
        else:
          command = re.sub(r"-o\s[A-Za-z0-9\/\.\_\-]+.?o?", "-o " + llvm_ir_dir + new_file_name + ".ll ", command)
          command = re.sub(r"\s[A-Za-z0-9\/\.\_\-]+.o\s", " ", command) # remove additional object files that would be linked
          command = re.sub(r"\s[A-Za-z0-9\/\.\_\-]+.d\s", " ", command) # remove additional header dependency files
          command = re.sub(r"\s[A-Za-z0-9\/\.\_\-]+.a\s", " ", command) # remove additional archive files that would be linked
        command = command.replace("clanclang++", "clang++")
        command = command.replace("-MMD", "")
        command = command.replace("-MT", "")
        command = command.replace("-MF", "")
        print(command+'\n')
        output = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT)
        processed_commands += 1
        if output:
          print(output)
compiled_files = len(os.listdir(llvm_ir_dir))
if compiled_files != processed_commands:
    print("inconsistency detected: number of compiled files: " + str(compiled_files) + " does not match number of compile commands: " + str(processed_commands))
    sys.exit(1)
print("successfully compiled whole compile commands database")

sys.exit(0)
# }