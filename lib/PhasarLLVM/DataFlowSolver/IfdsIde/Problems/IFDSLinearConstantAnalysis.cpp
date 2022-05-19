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
  return hash<const llvm::Value *>()(K.First) ^ hash<int>()(K.Second);
}

namespace psr {

LCAPair::LCAPair(const llvm::Value *V, int I) : First(V), Second(I) {}

bool operator==(const LCAPair &Lhs, const LCAPair &Rhs) {
  return tie(Lhs.First, Lhs.Second) == tie(Rhs.First, Rhs.Second);
}

bool operator!=(const LCAPair &Lhs, const LCAPair &Rhs) {
  return !(Lhs == Rhs);
}

bool operator<(const LCAPair &Lhs, const LCAPair &Rhs) {
  return tie(Lhs.First, Lhs.Second) < tie(Rhs.First, Rhs.Second);
}

IFDSLinearConstantAnalysis::IFDSLinearConstantAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IFDSLinearConstantAnalysis::ZeroValue =
      IFDSLinearConstantAnalysis::createZeroValue();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getNormalFlowFunction(
    IFDSLinearConstantAnalysis::n_t /*Curr*/,
    IFDSLinearConstantAnalysis::n_t /*Succ*/) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getCallFlowFunction(
    IFDSLinearConstantAnalysis::n_t /*CallSite*/,
    IFDSLinearConstantAnalysis::f_t /*DestFun*/) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getRetFlowFunction(
    IFDSLinearConstantAnalysis::n_t /*CallSite*/,
    IFDSLinearConstantAnalysis::f_t /*CalleeFun*/,
    IFDSLinearConstantAnalysis::n_t /*ExitStmt*/,
    IFDSLinearConstantAnalysis::n_t /*RetSite*/) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getCallToRetFlowFunction(
    IFDSLinearConstantAnalysis::n_t /*CallSite*/,
    IFDSLinearConstantAnalysis::n_t /*RetSite*/,
    set<IFDSLinearConstantAnalysis::f_t> /* Callees */) {
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

IFDSLinearConstantAnalysis::FlowFunctionPtrType
IFDSLinearConstantAnalysis::getSummaryFlowFunction(
    IFDSLinearConstantAnalysis::n_t /*CallSite*/,
    IFDSLinearConstantAnalysis::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSLinearConstantAnalysis::n_t, IFDSLinearConstantAnalysis::d_t,
             IFDSLinearConstantAnalysis::l_t>
IFDSLinearConstantAnalysis::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSLinearConstantAnalysis::initialSeeds()");
  InitialSeeds<IFDSLinearConstantAnalysis::n_t, IFDSLinearConstantAnalysis::d_t,
               IFDSLinearConstantAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue());
  }
  return Seeds;
}

IFDSLinearConstantAnalysis::d_t
IFDSLinearConstantAnalysis::createZeroValue() const {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSLinearConstantAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return {LLVMZeroValue::getInstance(), 0};
}

bool IFDSLinearConstantAnalysis::isZeroValue(
    IFDSLinearConstantAnalysis::d_t Fact) const {
  return Fact == ZeroValue;
}

void IFDSLinearConstantAnalysis::printNode(
    llvm::raw_ostream &OS, IFDSLinearConstantAnalysis::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IFDSLinearConstantAnalysis::printDataFlowFact(
    llvm::raw_ostream &OS, IFDSLinearConstantAnalysis::d_t Fact) const {
  OS << '<' + llvmIRToString(Fact.First) + ", " + std::to_string(Fact.Second) +
            '>';
}

void IFDSLinearConstantAnalysis::printFunction(
    llvm::raw_ostream &OS, IFDSLinearConstantAnalysis::f_t Func) const {
  OS << Func->getName();
}

} // namespace psr
