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

#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <json.hpp>

#include <phasar/PhasarLLVM/Plugins/Interfaces/ControlFlow/ICFGPlugin.h>

namespace psr {

using json = nlohmann::json;

class ICFGTestPlugin : public ICFGPlugin {
public:
  ICFGTestPlugin(ProjectIRDB &IRDB, const std::vector<std::string> EntryPoints);

  bool isCallStmt(const llvm::Instruction *stmt) override;

  const llvm::Function *getMethodOf(const llvm::Instruction *stmt) override;

  std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *stmt) override;

  std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *stmt) override;

  std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) override;

  std::vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *fun) override;

  bool isExitStmt(const llvm::Instruction *stmt) override;

  bool isStartPoint(const llvm::Instruction *stmt) override;

  bool isFieldLoad(const llvm::Instruction *stmt) override;

  bool isFieldStore(const llvm::Instruction *stmt) override;

  bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) override;

  bool isBranchTarget(const llvm::Instruction *stmt,
                      const llvm::Instruction *succ) override;

  std::string getMethodName(const llvm::Function *fun) override;

  const llvm::Function *getMethod(const std::string &fun) override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() override;

  std::set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *stmt) override;

  std::set<const llvm::Instruction *>
  getCallersOf(const llvm::Function *fun) override;

  std::set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *fun) override;

  std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *fun) override;

  std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *fun) override;

  std::set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *stmt) override;

  std::string getStatementId(const llvm::Instruction *) override;

  json getAsJson() override;
};

extern "C" std::unique_ptr<ICFGPlugin>
makeICFGTestPlugin(ProjectIRDB &IRDB,
                   const std::vector<std::string> EntryPoints);
} // namespace psr

#endif
