/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_ANALYSISSTRATEGY_HELPERANALYSES_H_
#define PHASAR_PHASARLLVM_ANALYSISSTRATEGY_HELPERANALYSES_H_

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/HelperAnalysisConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "nlohmann/json.hpp"

#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace psr {
class LLVMProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMBasedCFG;
class LLVMAliasSet;

class HelperAnalyses { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  explicit HelperAnalyses(std::string IRFile,
                          std::optional<nlohmann::json> PrecomputedPTS,
                          AliasAnalysisType PTATy, bool AllowLazyPTS,
                          std::vector<std::string> EntryPoints,
                          CallGraphAnalysisType CGTy, Soundness SoundnessLevel,
                          bool AutoGlobalSupport) noexcept;

  explicit HelperAnalyses(std::string IRFile,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {}) noexcept;
  explicit HelperAnalyses(const llvm::Twine &IRFile,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  explicit HelperAnalyses(const char *IRFile,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  ~HelperAnalyses() noexcept;

  [[nodiscard]] LLVMProjectIRDB &getProjectIRDB();
  [[nodiscard]] LLVMAliasSet &getAliasInfo();
  [[nodiscard]] LLVMTypeHierarchy &getTypeHierarchy();
  [[nodiscard]] DIBasedTypeHierarchy &getNewTypeHierarchy();
  [[nodiscard]] LLVMBasedICFG &getICFG();
  [[nodiscard]] LLVMBasedCFG &getCFG();

private:
  std::unique_ptr<LLVMProjectIRDB> IRDB;
  std::unique_ptr<LLVMAliasSet> PT;
  std::unique_ptr<LLVMTypeHierarchy> TH;
  std::unique_ptr<DIBasedTypeHierarchy> DiTh;
  std::unique_ptr<LLVMBasedICFG> ICF;
  std::unique_ptr<LLVMBasedCFG> CFG;

  // IRDB
  std::string IRFile;

  // PTS
  std::optional<nlohmann::json> PrecomputedPTS;
  AliasAnalysisType PTATy{};
  bool AllowLazyPTS{};

  // ICF
  std::vector<std::string> EntryPoints;
  CallGraphAnalysisType CGTy{};
  Soundness SoundnessLevel{};
  bool AutoGlobalSupport{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_ANALYSISSTRATEGY_HELPERANALYSES_H_
