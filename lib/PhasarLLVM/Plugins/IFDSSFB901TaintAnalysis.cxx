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

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h>

#include "IFDSSFB901TaintAnalysis.h"

using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSSFB901TaintAnalysis(const ProjectIRDB *IRDB,
                            const LLVMTypeHierarchy *TH,
                            const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                            std::set<std::string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSSFB901TaintAnalysis(IRDB, TH, ICF, PT, std::move(EntryPoints)));
}

__attribute__((constructor)) void init() {
  llvm::outs() << "init - IFDSSFB901TaintAnalysis\n";
  IFDSTabulationProblemPluginFactory["ifds_testplugin"] =
      &makeIFDSSFB901TaintAnalysis;
}

__attribute__((destructor)) void fini() {
  llvm::outs() << "fini - IFDSSFB901TaintAnalysis\n";
}

IFDSSFB901TaintAnalysis::IFDSSFB901TaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblemPlugin(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  ZeroValue = IFDSSFB901TaintAnalysis::createZeroValue();
}

const FlowFact *IFDSSFB901TaintAnalysis::createZeroValue() const {
  // static auto zero =
  //     std::make_unique<ValueFlowFactWrapper>(LLVMZeroValue::getInstance());
  // return zero.get();
  static auto *Zero = new ValueFlowFactWrapper(nullptr);
  return Zero;
}

IFDSSFB901TaintAnalysis::FlowFunctionPtrType
IFDSSFB901TaintAnalysis::getNormalFlowFunction(
    const llvm::Instruction * /*Curr*/, const llvm::Instruction * /*Succ*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSSFB901TaintAnalysis::FlowFunctionPtrType
IFDSSFB901TaintAnalysis::getCallFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*DestFun*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSSFB901TaintAnalysis::FlowFunctionPtrType
IFDSSFB901TaintAnalysis::getRetFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*CalleeFun*/,
    const llvm::Instruction * /*ExitSite*/,
    const llvm::Instruction * /*RetSite*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSSFB901TaintAnalysis::FlowFunctionPtrType
IFDSSFB901TaintAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Instruction * /*RetSite*/,
    set<const llvm::Function *> /*Callees*/) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSSFB901TaintAnalysis::FlowFunctionPtrType
IFDSSFB901TaintAnalysis::getSummaryFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSSFB901TaintAnalysis::n_t, IFDSSFB901TaintAnalysis::d_t,
             IFDSSFB901TaintAnalysis::l_t>
IFDSSFB901TaintAnalysis::initialSeeds() {
  llvm::outs() << "IFDSSFB901TaintAnalysis::initialSeeds()\n";
  InitialSeeds<IFDSSFB901TaintAnalysis::n_t, IFDSSFB901TaintAnalysis::d_t,
               IFDSSFB901TaintAnalysis::l_t>
      Seeds;
  Seeds.addSeed(&ICF->getFunction("run_service_contrast_cpu")->front().front(),
                getZeroValue());
  return Seeds;
}

} // namespace psr
