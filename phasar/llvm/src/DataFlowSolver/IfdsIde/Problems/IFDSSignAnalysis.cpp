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
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSignAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

namespace psr {

IFDSSignAnalysis::IFDSSignAnalysis(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IFDSSignAnalysis::ZeroValue = IFDSSignAnalysis::createZeroValue();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getNormalFlowFunction(IFDSSignAnalysis::n_t /*Curr*/,
                                        IFDSSignAnalysis::n_t /*Succ*/) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getCallFlowFunction(IFDSSignAnalysis::n_t /*CallSite*/,
                                      IFDSSignAnalysis::f_t /*DestFun*/) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType IFDSSignAnalysis::getRetFlowFunction(
    IFDSSignAnalysis::n_t /*CallSite*/, IFDSSignAnalysis::f_t /*CalleeFun*/,
    IFDSSignAnalysis::n_t /*ExitStmt*/, IFDSSignAnalysis::n_t /*RetSite*/) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getCallToRetFlowFunction(IFDSSignAnalysis::n_t /*CallSite*/,
                                           IFDSSignAnalysis::n_t /*RetSite*/,
                                           llvm::ArrayRef<f_t> /*Callees*/) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getSummaryFlowFunction(IFDSSignAnalysis::n_t /*CallSite*/,
                                         IFDSSignAnalysis::f_t /*DestFun*/) {
  return Identity<IFDSSignAnalysis::d_t>::getInstance();
}

InitialSeeds<IFDSSignAnalysis::n_t, IFDSSignAnalysis::d_t,
             IFDSSignAnalysis::l_t>
IFDSSignAnalysis::initialSeeds() {
  llvm::outs() << "IFDSSignAnalysis::initialSeeds()\n";
  InitialSeeds<IFDSSignAnalysis::n_t, IFDSSignAnalysis::d_t,
               IFDSSignAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue());
  }
  return Seeds;
}

IFDSSignAnalysis::d_t IFDSSignAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSignAnalysis::isZeroValue(IFDSSignAnalysis::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

void IFDSSignAnalysis::printNode(llvm::raw_ostream &OS,
                                 IFDSSignAnalysis::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IFDSSignAnalysis::printDataFlowFact(llvm::raw_ostream &OS,
                                         IFDSSignAnalysis::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IFDSSignAnalysis::printFunction(llvm::raw_ostream &OS,
                                     IFDSSignAnalysis::f_t Func) const {
  OS << Func->getName();
}

} // namespace psr
