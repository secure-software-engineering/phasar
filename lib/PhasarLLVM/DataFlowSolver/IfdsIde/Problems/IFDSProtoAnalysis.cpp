/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSProtoAnalysis.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace psr;
using namespace std;

namespace psr {

IFDSProtoAnalysis::IFDSProtoAnalysis(const ProjectIRDB *IRDB,
                                     const LLVMTypeHierarchy *TH,
                                     const LLVMBasedICFG *ICF,
                                     const LLVMPointsToInfo *PT,
                                     std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IFDSProtoAnalysis::ZeroValue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getNormalFlowFunction(IFDSProtoAnalysis::n_t curr,
                                         IFDSProtoAnalysis::n_t succ) {
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    return make_shared<Gen<IFDSProtoAnalysis::d_t>>(Store->getPointerOperand(),
                                                    getZeroValue());
  }
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getCallFlowFunction(IFDSProtoAnalysis::n_t callStmt,
                                       IFDSProtoAnalysis::f_t destFun) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getRetFlowFunction(IFDSProtoAnalysis::n_t callSite,
                                      IFDSProtoAnalysis::f_t calleeFun,
                                      IFDSProtoAnalysis::n_t exitStmt,
                                      IFDSProtoAnalysis::n_t retSite) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getCallToRetFlowFunction(
    IFDSProtoAnalysis::n_t callSite, IFDSProtoAnalysis::n_t retSite,
    set<IFDSProtoAnalysis::f_t> callees) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getSummaryFlowFunction(IFDSProtoAnalysis::n_t callStmt,
                                          IFDSProtoAnalysis::f_t destFun) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

map<IFDSProtoAnalysis::n_t, set<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::initialSeeds() {
  cout << "IFDSProtoAnalysis::initialSeeds()\n";
  map<IFDSProtoAnalysis::n_t, set<IFDSProtoAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IFDSProtoAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSProtoAnalysis::d_t IFDSProtoAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSProtoAnalysis::isZeroValue(IFDSProtoAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

void IFDSProtoAnalysis::printNode(ostream &os, IFDSProtoAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSProtoAnalysis::printDataFlowFact(ostream &os,
                                          IFDSProtoAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSProtoAnalysis::printFunction(ostream &os,
                                      IFDSProtoAnalysis::f_t m) const {
  os << m->getName().str();
}

} // namespace psr
