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

#include <filesystem>
#include <string>

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
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string PhasarVersion() { return XSTR(PHASAR_VERSION); }

  /// Stores the label/ tag with which we annotate the LLVM IR.
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string MetaDataKind() { return "psr.id"; }

  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string ConfigurationDirectory() { return ConfigDir; }

  /// Specifies the directory in which Phasar is located.
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string PhasarDirectory() { return PhasarDir; }

  /// Name of the file storing all standard header search paths used for
  /// compilation.
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string HeaderSearchPathsFileName() {
    return "standard_header_paths.conf";
  }

  /// Name of the compile_commands.json file (in case we wish to rename)
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string CompileCommandsJson() { return "compile_commands.json"; }

  /// Default Source- and Sink-Functions path
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string DefaultSourceSinkFunctionsPath() {
    return std::string(PhasarDirectory() +
                       "config/phasar-source-sink-function.json");
  }

  // Variables to be used in JSON export format
  /// Identifier for call graph export
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string JsonCallGraphID() { return "psr.cg"; }

  /// Identifier for type hierarchy graph export
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string JsonTypeHierarchyID() { return "psr.th"; }

  /// Identifier for points-to graph export
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string JsonPointsToGraphID() { return "psr.pt"; }

  /// Identifier for data-flow results export
  // NOLINTNEXTLINE(readability-identifier-naming)
  static std::string JsonDataFlowID() { return "psr.df"; }

  static PhasarConfig &getPhasarConfig();

  llvm::iterator_range<std::set<std::string>::iterator> specialFunctionNames() {
    return llvm::make_range(SpecialFuncNames.begin(), SpecialFuncNames.end());
  }

  /// Add a function name to the special functions list.
  /// Special functions are functions that cannot directly be analyzed but need
  /// to be handled by the analysis.
  ///
  /// Remark: Manually added special functions need to be added before creating
  /// the analysis.
  void addSpecialFunctionName(std::string SFName) {
    SpecialFuncNames.insert(std::move(SFName));
  }

  /// Variables map of the parsed command-line parameters
  // NOLINTNEXTLINE(readability-identifier-naming)
  static boost::program_options::variables_map &VariablesMap() {
    static boost::program_options::variables_map VMap;
    return VMap;
  }

  ~PhasarConfig() = default;
  PhasarConfig(const PhasarConfig &) = delete;
  PhasarConfig operator=(const PhasarConfig &) = delete;
  PhasarConfig(PhasarConfig &&) = delete;
  PhasarConfig operator=(PhasarConfig &&) = delete;

private:
  PhasarConfig();

  static std::string readConfigFile(const std::string &Path);
  void loadGlibcSpecialFunctionNames();
  void loadLLVMSpecialFunctionNames();

  std::set<std::string> SpecialFuncNames;

  /// Specifies the directory in which important configuration files are
  /// located.
  inline static const std::string ConfigDir = []() {
    auto *EnvHome = std::getenv("HOME");
    std::string ConfigFolder = "config/";
    if (EnvHome) { // Check if HOME was defined in the environment
      std::string PhasarConfDir = std::string(EnvHome) + "/.config/phasar/";
      if (std::filesystem::exists(PhasarConfDir) &&
          std::filesystem::is_directory(PhasarConfDir)) {
        ConfigFolder = PhasarConfDir;
      }
    }
    return ConfigFolder;
  }();

  /// Specifies the directory in which Phasar is located.
  static const std::string PhasarDir;

  /// Name of the file storing all glibc function names.
  static inline const std::string GLIBCFunctionListFileName =
      "glibc_function_list_v1-04.05.17.conf";

  /// Name of the file storing all LLVM intrinsic function names.
  static inline const std::string LLVMIntrinsicFunctionListFileName =
      "llvm_intrinsics_function_list_v1-04.05.17.conf";

  /// Log file directory
  static inline const std::string LogFileDirectory = "log/";
};

} // namespace psr

#endif
