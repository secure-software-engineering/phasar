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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

size_t hash<LCAPair>::operator()(const LCAPair &K) const {
  return hash<const llvm::Value *>()(K.first) ^ hash<int>()(K.second);
}

namespace psr {

LCAPair::LCAPair() : first(nullptr), second(0) {}

LCAPair::LCAPair(const llvm::Value *V, int I) : first(V), second(I) {}

bool operator==(const LCAPair &Lhs, const LCAPair &Rhs) {
  return tie(Lhs.first, Lhs.second) == tie(Rhs.first, Rhs.second);
}

bool operator!=(const LCAPair &Lhs, const LCAPair &Rhs) {
  return !(Lhs == Rhs);
}

bool operator<(const LCAPair &Lhs, const LCAPair &Rhs) {
  return tie(Lhs.first, Lhs.second) < tie(Rhs.first, Rhs.second);
}

IFDSLinearConstantAnalysis::IFDSLinearConstantAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IFDSLinearConstantAnalysis::ZeroValue = createZeroValue();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getNormalFlowFunction(
    IFDSLinearConstantAnalysis::n_t Curr,
    IFDSLinearConstantAnalysis::n_t Succ) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getCallFlowFunction(
    IFDSLinearConstantAnalysis::n_t CallStmt,
    IFDSLinearConstantAnalysis::f_t DestFun) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getRetFlowFunction(
    IFDSLinearConstantAnalysis::n_t CallSite,
    IFDSLinearConstantAnalysis::f_t CalleeFun,
    IFDSLinearConstantAnalysis::n_t ExitStmt,
    IFDSLinearConstantAnalysis::n_t RetSite) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getCallToRetFlowFunction(
    IFDSLinearConstantAnalysis::n_t CallSite,
    IFDSLinearConstantAnalysis::n_t RetSite,
    set<IFDSLinearConstantAnalysis::f_t> Callees) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getSummaryFlowFunction(
    IFDSLinearConstantAnalysis::n_t CallStmt,
    IFDSLinearConstantAnalysis::f_t DestFun) {
  return nullptr;
}

map<IFDSLinearConstantAnalysis::n_t, set<IFDSLinearConstantAnalysis::d_t>>
IFDSLinearConstantAnalysis::initialSeeds() {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "IFDSLinearConstantAnalysis::initialSeeds()");
  map<IFDSLinearConstantAnalysis::n_t, set<IFDSLinearConstantAnalysis::d_t>>
      SeedMap;
  for (const auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                  set<IFDSLinearConstantAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSLinearConstantAnalysis::d_t
IFDSLinearConstantAnalysis::createZeroValue() const {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "IFDSLinearConstantAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LCAPair(LLVMZeroValue::getInstance(), 0);
}

bool IFDSLinearConstantAnalysis::isZeroValue(
    IFDSLinearConstantAnalysis::d_t D) const {
  return D == ZeroValue;
}

void IFDSLinearConstantAnalysis::printNode(
    ostream &OS, IFDSLinearConstantAnalysis::n_t N) const {
  OS << llvmIRToString(N);
}

void IFDSLinearConstantAnalysis::printDataFlowFact(
    ostream &OS, IFDSLinearConstantAnalysis::d_t D) const {
  OS << '<' + llvmIRToString(D.first) + ", " + std::to_string(D.second) + '>';
}

void IFDSLinearConstantAnalysis::printFunction(
    ostream &OS, IFDSLinearConstantAnalysis::f_t M) const {
  OS << M->getName().str();
}

} // namespace psr
