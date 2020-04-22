/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PluginTest.cpp
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#include <iostream>
#include <utility>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>

#include "IFDSTabulationProblemTestPlugin.h"
using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin> makeIFDSTabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSTabulationProblemTestPlugin(IRDB, TH, ICF, PT,
                                          std::move(EntryPoints)));
}

__attribute__((constructor)) void init() {
  cout << "init - IFDSTabulationProblemTestPlugin\n";
  IFDSTabulationProblemPluginFactory["ifds_testplugin"] =
      &makeIFDSTabulationProblemTestPlugin;
}

__attribute__((destructor)) void fini() {
  cout << "fini - IFDSTabulationProblemTestPlugin\n";
}

IFDSTabulationProblemTestPlugin::IFDSTabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblemPlugin(IRDB, TH, ICF, PT, std::move(EntryPoints)) {}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction *Curr, const llvm::Instruction *Succ) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction *CallStmt, const llvm::Function *DestFun) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Function *CalleeFun,
    const llvm::Instruction *ExitStmt, const llvm::Instruction *RetSite) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Instruction *RetSite,
    set<const llvm::Function *> Callees) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction *CallStmt, const llvm::Function *DestFun) {
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::initialSeeds() {
  cout << "IFDSTabulationProblemTestPlugin::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                       set<const llvm::Value *>({getZeroValue()})));
  }
  return SeedMap;
}

} // namespace psr
