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

#include "phasar/PhasarLLVM/AnalysisStrategy/HelperAnalysisConfig.h"

#include "nlohmann/json.hpp"

#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace psr {
class LLVMProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMAliasSet;

class HelperAnalyses { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  explicit HelperAnalyses(std::string IRFile,
                          std::optional<nlohmann::json> PrecomputedPTS,
                          AliasAnalysisType PTATy, bool AllowLazyPTS,
                          std::vector<std::string> EntryPoints,
                          CallGraphAnalysisType CGTy, Soundness SoundnessLevel,
                          bool AutoGlobalSupport);

  explicit HelperAnalyses(std::string IRFile,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  ~HelperAnalyses();

  LLVMProjectIRDB &getProjectIRDB();
  LLVMAliasSet &getAliasInfo();
  LLVMTypeHierarchy &getTypeHierarchy();
  LLVMBasedICFG &getICFG();

private:
  std::unique_ptr<LLVMProjectIRDB> IRDB;
  std::unique_ptr<LLVMAliasSet> PT;
  std::unique_ptr<LLVMTypeHierarchy> TH;
  std::unique_ptr<LLVMBasedICFG> ICF;

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
