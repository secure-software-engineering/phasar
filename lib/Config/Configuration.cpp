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

#include <boost/filesystem.hpp>

#include <phasar/Config/Configuration.h>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

using namespace psr;

namespace psr {

const std::string PhasarVersion("PhASAR v1118");
const std::string MetaDataKind("phasar.instruction.id");
const std::string ConfigurationDirectory([]() {
  std::string phasar_config =
      std::string(std::getenv("HOME")) + "/.config/phasar/";
  return (bfs::exists(phasar_config) && bfs::is_directory(phasar_config))
             ? phasar_config
             : "config/";
}());
const std::string PhasarDirectory([]() {
  std::string curr_path = bfs::current_path().string();
  size_t i = curr_path.rfind("build", curr_path.length());
  return curr_path.substr(0, i);
}());
const std::string
    GLIBCFunctionListFileName("glibc_function_list_v1-04.05.17.conf");
const std::string LLVMIntrinsicFunctionListFileName(
    "llvm_intrinsics_function_list_v1-04.05.17.conf");
const std::string HeaderSearchPathsFileName("standard_header_paths.conf");
const std::string CompileCommandsJson("compile_commands.json");
bpo::variables_map VariablesMap;
const std::string LogFileDirectory("log/");
const std::string DefaultSourceSinkFunctionsPath(
    PhasarDirectory + "config/phasar-source-sink-function.json");
const std::string JsonCallGraphID("CallGraph");
const std::string JsonTypeHierarchyID("TypeHierarchy");
const std::string JsonPointToGraphID("PointsToGraph");
const std::string JsonDataFlowID("DataFlowInformation");

} // namespace psr
