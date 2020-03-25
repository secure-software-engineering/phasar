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

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"

#include "IFDSSimpleTaintAnalysis.h"

using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin> makeIFDSSimpleTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSSimpleTaintAnalysis(IRDB, TH, ICF, PT, EntryPoints));
}

__attribute__((constructor)) void init() {
  cout << "init - IFDSSimpleTaintAnalysis\n";
  IFDSTabulationProblemPluginFactory["ifds_testplugin"] =
      &makeIFDSSimpleTaintAnalysis;
}

__attribute__((destructor)) void fini() {
  cout << "fini - IFDSSimpleTaintAnalysis\n";
}

IFDSSimpleTaintAnalysis::IFDSSimpleTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblemPlugin(IRDB, TH, ICF, PT, EntryPoints) {}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSimpleTaintAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                               const llvm::Instruction *succ) {
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    struct STA : FlowFunction<const llvm::Value *> {
      const llvm::StoreInst *Store;
      STA(const llvm::StoreInst *S) : Store(S) {}
      set<const llvm::Value *> computeTargets(const llvm::Value *src) {
        if (Store->getValueOperand() == src) {
          return {Store->getValueOperand(), Store->getPointerOperand()};
        } else {
          return {Store->getValueOperand()};
        }
      }
    };
    return make_shared<STA>(Store);
  }
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSimpleTaintAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                             const llvm::Function *destFun) {
  if (auto Call = llvm::dyn_cast<llvm::CallInst>(callStmt)) {
    if (destFun->getName().str() == "taint") {
      return make_shared<Gen<const llvm::Value *>>(Call, getZeroValue());
    } else if (destFun->getName().str() == "leak") {
    } else {
    }
  }
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSimpleTaintAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Function *calleeFun,
                                            const llvm::Instruction *exitStmt,
                                            const llvm::Instruction *retSite) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSimpleTaintAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    set<const llvm::Function *> callees) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSimpleTaintAnalysis::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destFun) {
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSSimpleTaintAnalysis::initialSeeds() {
  cout << "IFDSSimpleTaintAnalysis::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                       set<const llvm::Value *>({getZeroValue()})));
  }
  return SeedMap;
}

} // namespace psr
