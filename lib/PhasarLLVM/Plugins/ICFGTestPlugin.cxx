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

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include "phasar/DB/ProjectIRDB.h"

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

ICFGTestPlugin::f_t
ICFGTestPlugin::getFunctionOf(ICFGTestPlugin::n_t stmt) const {
  return nullptr;
}

std::vector<ICFGTestPlugin::n_t>
ICFGTestPlugin::getPredsOf(ICFGTestPlugin::n_t stmt) const {
  return {};
}

std::vector<ICFGTestPlugin::n_t>
ICFGTestPlugin::getSuccsOf(ICFGTestPlugin::n_t stmt) const {
  return {};
}

std::vector<std::pair<ICFGTestPlugin::n_t, ICFGTestPlugin::n_t>>
ICFGTestPlugin::getAllControlFlowEdges(ICFGTestPlugin::f_t fun) const {
  return {};
}

std::vector<ICFGTestPlugin::n_t>
ICFGTestPlugin::getAllInstructionsOf(ICFGTestPlugin::f_t fun) const {
  return {};
}

bool ICFGTestPlugin::isExitStmt(ICFGTestPlugin::n_t stmt) const {
  return false;
}

bool ICFGTestPlugin::isStartPoint(ICFGTestPlugin::n_t stmt) const {
  return false;
}

bool ICFGTestPlugin::isFieldLoad(ICFGTestPlugin::n_t stmt) const {
  return false;
}

bool ICFGTestPlugin::isFieldStore(ICFGTestPlugin::n_t stmt) const {
  return false;
}

bool ICFGTestPlugin::isFallThroughSuccessor(ICFGTestPlugin::n_t stmt,
                                            ICFGTestPlugin::n_t succ) const {
  return false;
}

bool ICFGTestPlugin::isBranchTarget(ICFGTestPlugin::n_t stmt,
                                    ICFGTestPlugin::n_t succ) const {
  return false;
}

std::string ICFGTestPlugin::getStatementId(ICFGTestPlugin::n_t stmt) const {
  return "";
}

std::string ICFGTestPlugin::getFunctionName(ICFGTestPlugin::f_t fun) const {
  return "";
}

void ICFGTestPlugin::print(ICFGTestPlugin::f_t F, std::ostream &OS) const {}

nlohmann::json ICFGTestPlugin::getAsJson(ICFGTestPlugin::f_t F) const {
  return "";
}

// ICFG parts

std::set<ICFGTestPlugin::f_t> ICFGTestPlugin::getAllFunctions() const {
  return {};
}

ICFGTestPlugin::f_t ICFGTestPlugin::getFunction(const std::string &fun) const {
  return nullptr;
}

bool ICFGTestPlugin::isCallStmt(ICFGTestPlugin::n_t stmt) const {
  return false;
}

bool ICFGTestPlugin::isIndirectFunctionCall(ICFGTestPlugin::n_t stmt) const {
  return false;
}

bool ICFGTestPlugin::isVirtualFunctionCall(ICFGTestPlugin::n_t stmt) const {
  return false;
}

std::set<ICFGTestPlugin::n_t> ICFGTestPlugin::allNonCallStartNodes() const {
  return {};
}

std::set<ICFGTestPlugin::f_t>
ICFGTestPlugin::getCalleesOfCallAt(ICFGTestPlugin::n_t stmt) const {
  return {};
}

std::set<ICFGTestPlugin::n_t>
ICFGTestPlugin::getCallersOf(ICFGTestPlugin::f_t fun) const {
  return {};
}

std::set<ICFGTestPlugin::n_t>
ICFGTestPlugin::getCallsFromWithin(ICFGTestPlugin::f_t fun) const {
  return {};
}

std::set<ICFGTestPlugin::n_t>
ICFGTestPlugin::getStartPointsOf(ICFGTestPlugin::f_t fun) const {
  return {};
}

std::set<ICFGTestPlugin::n_t>
ICFGTestPlugin::getExitPointsOf(ICFGTestPlugin::f_t fun) const {
  return {};
}

std::set<ICFGTestPlugin::n_t>
ICFGTestPlugin::getReturnSitesOfCallAt(ICFGTestPlugin::n_t stmt) const {
  return {};
}

void ICFGTestPlugin::print(std::ostream &OS) const {}

nlohmann::json ICFGTestPlugin::getAsJson() const { return ""_json; }

} // namespace psr
