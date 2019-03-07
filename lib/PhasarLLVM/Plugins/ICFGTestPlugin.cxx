/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>
#include <utility>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

#include <phasar/DB/ProjectIRDB.h>

#include "ICFGTestPlugin.h"

using namespace std;
using namespace psr;

namespace psr {

__attribute__((constructor)) void init() {
  cout << "init - ICFGTestPlugin\n";
  ICFGPluginFactory["icfg_testplugin"] = &makeICFGTestPlugin;
}

__attribute__((destructor)) void fini() { cout << "fini - ICFGTestPlugin\n"; }

unique_ptr<ICFGPlugin> makeICFGTestPlugin(ProjectIRDB &IRDB,
                                          const vector<string> EntryPoints) {
  return unique_ptr<ICFGPlugin>(new ICFGTestPlugin(IRDB, EntryPoints));
}

ICFGTestPlugin::ICFGTestPlugin(ProjectIRDB &IRDB,
                               const vector<string> EntryPoints)
    : ICFGPlugin(IRDB, EntryPoints) {}

bool ICFGTestPlugin::isCallStmt(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

const llvm::Function *
ICFGTestPlugin::getMethodOf(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return nullptr;
}

vector<const llvm::Instruction *>
ICFGTestPlugin::getPredsOf(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

vector<const llvm::Instruction *>
ICFGTestPlugin::getSuccsOf(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
ICFGTestPlugin::getAllControlFlowEdges(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

vector<const llvm::Instruction *>
ICFGTestPlugin::getAllInstructionsOf(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

bool ICFGTestPlugin::isExitStmt(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool ICFGTestPlugin::isStartPoint(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool ICFGTestPlugin::isFieldLoad(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool ICFGTestPlugin::isFieldStore(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool ICFGTestPlugin::isFallThroughSuccessor(const llvm::Instruction *stmt,
                                            const llvm::Instruction *succ) {
  throw logic_error("Not implemented yet!");
  return false;
}

bool ICFGTestPlugin::isBranchTarget(const llvm::Instruction *stmt,
                                    const llvm::Instruction *succ) {
  throw logic_error("Not implemented yet!");
  return false;
}

string ICFGTestPlugin::getMethodName(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return "";
}

const llvm::Function *ICFGTestPlugin::getMethod(const string &fun) {
  throw logic_error("Not implemented yet!");
  return nullptr;
}

set<const llvm::Instruction *> ICFGTestPlugin::allNonCallStartNodes() {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Function *>
ICFGTestPlugin::getCalleesOfCallAt(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *>
ICFGTestPlugin::getCallersOf(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *>
ICFGTestPlugin::getCallsFromWithin(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *>
ICFGTestPlugin::getStartPointsOf(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *>
ICFGTestPlugin::getExitPointsOf(const llvm::Function *fun) {
  throw logic_error("Not implemented yet!");
  return {};
}

set<const llvm::Instruction *>
ICFGTestPlugin::getReturnSitesOfCallAt(const llvm::Instruction *stmt) {
  throw logic_error("Not implemented yet!");
  return {};
}

string ICFGTestPlugin::getStatementId(const llvm::Instruction *stmt) {
  return llvm::cast<llvm::MDString>(
             stmt->getMetadata(MetaDataKind)->getOperand(0))
      ->getString()
      .str();
}

json ICFGTestPlugin::getAsJson() { return json{}; }

} // namespace psr
