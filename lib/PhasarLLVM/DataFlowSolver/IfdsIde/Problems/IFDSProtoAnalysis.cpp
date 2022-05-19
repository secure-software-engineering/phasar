/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

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
  IFDSProtoAnalysis::ZeroValue = IFDSProtoAnalysis::createZeroValue();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getNormalFlowFunction(IFDSProtoAnalysis::n_t Curr,
                                         IFDSProtoAnalysis::n_t /*Succ*/) {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return make_shared<Gen<IFDSProtoAnalysis::d_t>>(Store->getPointerOperand(),
                                                    getZeroValue());
  }
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getCallFlowFunction(IFDSProtoAnalysis::n_t /*CallSite*/,
                                       IFDSProtoAnalysis::f_t /*DestFun*/) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType IFDSProtoAnalysis::getRetFlowFunction(
    IFDSProtoAnalysis::n_t /*CallSite*/, IFDSProtoAnalysis::f_t /*CalleeFun*/,
    IFDSProtoAnalysis::n_t /*ExitInst*/, IFDSProtoAnalysis::n_t /*RetSite*/) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getCallToRetFlowFunction(
    IFDSProtoAnalysis::n_t /*CallSite*/, IFDSProtoAnalysis::n_t /*RetSite*/,
    set<IFDSProtoAnalysis::f_t> /*Callees*/) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getSummaryFlowFunction(IFDSProtoAnalysis::n_t /*CallSite*/,
                                          IFDSProtoAnalysis::f_t /*DestFun*/) {
  return Identity<IFDSProtoAnalysis::d_t>::getInstance();
}

InitialSeeds<IFDSProtoAnalysis::n_t, IFDSProtoAnalysis::d_t,
             IFDSProtoAnalysis::l_t>
IFDSProtoAnalysis::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSProtoAnalysis::initialSeeds()");
  InitialSeeds<IFDSProtoAnalysis::n_t, IFDSProtoAnalysis::d_t,
               IFDSProtoAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue());
  }
  return Seeds;
}

IFDSProtoAnalysis::d_t IFDSProtoAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSProtoAnalysis::isZeroValue(IFDSProtoAnalysis::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

void IFDSProtoAnalysis::printNode(llvm::raw_ostream &OS,
                                  IFDSProtoAnalysis::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IFDSProtoAnalysis::printDataFlowFact(llvm::raw_ostream &OS,
                                          IFDSProtoAnalysis::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IFDSProtoAnalysis::printFunction(llvm::raw_ostream &OS,
                                      IFDSProtoAnalysis::f_t Func) const {
  OS << Func->getName();
}

} // namespace psr
