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
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSignAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

IFDSSignAnalysis::IFDSSignAnalysis(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IFDSSignAnalysis::ZeroValue = createZeroValue();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getNormalFlowFunction(IFDSSignAnalysis::n_t Curr,
                                        IFDSSignAnalysis::n_t Succ) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getCallFlowFunction(IFDSSignAnalysis::n_t CallStmt,
                                      IFDSSignAnalysis::f_t DestFun) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType IFDSSignAnalysis::getRetFlowFunction(
    IFDSSignAnalysis::n_t CallSite, IFDSSignAnalysis::f_t CalleeFun,
    IFDSSignAnalysis::n_t ExitStmt, IFDSSignAnalysis::n_t RetSite) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getCallToRetFlowFunction(IFDSSignAnalysis::n_t CallSite,
                                           IFDSSignAnalysis::n_t RetSite,
                                           set<IFDSSignAnalysis::f_t> Callees) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getSummaryFlowFunction(IFDSSignAnalysis::n_t CallStmt,
                                         IFDSSignAnalysis::f_t DestFun) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

map<IFDSSignAnalysis::n_t, set<IFDSSignAnalysis::d_t>>
IFDSSignAnalysis::initialSeeds() {
  cout << "IFDSSignAnalysis::initialSeeds()\n";
  map<IFDSSignAnalysis::n_t, set<IFDSSignAnalysis::d_t>> SeedMap;
  for (const auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IFDSSignAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSSignAnalysis::d_t IFDSSignAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSignAnalysis::isZeroValue(IFDSSignAnalysis::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

void IFDSSignAnalysis::printNode(ostream &OS, IFDSSignAnalysis::n_t N) const {
  OS << llvmIRToString(N);
}

void IFDSSignAnalysis::printDataFlowFact(ostream &OS,
                                         IFDSSignAnalysis::d_t D) const {
  OS << llvmIRToString(D);
}

void IFDSSignAnalysis::printFunction(ostream &OS,
                                     IFDSSignAnalysis::f_t M) const {
  OS << M->getName().str();
}

} // namespace psr
