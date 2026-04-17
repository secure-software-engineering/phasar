/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_HELPERANALYSES_H
#define PHASAR_PHASARLLVM_HELPERANALYSES_H

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/ControlFlow/CallGraphData.h"
#include "phasar/PhasarLLVM/HelperAnalysisConfig.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSetData.h"
#include "phasar/Pointer/UnionFindAliasAnalysisType.h"
#include "phasar/Utils/FunctionId.h"

#include "llvm/ADT/Twine.h"

#include <memory>
#include <optional>
#include <vector>

namespace llvm {
class Module;
class GlobalVariable;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;
class DIBasedTypeHierarchy;
class LLVMBasedICFG;
class LLVMBasedCFG;
template <typename GraphNodeId> struct SCCHolder;
template <typename GraphNodeId> struct SCCDependencyGraph;
template <typename G> struct UsedGlobalsHolder;

class HelperAnalyses { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  explicit HelperAnalyses(std::string IRFile,
                          std::optional<LLVMAliasSetData> PrecomputedPTS,
                          AliasAnalysisType PTATy, bool AllowLazyPTS,
                          std::vector<std::string> EntryPoints,
                          std::optional<CallGraphData> PrecomputedCG,
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
  explicit HelperAnalyses(llvm::Module *IRModule,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  explicit HelperAnalyses(std::unique_ptr<llvm::Module> IRModule,
                          std::vector<std::string> EntryPoints,
                          HelperAnalysisConfig Config = {});
  ~HelperAnalyses() noexcept;

  [[nodiscard]] LLVMProjectIRDB &getProjectIRDB();
  [[nodiscard]] LLVMAliasInfoRef getAliasInfo();
  [[nodiscard]] DIBasedTypeHierarchy &getTypeHierarchy();
  [[nodiscard]] LLVMBasedICFG &getICFG();
  [[nodiscard]] LLVMBasedCFG &getCFG();
  [[nodiscard]] FunctionCompressor<const llvm::Function *> &
  getCompressedFunctions();
  [[nodiscard]] const SCCHolder<FunctionId> &getCGSCCs();
  [[nodiscard]] const SCCDependencyGraph<FunctionId> &getCGSCCCallers();
  [[nodiscard]] const UsedGlobalsHolder<const llvm::GlobalVariable *> &
  getUsedGlobals();

private:
  std::unique_ptr<LLVMProjectIRDB> IRDB;
  LLVMAliasInfo PT{};
  std::unique_ptr<DIBasedTypeHierarchy> TH;
  std::unique_ptr<LLVMBasedICFG> ICF;
  std::unique_ptr<LLVMBasedCFG> CFG;
  std::unique_ptr<FunctionCompressor<const llvm::Function *>> FC;
  std::unique_ptr<SCCHolder<FunctionId>> SCCs;
  std::unique_ptr<SCCDependencyGraph<FunctionId>> SCCCallers;
  std::unique_ptr<UsedGlobalsHolder<const llvm::GlobalVariable *>> UsedGlobals;

  // IRDB
  std::string IRFile;

  // PTS
  std::optional<LLVMAliasSetData> PrecomputedPTS;
  AliasAnalysisType PTATy{};
  UnionFindAliasAnalysisType UFAATy{};
  bool AllowLazyPTS{};

  // ICF
  std::optional<CallGraphData> PrecomputedCG;
  std::vector<std::string> EntryPoints;
  CallGraphAnalysisType CGTy{};
  Soundness SoundnessLevel{};
  bool AutoGlobalSupport{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_HELPERANALYSES_H
