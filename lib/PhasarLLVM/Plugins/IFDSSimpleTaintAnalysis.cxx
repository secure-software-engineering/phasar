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

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"

#include "IFDSSimpleTaintAnalysis.h"

using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSSimpleTaintAnalysis(const ProjectIRDB *IRDB,
                            const LLVMTypeHierarchy *TH,
                            const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                            std::set<std::string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSSimpleTaintAnalysis(IRDB, TH, ICF, PT, std::move(EntryPoints)));
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
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblemPlugin(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  ZeroValue = ffManager.getOrCreateZero();
}

const FlowFact *IFDSSimpleTaintAnalysis::createZeroValue() const {
  // static auto zero =
  //     std::make_unique<ValueFlowFactWrapper>(LLVMZeroValue::getInstance());
  // return zero.get();
  return ZeroValue;
}

IFDSSimpleTaintAnalysis::FlowFunctionPtrType
IFDSSimpleTaintAnalysis::getNormalFlowFunction(const llvm::Instruction *Curr,
                                               const llvm::Instruction *Succ) {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return make_shared<LambdaFlow<const FlowFact *>>(
        [this, Store](const FlowFact *source) -> set<const FlowFact *> {
          // auto VFW = static_cast<const ValueFlowFactWrapper *>(source);
          if (Store->getValueOperand() ==
              source->as<ValueFlowFactWrapper>()->get()) {
            return ffManager.getOrCreateFlowFacts(Store->getValueOperand(),
                                                  Store->getPointerOperand());
          } else {
            return ffManager.getOrCreateFlowFacts(Store->getValueOperand());
          }
        });
  }
  return Identity<const FlowFact *>::getInstance();
}

IFDSSimpleTaintAnalysis::FlowFunctionPtrType
IFDSSimpleTaintAnalysis::getCallFlowFunction(const llvm::Instruction *CallStmt,
                                             const llvm::Function *DestFun) {
  if (const auto *Call = llvm::dyn_cast<llvm::CallInst>(CallStmt)) {
    if (DestFun->getName().str() == "taint") {
      return make_shared<Gen<const FlowFact *>>(
          ffManager.getOrCreateFlowFact(Call), getZeroValue());
    } else if (DestFun->getName().str() == "leak") {
    } else {
    }
  }
  return Identity<const FlowFact *>::getInstance();
}

IFDSSimpleTaintAnalysis::FlowFunctionPtrType
IFDSSimpleTaintAnalysis::getRetFlowFunction(const llvm::Instruction *CallSite,
                                            const llvm::Function *CalleeFun,
                                            const llvm::Instruction *ExitStmt,
                                            const llvm::Instruction *RetSite) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSSimpleTaintAnalysis::FlowFunctionPtrType
IFDSSimpleTaintAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Instruction *RetSite,
    set<const llvm::Function *> Callees) {
  return Identity<const FlowFact *>::getInstance();
}

IFDSSimpleTaintAnalysis::FlowFunctionPtrType
IFDSSimpleTaintAnalysis::getSummaryFlowFunction(
    const llvm::Instruction *CallStmt, const llvm::Function *DestFun) {
  return nullptr;
}

map<const llvm::Instruction *, set<const FlowFact *>>
IFDSSimpleTaintAnalysis::initialSeeds() {
  cout << "IFDSSimpleTaintAnalysis::initialSeeds()\n";
  map<const llvm::Instruction *, set<const FlowFact *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                       set<const FlowFact *>({getZeroValue()})));
  }
  return SeedMap;
}

} // namespace psr
