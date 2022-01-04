/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef ICFGTESTPLUGIN_H_
#define ICFGTESTPLUGIN_H_

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <phasar/PhasarLLVM/Plugins/Interfaces/ControlFlow/ICFGPlugin.h>

namespace llvm {
class Instruction;
class Function;
} // namespace llvm

namespace psr {

using json = nlohmann::json;

class ICFGTestPlugin : public ICFGPlugin {
public:
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;

  ICFGTestPlugin(ProjectIRDB &IRDB,
                 const std::vector<std::string> &EntryPoints);

  ~ICFGTestPlugin() override = default;

  // CFG parts

  f_t getFunctionOf(n_t Inst) const override;

  std::vector<n_t> getPredsOf(n_t Inst) const override;

  std::vector<n_t> getSuccsOf(n_t Inst) const override;

  std::vector<std::pair<n_t, n_t>>
  getAllControlFlowEdges(f_t Fun) const override;

  std::vector<n_t> getAllInstructionsOf(f_t Fun) const override;

  std::set<n_t> getStartPointsOf(f_t Fun) const override;

  std::set<n_t> getExitPointsOf(f_t Fun) const override;

  bool isCallSite(n_t Inst) const override;

  bool isExitInst(n_t Inst) const override;

  bool isStartPoint(n_t Inst) const override;

  bool isFieldLoad(n_t Inst) const override;

  bool isFieldStore(n_t Inst) const override;

  bool isFallThroughSuccessor(n_t Inst, n_t Succ) const override;

  bool isBranchTarget(n_t Inst, n_t Succ) const override;

  bool isHeapAllocatingFunction(f_t Fun) const override;

  bool isSpecialMemberFunction(f_t Fun) const override;

  SpecialMemberFunctionType
  getSpecialMemberFunctionType(f_t Fun) const override;

  std::string getStatementId(n_t Inst) const override;

  std::string getFunctionName(f_t Fun) const override;

  std::string getDemangledFunctionName(f_t Fun) const override;

  void print(f_t F, std::ostream &OS = std::cout) const override;

  nlohmann::json getAsJson(f_t F) const override;

  // ICFG parts

  [[nodiscard]] std::set<f_t> getAllFunctions() const override;

  [[nodiscard]] f_t getFunction(const std::string &Fun) const override;

  [[nodiscard]] bool isIndirectFunctionCall(n_t Inst) const override;

  [[nodiscard]] bool isVirtualFunctionCall(n_t Inst) const override;

  [[nodiscard]] std::set<n_t> allNonCallStartNodes() const override;

  [[nodiscard]] std::set<f_t> getCalleesOfCallAt(n_t Inst) const override;

  [[nodiscard]] std::set<n_t> getCallersOf(f_t Fun) const override;

  [[nodiscard]] std::set<n_t> getCallsFromWithin(f_t Fun) const override;

  [[nodiscard]] std::set<n_t> getReturnSitesOfCallAt(n_t Inst) const override;

  void print(std::ostream &OS = std::cout) const override;

  [[nodiscard]] nlohmann::json getAsJson() const override;

protected:
  void collectGlobalCtors() override;

  void collectGlobalDtors() override;

  void collectGlobalInitializers() override;

  void collectRegisteredDtors() override;
};

extern "C" std::unique_ptr<ICFGPlugin>
makeICFGTestPlugin(ProjectIRDB &IRDB,
                   const std::vector<std::string> &EntryPoints);
} // namespace psr

#endif
