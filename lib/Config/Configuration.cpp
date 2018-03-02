/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Configuration.cpp
 *
 *  Created on: 04.05.2017
 *      Author: philipp
 */

#include <phasar/Config/Configuration.h>

const string MetaDataKind("phasar.instruction.id");
const string ConfigurationDirectory("../config/");
const string GLIBCFunctionListFileName("glibc_function_list_v1-04.05.17.conf");
const string LLVMIntrinsicFunctionListFileName(
    "llvm_intrinsics_function_list_v1-04.05.17.conf");
const string HeaderSearchPathsFileName("standard_header_paths.conf");
const string CompileCommandsJson("compile_commands.json");
bpo::variables_map VariablesMap;
const string LogFileDirectory("log/");