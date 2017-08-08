/*
 * Configuration.cpp
 *
 *  Created on: 04.05.2017
 *      Author: philipp
 */

#include "Configuration.hh"

const string MetaDataKind("ourframework.id");
const string ConfigurationDirectory("config/");
const string GLIBCFunctionListFileName("glibc_function_list_v1-04.05.17.conf");
const string LLVMIntrinsicFunctionListFileName("llvm_intrinsics_function_list_v1-04.05.17.conf");
const string HeaderSearchPathsFileName("standard_header_paths.conf");
const string CompileCommandsJson("compile_commands.json");
bpo::variables_map VariablesMap;
const string LogFileDirectory("log/");