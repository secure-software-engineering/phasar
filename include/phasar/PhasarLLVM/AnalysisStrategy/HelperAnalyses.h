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
class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMPointsToInfo;

class HelperAnalyses { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  explicit HelperAnalyses(std::vector<std::string> IRFiles,
                          std::optional<nlohmann::json> PrecomputedPTS,
                          PointerAnalysisType PTATy, bool AllowLazyPTS,
                          std::vector<std::string> EntryPoints,
                          CallGraphAnalysisType CGTy, Soundness SoundnessLevel,
                          bool AutoGlobalSupport);

  explicit HelperAnalyses(std::vector<std::string> IRFiles,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  ~HelperAnalyses();

  ProjectIRDB &getProjectIRDB();
  LLVMPointsToInfo &getPointsToInfo();
  LLVMTypeHierarchy &getTypeHierarchy();
  LLVMBasedICFG &getICFG();

private:
  std::unique_ptr<ProjectIRDB> IRDB;
  std::unique_ptr<LLVMPointsToInfo> PT;
  std::unique_ptr<LLVMTypeHierarchy> TH;
  std::unique_ptr<LLVMBasedICFG> ICF;

  // IRDB
  std::vector<std::string> IRFiles;

  // PTS
  std::optional<nlohmann::json> PrecomputedPTS;
  PointerAnalysisType PTATy{};
  bool AllowLazyPTS{};

  // ICF
  std::vector<std::string> EntryPoints;
  CallGraphAnalysisType CGTy{};
  Soundness SoundnessLevel{};
  bool AutoGlobalSupport{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_ANALYSISSTRATEGY_HELPERANALYSES_H_
