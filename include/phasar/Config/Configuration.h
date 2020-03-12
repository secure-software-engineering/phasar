/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Configuration.hh
 *
 *  Created on: 04.05.2017
 *      Author: philipp
 */

#ifndef PHASAR_CONFIG_CONFIGURATION_H_
#define PHASAR_CONFIG_CONFIGURATION_H_

#include <string>

#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"

#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/ManagedStatic.h"

#include "phasar/Config/Version.h"

#define XSTR(S) STR(S)
#define STR(S) #S

namespace psr {

class PhasarConfig {
public:
  /// Current Phasar version
  static const std::string PhasarVersion() { return XSTR(PHASAR_VERSION); }

  /// Stores the label/ tag with which we annotate the LLVM IR.
  static const std::string MetaDataKind() { return "psr.id"; }

  static const std::string ConfigurationDirectory() {
    return configuration_directory;
  }

  /// Specifies the directory in which Phasar is located.
  static const std::string PhasarDirectory() { return phasar_directory; }

  /// Name of the file storing all standard header search paths used for
  /// compilation.
  static const std::string HeaderSearchPathsFileName() {
    return "standard_header_paths.conf";
  }

  /// Name of the compile_commands.json file (in case we wish to rename)
  static const std::string CompileCommandsJson() {
    return "compile_commands.json";
  }

  /// Default Source- and Sink-Functions path
  static const std::string DefaultSourceSinkFunctionsPath() {
    return std::string(PhasarDirectory() +
                       "config/phasar-source-sink-function.json");
  }

  // Variables to be used in JSON export format
  /// Identifier for call graph export
  static const std::string JsonCallGraphID() { return "CallGraph"; }

  /// Identifier for type hierarchy graph export
  static const std::string JsonTypeHierarchyID() { return "TypeHierarchy"; }

  /// Identifier for points-to graph export
  static const std::string JsonPointToGraphID() { return "PointsToGraph"; }

  /// Identifier for data-flow results export
  static const std::string JsonDataFlowID() { return "DataFlowInformation"; }

  static PhasarConfig &getPhasarConfig();

  llvm::iterator_range<std::set<std::string>::iterator> specialFunctionNames() {
    return llvm::make_range(special_function_names.begin(),
                            special_function_names.end());
  }

  /// Add a function name to the special functions list.
  /// Special functions are functions that cannot directly be analyzed but need
  /// to be handled by the analysis.
  ///
  /// Remark: Manually added special functions need to be added before creating
  /// the analysis.
  void addSpecialFunctionName(std::string SFName) {
    special_function_names.insert(std::move(SFName));
  }

  /// Variables map of the parsed command-line parameters
  static boost::program_options::variables_map &VariablesMap() {
    static boost::program_options::variables_map variables_map;
    return variables_map;
  }

  ~PhasarConfig() = default;
  PhasarConfig(const PhasarConfig &) = delete;
  PhasarConfig(PhasarConfig &&) = delete;

private:
  PhasarConfig();

  std::string readConfigFile(const std::string &path);
  void loadGlibcSpecialFunctionNames();
  void loadLLVMSpecialFunctionNames();

  std::set<std::string> special_function_names;

  /// Specifies the directory in which important configuration files are
  /// located.
  inline static const std::string configuration_directory = []() {
    char *env_home = std::getenv("HOME");
    std::string config_folder = "config/";
    if (env_home) { // Check if HOME was defined in the environment
      std::string phasar_config = std::string(env_home) + "/.config/phasar/";
      if (boost::filesystem::exists(phasar_config) &&
          boost::filesystem::is_directory(phasar_config)) {
        config_folder = phasar_config;
      }
    }
    return config_folder;
  }();

  /// Specifies the directory in which Phasar is located.
  static const std::string phasar_directory;

  /// Name of the file storing all glibc function names.
  const std::string GLIBCFunctionListFileName =
      "glibc_function_list_v1-04.05.17.conf";

  /// Name of the file storing all LLVM intrinsic function names.
  const std::string LLVMIntrinsicFunctionListFileName =
      "llvm_intrinsics_function_list_v1-04.05.17.conf";

  /// Log file directory
  const std::string LogFileDirectory = "log/";
};

} // namespace psr

#endif
