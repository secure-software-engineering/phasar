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
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSProtoAnalysis.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace psr;
using namespace std;

namespace psr {

IFDSProtoAnalysis::IFDSProtoAnalysis(IFDSProtoAnalysis::i_t icfg,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  IFDSProtoAnalysis::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getNormalFlowFunction(IFDSProtoAnalysis::n_t curr,
                                         IFDSProtoAnalysis::n_t succ) {
  cout << "IFDSProtoAnalysis::getNormalFlowFunction()\n";
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    return make_shared<Gen<IFDSProtoAnalysis::d_t>>(
        Store->getPointerOperand(), DefaultIFDSTabulationProblem::zerovalue);
  }
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getCallFlowFunction(IFDSProtoAnalysis::n_t callStmt,
                                       IFDSProtoAnalysis::m_t destMthd) {
  cout << "IFDSProtoAnalysis::getCallFlowFunction()\n";
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getRetFlowFunction(IFDSProtoAnalysis::n_t callSite,
                                      IFDSProtoAnalysis::m_t calleeMthd,
                                      IFDSProtoAnalysis::n_t exitStmt,
                                      IFDSProtoAnalysis::n_t retSite) {
  cout << "IFDSProtoAnalysis::getRetFlowFunction()\n";
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getCallToRetFlowFunction(
    IFDSProtoAnalysis::n_t callSite, IFDSProtoAnalysis::n_t retSite,
    set<IFDSProtoAnalysis::m_t> callees) {
  cout << "IFDSProtoAnalysis::getCallToRetFlowFunction()\n";
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::getSummaryFlowFunction(IFDSProtoAnalysis::n_t callStmt,
                                          IFDSProtoAnalysis::m_t destMthd) {
  cout << "IFDSProtoAnalysis::getSummaryFlowFunction()\n";
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

map<IFDSProtoAnalysis::n_t, set<IFDSProtoAnalysis::d_t>>
IFDSProtoAnalysis::initialSeeds() {
  cout << "IFDSProtoAnalysis::initialSeeds()\n";
  map<IFDSProtoAnalysis::n_t, set<IFDSProtoAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                             set<IFDSProtoAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSProtoAnalysis::d_t IFDSProtoAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSProtoAnalysis::isZeroValue(IFDSProtoAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

void IFDSProtoAnalysis::printNode(ostream &os, IFDSProtoAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSProtoAnalysis::printDataFlowFact(ostream &os,
                                          IFDSProtoAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSProtoAnalysis::printMethod(ostream &os,
                                    IFDSProtoAnalysis::m_t m) const {
  os << m->getName().str();
}

} // namespace psr
