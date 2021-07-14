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
  static std::string phasarVersion() { return XSTR(PHASAR_VERSION); }

  /// Stores the label/ tag with which we annotate the LLVM IR.
  static std::string metaDataKind() { return "psr.id"; }

  static std::string configurationDirectory() { return ConfigurationDirectory; }

  /// Specifies the directory in which Phasar is located.
  static std::string phasarDirectory() { return PhasarDir; }

  /// Name of the file storing all standard header search paths used for
  /// compilation.
  static std::string headerSearchPathsFileName() {
    return "standard_header_paths.conf";
  }

  /// Name of the compile_commands.json file (in case we wish to rename)
  static std::string compileCommandsJson() { return "compile_commands.json"; }

  /// Default Source- and Sink-Functions path
  static std::string defaultSourceSinkFunctionsPath() {
    return std::string(phasarDirectory() +
                       "config/phasar-source-sink-function.json");
  }

  // Variables to be used in JSON export format
  /// Identifier for call graph export
  static std::string jsonCallGraphId() { return "CallGraph"; }

  /// Identifier for type hierarchy graph export
  static std::string jsonTypeHierarchyId() { return "TypeHierarchy"; }

  /// Identifier for points-to graph export
  static std::string jsonPointsToGraphId() { return "PointsToGraph"; }

  /// Identifier for data-flow results export
  static std::string jsonDataFlowId() { return "DataFlowInformation"; }

  static PhasarConfig &getPhasarConfig();

  llvm::iterator_range<std::set<std::string>::iterator> specialFunctionNames() {
    return llvm::make_range(SpecialFunctionNames.begin(),
                            SpecialFunctionNames.end());
  }

  /// Add a function name to the special functions list.
  /// Special functions are functions that cannot directly be analyzed but need
  /// to be handled by the analysis.
  ///
  /// Remark: Manually added special functions need to be added before creating
  /// the analysis.
  void addSpecialFunctionName(std::string SFName) {
    SpecialFunctionNames.insert(std::move(SFName));
  }

  /// Variables map of the parsed command-line parameters
  static boost::program_options::variables_map &variablesMap() {
    static boost::program_options::variables_map VariablesMap;
    return VariablesMap;
  }

  ~PhasarConfig() = default;
  PhasarConfig(const PhasarConfig &) = delete;
  PhasarConfig(PhasarConfig &&) = delete;

private:
  PhasarConfig();

  static std::string readConfigFile(const std::string &Path);
  void loadGlibcSpecialFunctionNames();
  void loadLLVMSpecialFunctionNames();

  std::set<std::string> SpecialFunctionNames;

  /// Specifies the directory in which important configuration files are
  /// located.
  inline static const std::string ConfigurationDirectory = []() {
    char *EnvHome = std::getenv("HOME");
    std::string ConfigFolder = "config/";
    if (EnvHome) { // Check if HOME was defined in the environment
      std::string PhasarConfig = std::string(EnvHome) + "/.config/phasar/";
      if (boost::filesystem::exists(PhasarConfig) &&
          boost::filesystem::is_directory(PhasarConfig)) {
        ConfigFolder = PhasarConfig;
      }
    }
    return ConfigFolder;
  }();

  /// Specifies the directory in which Phasar is located.
  static const std::string PhasarDir;

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
