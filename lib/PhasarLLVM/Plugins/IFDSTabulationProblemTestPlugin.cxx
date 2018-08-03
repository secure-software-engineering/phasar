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

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>

#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>

#include "IFDSTabulationProblemTestPlugin.h"
using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSTabulationProblemTestPlugin(LLVMBasedICFG &I,
                                    vector<string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSTabulationProblemTestPlugin(I, EntryPoints));
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
    LLVMBasedICFG &I, vector<string> EntryPoints)
    : IFDSTabulationProblemPlugin(I, EntryPoints) {}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction *curr, const llvm::Instruction *succ) {
  cout << "IFDSTabulationProblemTestPlugin::getNormalFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  cout << "IFDSTabulationProblemTestPlugin::getCallFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  cout << "IFDSTabulationProblemTestPlugin::getRetFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    set<const llvm::Function *> callees) {
  cout << "IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  cout << "IFDSTabulationProblemTestPlugin::getSummaryFlowFunction()\n";
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::initialSeeds() {
  cout << "IFDSTabulationProblemTestPlugin::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

} // namespace psr
