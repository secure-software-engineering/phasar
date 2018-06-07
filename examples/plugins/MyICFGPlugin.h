#ifndef MYICFGPLUGIN_H_
#define MYICFGPLUGIN_H_

#include <phasar/PhasarLLVM/Plugins/Interfaces/ControlFlow/ICFGPlugin.h>
#include <iostream>
#include <json.hpp>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;

class MyICFGPlugin : public psr::ICFGPlugin {
 public:
  MyICFGPlugin(psr::ProjectIRDB &IRDB,
               const std::vector<std::string> EntryPoints);

  bool isCallStmt(const llvm::Instruction *stmt) override;

  const llvm::Function *getMethodOf(const llvm::Instruction *stmt) override;

  std::vector<const llvm::Instruction *> getPredsOf(
      const llvm::Instruction *stmt) override;

  std::vector<const llvm::Instruction *> getSuccsOf(
      const llvm::Instruction *stmt) override;

  std::vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) override;

  std::vector<const llvm::Instruction *> getAllInstructionsOf(
      const llvm::Function *fun) override;

  bool isExitStmt(const llvm::Instruction *stmt) override;

  bool isStartPoint(const llvm::Instruction *stmt) override;

  bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) override;

  bool isBranchTarget(const llvm::Instruction *stmt,
                      const llvm::Instruction *succ) override;

  std::string getMethodName(const llvm::Function *fun) override;

  const llvm::Function *getMethod(const string &fun) override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() override;

  std::set<const llvm::Function *> getCalleesOfCallAt(
      const llvm::Instruction *stmt) override;

  std::set<const llvm::Instruction *> getCallersOf(
      const llvm::Function *fun) override;

  std::set<const llvm::Instruction *> getCallsFromWithin(
      const llvm::Function *fun) override;

  std::set<const llvm::Instruction *> getStartPointsOf(
      const llvm::Function *fun) override;

  std::set<const llvm::Instruction *> getExitPointsOf(
      const llvm::Function *fun) override;

  std::set<const llvm::Instruction *> getReturnSitesOfCallAt(
      const llvm::Instruction *stmt) override;

  std::string getStatementId(const llvm::Instruction *) override;

  json getAsJson() override;
};

extern "C" std::unique_ptr<psr::ICFGPlugin> makeMyICFGPlugin(
    psr::ProjectIRDB &IRDB, const std::vector<std::string> EntryPoints);

#endif
