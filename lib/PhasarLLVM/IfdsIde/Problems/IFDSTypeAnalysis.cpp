/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTypeAnalysis.h>

#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

IFDSTypeAnalysis::IFDSTypeAnalysis(IFDSTypeAnalysis::i_t icfg,
                                   vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  IFDSTypeAnalysis::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::getNormalFlowFunction(IFDSTypeAnalysis::n_t curr,
                                        IFDSTypeAnalysis::n_t succ) {
  cout << "type analysis getNormalFlowFunction()" << endl;
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
                                      IFDSTypeAnalysis::m_t destMthd) {
  cout << "type analysis getCallFlowFunction()" << endl;
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
                                     IFDSTypeAnalysis::m_t calleeMthd,
                                     IFDSTypeAnalysis::n_t exitStmt,
                                     IFDSTypeAnalysis::n_t retSite) {
  cout << "type analysis getRetFlowFunction()" << endl;
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
                                           set<IFDSTypeAnalysis::m_t> callees) {
  cout << "type analysis getCallToRetFlowFunction()" << endl;
  struct TAFF : FlowFunction<IFDSTypeAnalysis::d_t> {
    set<IFDSTypeAnalysis::d_t>
    computeTargets(IFDSTypeAnalysis::d_t source) override {
      return set<IFDSTypeAnalysis::d_t>{};
    }
  };
  return make_shared<TAFF>();
}

map<IFDSTypeAnalysis::n_t, set<IFDSTypeAnalysis::d_t>>
IFDSTypeAnalysis::initialSeeds() {
  map<IFDSTypeAnalysis::n_t, set<IFDSTypeAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                             set<IFDSTypeAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSTypeAnalysis::d_t IFDSTypeAnalysis::createZeroValue() {
  return LLVMZeroValue::getInstance();
}

bool IFDSTypeAnalysis::isZeroValue(IFDSTypeAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

void IFDSTypeAnalysis::printNode(ostream &os, IFDSTypeAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSTypeAnalysis::printDataFlowFact(ostream &os,
                                         IFDSTypeAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSTypeAnalysis::printMethod(ostream &os, IFDSTypeAnalysis::m_t m) const {
  os << m->getName().str();
}

} // namespace psr
