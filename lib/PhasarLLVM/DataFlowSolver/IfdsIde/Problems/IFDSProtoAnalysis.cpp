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
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSProtoAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;
using namespace std;

namespace psr {

IFDSProtoAnalysis::IFDSProtoAnalysis(const ProjectIRDB *IRDB,
                                     const LLVMTypeHierarchy *TH,
                                     const LLVMBasedICFG *ICF,
                                     LLVMPointsToInfo *PT,
                                     std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IFDSProtoAnalysis::ZeroValue = createZeroValue();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getNormalFlowFunction(IFDSProtoAnalysis::n_t Curr,
                                         IFDSProtoAnalysis::n_t Succ) {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return make_shared<Gen<IFDSProtoAnalysis::d_t>>(Store->getPointerOperand(),
                                                    getZeroValue());
  }
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getCallFlowFunction(IFDSProtoAnalysis::n_t CallStmt,
                                       IFDSProtoAnalysis::f_t DestFun) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType IFDSProtoAnalysis::getRetFlowFunction(
    IFDSProtoAnalysis::n_t CallSite, IFDSProtoAnalysis::f_t CalleeFun,
    IFDSProtoAnalysis::n_t ExitStmt, IFDSProtoAnalysis::n_t RetSite) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getCallToRetFlowFunction(
    IFDSProtoAnalysis::n_t CallSite, IFDSProtoAnalysis::n_t RetSite,
    set<IFDSProtoAnalysis::f_t> Callees) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getSummaryFlowFunction(IFDSProtoAnalysis::n_t CallStmt,
                                          IFDSProtoAnalysis::f_t DestFun) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

map<IFDSProtoAnalysis::n_t, set<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::initialSeeds() {
  cout << "IFDSProtoAnalysis::initialSeeds()\n";
  map<IFDSProtoAnalysis::n_t, set<IFDSProtoAnalysis::d_t>> SeedMap;
  for (const auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IFDSProtoAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSProtoAnalysis::d_t IFDSProtoAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSProtoAnalysis::isZeroValue(IFDSProtoAnalysis::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

void IFDSProtoAnalysis::printNode(ostream &OS, IFDSProtoAnalysis::n_t N) const {
  OS << llvmIRToString(N);
}

void IFDSProtoAnalysis::printDataFlowFact(ostream &OS,
                                          IFDSProtoAnalysis::d_t D) const {
  OS << llvmIRToString(D);
}

void IFDSProtoAnalysis::printFunction(ostream &OS,
                                      IFDSProtoAnalysis::f_t M) const {
  OS << M->getName().str();
}

} // namespace psr
