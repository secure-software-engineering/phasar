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
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"

#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace psr {
class LLVMProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMPointsToInfo;

class HelperAnalyses { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  explicit HelperAnalyses(std::string IRFile,
                          std::optional<nlohmann::json> PrecomputedPTS,
                          PointerAnalysisType PTATy, bool AllowLazyPTS,
                          std::vector<std::string> EntryPoints,
                          CallGraphAnalysisType CGTy, Soundness SoundnessLevel,
                          bool AutoGlobalSupport);

  explicit HelperAnalyses(std::string IRFile,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  explicit HelperAnalyses(const llvm::Twine &IRFile,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  explicit HelperAnalyses(const char *IRFile,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  ~HelperAnalyses();

  LLVMProjectIRDB &getProjectIRDB();
  LLVMPointsToInfo &getPointsToInfo();
  LLVMTypeHierarchy &getTypeHierarchy();
  LLVMBasedICFG &getICFG();

  void setCGTy(CallGraphAnalysisType CGTy) noexcept {
    assert(ICF == nullptr && "The ICFG has already been constructed. CGTy "
                             "change does not take effect");
    this->CGTy = CGTy;
  }

private:
  std::unique_ptr<LLVMProjectIRDB> IRDB;
  std::unique_ptr<LLVMPointsToInfo> PT;
  std::unique_ptr<LLVMTypeHierarchy> TH;
  std::unique_ptr<LLVMBasedICFG> ICF;

  // IRDB
  std::string IRFile;

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
