#ifndef MYICFGPLUGIN_H_
#define MYICFGPLUGIN_H_

#include <iostream>
#include <json.hpp>
#include <phasar/PhasarLLVM/Plugins/Interfaces/ControlFlow/ICFGPlugin.h>
#include <stdexcept>

using namespace std;
using json = nlohmann::json;

class MyICFGPlugin : public ICFGPlugin {
public:
  MyICFGPlugin(ProjectIRDB &IRDB, const vector<string> EntryPoints);

  bool isCallStmt(const llvm::Instruction *stmt) override;

  const llvm::Function *getMethodOf(const llvm::Instruction *stmt) override;

  vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *stmt) override;

  vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *stmt) override;

  vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) override;

  vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *fun) override;

  bool isExitStmt(const llvm::Instruction *stmt) override;

  bool isStartPoint(const llvm::Instruction *stmt) override;

  bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) override;

  bool isBranchTarget(const llvm::Instruction *stmt,
                      const llvm::Instruction *succ) override;

  string getMethodName(const llvm::Function *fun) override;

  const llvm::Function *getMethod(const string &fun) override;

  set<const llvm::Instruction *> allNonCallStartNodes() override;

  set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *stmt) override;

  set<const llvm::Instruction *>
  getCallersOf(const llvm::Function *fun) override;

  set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *fun) override;

  set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *fun) override;

  set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *fun) override;

  set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *stmt) override;

  string getStatementId(const llvm::Instruction *) override;

  json getAsJson() override;
};

extern "C" unique_ptr<ICFGPlugin>
makeMyICFGPlugin(ProjectIRDB &IRDB, const vector<string> EntryPoints);

#endif
