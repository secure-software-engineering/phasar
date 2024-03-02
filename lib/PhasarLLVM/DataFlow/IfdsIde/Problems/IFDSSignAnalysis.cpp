/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSignAnalysis.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include <utility>

namespace psr {

IFDSSignAnalysis::IFDSSignAnalysis(const LLVMProjectIRDB *IRDB,
                                   std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getNormalFlowFunction(IFDSSignAnalysis::n_t /*Curr*/,
                                        IFDSSignAnalysis::n_t /*Succ*/) {
  return identityFlow();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getCallFlowFunction(IFDSSignAnalysis::n_t /*CallSite*/,
                                      IFDSSignAnalysis::f_t /*DestFun*/) {
  return identityFlow();
}

IFDSSignAnalysis::FlowFunctionPtrType IFDSSignAnalysis::getRetFlowFunction(
    IFDSSignAnalysis::n_t /*CallSite*/, IFDSSignAnalysis::f_t /*CalleeFun*/,
    IFDSSignAnalysis::n_t /*ExitStmt*/, IFDSSignAnalysis::n_t /*RetSite*/) {
  return identityFlow();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getCallToRetFlowFunction(IFDSSignAnalysis::n_t /*CallSite*/,
                                           IFDSSignAnalysis::n_t /*RetSite*/,
                                           llvm::ArrayRef<f_t> /*Callees*/) {
  return identityFlow();
}

IFDSSignAnalysis::FlowFunctionPtrType
IFDSSignAnalysis::getSummaryFlowFunction(IFDSSignAnalysis::n_t /*CallSite*/,
                                         IFDSSignAnalysis::f_t /*DestFun*/) {
  return identityFlow();
}

InitialSeeds<IFDSSignAnalysis::n_t, IFDSSignAnalysis::d_t,
             IFDSSignAnalysis::l_t>
IFDSSignAnalysis::initialSeeds() {
  llvm::outs() << "IFDSSignAnalysis::initialSeeds()\n";
  return createDefaultSeeds();
}

IFDSSignAnalysis::d_t IFDSSignAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSignAnalysis::isZeroValue(IFDSSignAnalysis::d_t Fact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

} // namespace psr
