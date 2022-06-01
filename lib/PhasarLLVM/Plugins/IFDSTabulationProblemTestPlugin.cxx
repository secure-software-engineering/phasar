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

#include <utility>

#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"

#include "IFDSTabulationProblemTestPlugin.h"

using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin> makeIFDSTabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSTabulationProblemTestPlugin(IRDB, TH, ICF, PT,
                                          std::move(EntryPoints)));
}

__attribute__((constructor)) void init() {
  llvm::outs() << "init - IFDSTabulationProblemTestPlugin\n";
  IFDSTabulationProblemPluginFactory["ifds_testplugin"] =
      &makeIFDSTabulationProblemTestPlugin;
}

__attribute__((destructor)) void fini() {
  llvm::outs() << "fini - IFDSTabulationProblemTestPlugin\n";
}

IFDSTabulationProblemTestPlugin::IFDSTabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblemPlugin(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  ZeroValue = FFManager.getOrCreateZero();
}

const FlowFact *IFDSTabulationProblemTestPlugin::createZeroValue() const {
  return ZeroValue;
}

IFDSTabulationProblemTestPlugin::FlowFunctionPtrType
IFDSTabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction * /*Curr*/, const llvm::Instruction * /*Succ*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSTabulationProblemTestPlugin::FlowFunctionPtrType
IFDSTabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*DestFun*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSTabulationProblemTestPlugin::FlowFunctionPtrType
IFDSTabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*CalleeFun*/,
    const llvm::Instruction * /*ExitSite*/,
    const llvm::Instruction * /*RetSite*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSTabulationProblemTestPlugin::FlowFunctionPtrType
IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Instruction * /*RetSite*/,
    set<const llvm::Function *> /*Callees*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSTabulationProblemTestPlugin::FlowFunctionPtrType
IFDSTabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSTabulationProblemTestPlugin::n_t,
             IFDSTabulationProblemTestPlugin::d_t,
             IFDSTabulationProblemTestPlugin::l_t>
IFDSTabulationProblemTestPlugin::initialSeeds() {
  llvm::outs() << "IFDSTabulationProblemTestPlugin::initialSeeds()\n";
  InitialSeeds<IFDSTabulationProblemTestPlugin::n_t,
               IFDSTabulationProblemTestPlugin::d_t,
               IFDSTabulationProblemTestPlugin::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue());
  }
  return Seeds;
}

} // namespace psr
