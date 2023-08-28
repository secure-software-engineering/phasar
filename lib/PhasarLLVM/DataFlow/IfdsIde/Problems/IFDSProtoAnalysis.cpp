/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSProtoAnalysis.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include <utility>

namespace psr {

IFDSProtoAnalysis::IFDSProtoAnalysis(const LLVMProjectIRDB *IRDB,
                                     std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getNormalFlowFunction(IFDSProtoAnalysis::n_t Curr,
                                         IFDSProtoAnalysis::n_t /*Succ*/) {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return generateFromZero(Store->getPointerOperand());
  }
  return identityFlow();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getCallFlowFunction(IFDSProtoAnalysis::n_t /*CallSite*/,
                                       IFDSProtoAnalysis::f_t /*DestFun*/) {
  return identityFlow();
}

IFDSProtoAnalysis::FlowFunctionPtrType IFDSProtoAnalysis::getRetFlowFunction(
    IFDSProtoAnalysis::n_t /*CallSite*/, IFDSProtoAnalysis::f_t /*CalleeFun*/,
    IFDSProtoAnalysis::n_t /*ExitInst*/, IFDSProtoAnalysis::n_t /*RetSite*/) {
  return identityFlow();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getCallToRetFlowFunction(IFDSProtoAnalysis::n_t /*CallSite*/,
                                            IFDSProtoAnalysis::n_t /*RetSite*/,
                                            llvm::ArrayRef<f_t> /*Callees*/) {
  return identityFlow();
}

IFDSProtoAnalysis::FlowFunctionPtrType
IFDSProtoAnalysis::getSummaryFlowFunction(IFDSProtoAnalysis::n_t /*CallSite*/,
                                          IFDSProtoAnalysis::f_t /*DestFun*/) {
  return identityFlow();
}

InitialSeeds<IFDSProtoAnalysis::n_t, IFDSProtoAnalysis::d_t,
             IFDSProtoAnalysis::l_t>
IFDSProtoAnalysis::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSProtoAnalysis::initialSeeds()");
  return createDefaultSeeds();
}

IFDSProtoAnalysis::d_t IFDSProtoAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSProtoAnalysis::isZeroValue(
    IFDSProtoAnalysis::d_t Fact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

} // namespace psr
