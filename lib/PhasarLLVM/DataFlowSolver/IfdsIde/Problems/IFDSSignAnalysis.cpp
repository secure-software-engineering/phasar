/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
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
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IFDSSignAnalysis::ZeroValue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSSignAnalysis::d_t>>
IFDSSignAnalysis::getNormalFlowFunction(IFDSSignAnalysis::n_t curr,
                                        IFDSSignAnalysis::n_t succ) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSignAnalysis::d_t>>
IFDSSignAnalysis::getCallFlowFunction(IFDSSignAnalysis::n_t callStmt,
                                      IFDSSignAnalysis::f_t destFun) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSignAnalysis::d_t>>
IFDSSignAnalysis::getRetFlowFunction(IFDSSignAnalysis::n_t callSite,
                                     IFDSSignAnalysis::f_t calleeFun,
                                     IFDSSignAnalysis::n_t exitStmt,
                                     IFDSSignAnalysis::n_t retSite) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSignAnalysis::d_t>>
IFDSSignAnalysis::getCallToRetFlowFunction(IFDSSignAnalysis::n_t callSite,
                                           IFDSSignAnalysis::n_t retSite,
                                           set<IFDSSignAnalysis::f_t> callees) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSignAnalysis::d_t>>
IFDSSignAnalysis::getSummaryFlowFunction(IFDSSignAnalysis::n_t callStmt,
                                         IFDSSignAnalysis::f_t destFun) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

map<IFDSSignAnalysis::n_t, set<IFDSSignAnalysis::d_t>>
IFDSSignAnalysis::initialSeeds() {
  cout << "IFDSSignAnalysis::initialSeeds()\n";
  map<IFDSSignAnalysis::n_t, set<IFDSSignAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IFDSSignAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSSignAnalysis::d_t IFDSSignAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSignAnalysis::isZeroValue(IFDSSignAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

void IFDSSignAnalysis::printNode(ostream &os, IFDSSignAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSSignAnalysis::printDataFlowFact(ostream &os,
                                         IFDSSignAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSSignAnalysis::printFunction(ostream &os,
                                     IFDSSignAnalysis::f_t m) const {
  os << m->getName().str();
}

} // namespace psr
