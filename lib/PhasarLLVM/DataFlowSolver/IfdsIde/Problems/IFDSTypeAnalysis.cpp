/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <utility>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTypeAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

IFDSTypeAnalysis::IFDSTypeAnalysis(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IFDSTypeAnalysis::ZeroValue = IFDSTypeAnalysis::createZeroValue();
}

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
IFDSTypeAnalysis::getCallToRetFlowFunction(
    IFDSTypeAnalysis::n_t /*CallSite*/, IFDSTypeAnalysis::n_t /*RetSite*/,
    set<IFDSTypeAnalysis::f_t> /*Callees*/) {
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
  InitialSeeds<IFDSTypeAnalysis::n_t, IFDSTypeAnalysis::d_t,
               IFDSTypeAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue());
  }
  return Seeds;
}

IFDSTypeAnalysis::d_t IFDSTypeAnalysis::createZeroValue() const {
  return LLVMZeroValue::getInstance();
}

bool IFDSTypeAnalysis::isZeroValue(IFDSTypeAnalysis::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

void IFDSTypeAnalysis::printNode(ostream &OS,
                                 IFDSTypeAnalysis::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IFDSTypeAnalysis::printDataFlowFact(ostream &OS,
                                         IFDSTypeAnalysis::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IFDSTypeAnalysis::printFunction(ostream &OS,
                                     IFDSTypeAnalysis::f_t Func) const {
  OS << Func->getName().str();
}

} // namespace psr
