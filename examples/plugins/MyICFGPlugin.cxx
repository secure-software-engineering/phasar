#include "MyICFGPlugin.h"

using namespace std;
using namespace psr;

__attribute__((constructor)) void init() {
  cout << "init - MyICFGPlugin\n";
  ICFGPluginFactory["icfg_testplugin"] = &makeMyICFGPlugin;
}

__attribute__((destructor)) void fini() { cout << "fini - MyICFGPlugin\n"; }

unique_ptr<ICFGPlugin> makeMyICFGPlugin(ProjectIRDB &IRDB,
                                        const vector<string> EntryPoints) {
  return unique_ptr<ICFGPlugin>(new MyICFGPlugin(IRDB, EntryPoints));
}

MyICFGPlugin::MyICFGPlugin(ProjectIRDB &IRDB, const vector<string> EntryPoints)
    : ICFGPlugin(IRDB, EntryPoints) {}

bool MyICFGPlugin::isCallStmt(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

const llvm::Function *MyICFGPlugin::getMethodOf(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return nullptr;
}

vector<const llvm::Instruction *> MyICFGPlugin::getPredsOf(
    const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

vector<const llvm::Instruction *> MyICFGPlugin::getSuccsOf(
    const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
MyICFGPlugin::getAllControlFlowEdges(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

vector<const llvm::Instruction *> MyICFGPlugin::getAllInstructionsOf(
    const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

bool MyICFGPlugin::isExitStmt(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool MyICFGPlugin::isStartPoint(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool MyICFGPlugin::isFallThroughSuccessor(const llvm::Instruction *stmt,
                                          const llvm::Instruction *succ) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool MyICFGPlugin::isBranchTarget(const llvm::Instruction *stmt,
                                  const llvm::Instruction *succ) {
  throw logic_error("Not implemented yet!");
  return false;
}

string MyICFGPlugin::getMethodName(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return "";
}

const llvm::Function *MyICFGPlugin::getMethod(const string &fun) {
  throw logic_error("Not implemented yet!");
  return nullptr;
}

set<const llvm::Instruction *> MyICFGPlugin::allNonCallStartNodes() {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Function *> MyICFGPlugin::getCalleesOfCallAt(
    const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *> MyICFGPlugin::getCallersOf(
    const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *> MyICFGPlugin::getCallsFromWithin(
    const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *> MyICFGPlugin::getStartPointsOf(
    const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *> MyICFGPlugin::getExitPointsOf(
    const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *> MyICFGPlugin::getReturnSitesOfCallAt(
    const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

string MyICFGPlugin::getStatementId(const llvm::Instruction *stmt) {
  return llvm::cast<llvm::MDString>(
             stmt->getMetadata(MetaDataKind)->getOperand(0))
      ->getString()
      .str();
}

json MyICFGPlugin::getAsJson() {
  json j;
  return j;
}
