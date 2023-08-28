/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include <utility>

namespace psr {

IFDSSolverTest::IFDSSolverTest(const LLVMProjectIRDB *IRDB,
                               std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getNormalFlowFunction(IFDSSolverTest::n_t /*Curr*/,
                                      IFDSSolverTest::n_t /*Succ*/) {
  return identityFlow();
}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getCallFlowFunction(IFDSSolverTest::n_t /*CallSite*/,
                                    IFDSSolverTest::f_t /*DestFun*/) {
  return identityFlow();
}

IFDSSolverTest::FlowFunctionPtrType IFDSSolverTest::getRetFlowFunction(
    IFDSSolverTest::n_t /*CallSite*/, IFDSSolverTest::f_t /*CalleeFun*/,
    IFDSSolverTest::n_t /*ExitStmt*/, IFDSSolverTest::n_t /*RetSite*/) {
  return identityFlow();
}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getCallToRetFlowFunction(IFDSSolverTest::n_t /*CallSite*/,
                                         IFDSSolverTest::n_t /*RetSite*/,
                                         llvm::ArrayRef<f_t> /*Callees*/) {
  return identityFlow();
}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getSummaryFlowFunction(IFDSSolverTest::n_t /*CallSite*/,
                                       IFDSSolverTest::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSSolverTest::n_t, IFDSSolverTest::d_t, IFDSSolverTest::l_t>
IFDSSolverTest::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSSolverTest::initialSeeds()");
  return createDefaultSeeds();
}

IFDSSolverTest::d_t IFDSSolverTest::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSolverTest::isZeroValue(IFDSSolverTest::d_t Fact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

} // namespace psr
