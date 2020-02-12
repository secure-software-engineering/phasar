/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTypeAnalysis.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

IFDSTypeAnalysis::IFDSTypeAnalysis(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IFDSTypeAnalysis::ZeroValue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::getNormalFlowFunction(IFDSTypeAnalysis::n_t curr,
                                        IFDSTypeAnalysis::n_t succ) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t source) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

shared_ptr<FlowFunction<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::getCallFlowFunction(IFDSTypeAnalysis::n_t callStmt,
                                      IFDSTypeAnalysis::f_t destFun) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t source) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

shared_ptr<FlowFunction<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::getRetFlowFunction(IFDSTypeAnalysis::n_t callSite,
                                     IFDSTypeAnalysis::f_t calleeFun,
                                     IFDSTypeAnalysis::n_t exitStmt,
                                     IFDSTypeAnalysis::n_t retSite) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t source) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

shared_ptr<FlowFunction<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::getCallToRetFlowFunction(IFDSTypeAnalysis::n_t callSite,
                                           IFDSTypeAnalysis::n_t retSite,
                                           set<IFDSTypeAnalysis::f_t> callees) {
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t source) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

shared_ptr<FlowFunction<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::getSummaryFlowFunction(IFDSTypeAnalysis::n_t curr,
                                         IFDSTypeAnalysis::f_t destFun) {
  return nullptr;
}

map<IFDSTypeAnalysis::n_t, set<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::initialSeeds() {
  map<IFDSTypeAnalysis::n_t, set<IFDSTypeAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IFDSTypeAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSTypeAnalysis::d_t IFDSTypeAnalysis::createZeroValue() const {
  return LLVMZeroValue::getInstance();
}

bool IFDSTypeAnalysis::isZeroValue(IFDSTypeAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

void IFDSTypeAnalysis::printNode(ostream &os, IFDSTypeAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSTypeAnalysis::printDataFlowFact(ostream &os,
                                         IFDSTypeAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSTypeAnalysis::printFunction(ostream &os,
                                     IFDSTypeAnalysis::f_t m) const {
  os << m->getName().str();
}

} // namespace psr
