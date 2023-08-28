/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTypeAnalysis.h"

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include <utility>

using namespace std;
using namespace psr;

namespace psr {

IFDSTypeAnalysis::IFDSTypeAnalysis(const LLVMProjectIRDB *IRDB,
                                   std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IFDSTypeAnalysis::FlowFunctionPtrType
IFDSTypeAnalysis::getNormalFlowFunction(IFDSTypeAnalysis::n_t /*Curr*/,
                                        IFDSTypeAnalysis::n_t /*Succ*/) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t /*Source*/) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

IFDSTypeAnalysis::FlowFunctionPtrType
IFDSTypeAnalysis::getCallFlowFunction(IFDSTypeAnalysis::n_t /*CallSite*/,
                                      IFDSTypeAnalysis::f_t /*DestFun*/) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t /*Source*/) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

IFDSTypeAnalysis::FlowFunctionPtrType IFDSTypeAnalysis::getRetFlowFunction(
    IFDSTypeAnalysis::n_t /*CallSite*/, IFDSTypeAnalysis::f_t /*CalleeFun*/,
    IFDSTypeAnalysis::n_t /*ExitStmt*/, IFDSTypeAnalysis::n_t /*RetSite*/) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t /*Source*/) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

IFDSTypeAnalysis::FlowFunctionPtrType
IFDSTypeAnalysis::getCallToRetFlowFunction(IFDSTypeAnalysis::n_t /*CallSite*/,
                                           IFDSTypeAnalysis::n_t /*RetSite*/,
                                           llvm::ArrayRef<f_t> /*Callees*/) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t /*Source*/) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

IFDSTypeAnalysis::FlowFunctionPtrType
IFDSTypeAnalysis::getSummaryFlowFunction(IFDSTypeAnalysis::n_t /*Curr*/,
                                         IFDSTypeAnalysis::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSTypeAnalysis::n_t, IFDSTypeAnalysis::d_t,
             IFDSTypeAnalysis::l_t>
IFDSTypeAnalysis::initialSeeds() {
  return createDefaultSeeds();
}

IFDSTypeAnalysis::d_t IFDSTypeAnalysis::createZeroValue() const {
  return LLVMZeroValue::getInstance();
}

bool IFDSTypeAnalysis::isZeroValue(IFDSTypeAnalysis::d_t Fact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

} // namespace psr
