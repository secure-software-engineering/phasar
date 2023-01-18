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

#include "phasar/Config/Version.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"

#include <filesystem>
#include <set>
#include <string>

#define XSTR(S) STR(S)
#define STR(S) #S

namespace psr {

class PhasarConfig {
public:
  /// Current Phasar version
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] static constexpr llvm::StringRef PhasarVersion() noexcept {
    return XSTR(PHASAR_VERSION);
  }

  /// Stores the label/ tag with which we annotate the LLVM IR.
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] static constexpr llvm::StringRef MetaDataKind() noexcept {
    return "psr.id";
  }

  /// Specifies the directory in which important configuration files are
  /// located.
  [[nodiscard]] static constexpr llvm::StringRef
  // NOLINTNEXTLINE(readability-identifier-naming)
  ConfigurationDirectory() noexcept {
    return PHASAR_CONFIG_DIR;
  }

  /// Specifies the directory in which Phasar is located.
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] static constexpr llvm::StringRef PhasarDirectory() noexcept {
    return PHASAR_DIR;
  }

  /// Name of the file storing all standard header search paths used for
  /// compilation.
  [[nodiscard]] static constexpr llvm::StringRef
  // NOLINTNEXTLINE(readability-identifier-naming)
  HeaderSearchPathsFileName() noexcept {
    return "standard_header_paths.conf";
  }

  /// Name of the compile_commands.json file (in case we wish to rename)
  [[nodiscard]] static constexpr llvm::StringRef
  // NOLINTNEXTLINE(readability-identifier-naming)
  CompileCommandsJson() noexcept {
    return "compile_commands.json";
  }

  /// Default Source- and Sink-Functions path
  [[nodiscard]] static constexpr llvm::StringRef
  // NOLINTNEXTLINE(readability-identifier-naming)
  DefaultSourceSinkFunctionsPath() noexcept {
    return PHASAR_DIR "/config/phasar-source-sink-function.json";
  }

  // Variables to be used in JSON export format
  /// Identifier for call graph export
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] static constexpr llvm::StringRef JsonCallGraphID() noexcept {
    return "psr.cg";
  }

  /// Identifier for type hierarchy graph export
  [[nodiscard]] static constexpr llvm::StringRef
  // NOLINTNEXTLINE(readability-identifier-naming)
  JsonTypeHierarchyID() noexcept {
    return "psr.th";
  }

  /// Identifier for points-to graph export
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] static constexpr llvm::StringRef JsonAliasGraphID() noexcept {
    return "psr.pt";
  }

  /// Identifier for data-flow results export
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] static constexpr llvm::StringRef JsonDataFlowID() noexcept {
    return "psr.df";
  }

  [[nodiscard]] static PhasarConfig &getPhasarConfig();

  [[nodiscard]] llvm::iterator_range<std::set<std::string>::iterator>
  specialFunctionNames() const {
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

  ~PhasarConfig() = default;
  PhasarConfig(const PhasarConfig &) = delete;
  PhasarConfig operator=(const PhasarConfig &) = delete;
  PhasarConfig(PhasarConfig &&) = delete;
  PhasarConfig operator=(PhasarConfig &&) = delete;

private:
  PhasarConfig();

  void loadGlibcSpecialFunctionNames();
  void loadLLVMSpecialFunctionNames();

  std::set<std::string> SpecialFuncNames;

  /// Name of the file storing all glibc function names.
  static constexpr llvm::StringLiteral GLIBCFunctionListFileName =
      "glibc_function_list_v1-04.05.17.conf";

  /// Name of the file storing all LLVM intrinsic function names.
  static constexpr llvm::StringLiteral LLVMIntrinsicFunctionListFileName =
      "llvm_intrinsics_function_list_v1-04.05.17.conf";

  /// Log file directory
  static constexpr llvm::StringLiteral LogFileDirectory = "log/";
};

} // namespace psr

#endif
