/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/ControlFlow/CallGraphData.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Soundness.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/ErrorHandling.h"

#include <utility>

namespace psr {

void LLVMBasedICFG::initialize(LLVMProjectIRDB *IRDB, Resolver &CGResolver,
                               llvm::ArrayRef<std::string> EntryPoints,
                               Soundness S, bool IncludeGlobals) {
  if (IncludeGlobals) {
    auto *EntryFun = GlobalCtorsDtorsModel::buildModel(*IRDB, EntryPoints);
    this->CG = buildLLVMBasedCallGraph(*IRDB, CGResolver, {EntryFun}, S);
  } else {
    this->CG = buildLLVMBasedCallGraph(*IRDB, CGResolver, EntryPoints, S);
  }
}

LLVMBasedICFG::LLVMBasedICFG(LLVMProjectIRDB *IRDB,
                             CallGraphAnalysisType CGType,
                             llvm::ArrayRef<std::string> EntryPoints,
                             DIBasedTypeHierarchy *TH, LLVMAliasInfoRef PT,
                             Soundness S, bool IncludeGlobals)
    : IRDB(IRDB), VTP(*IRDB) {
  assert(IRDB != nullptr);

  LLVMAliasInfo PTOwn;

  if (!PT && CGType == CallGraphAnalysisType::OTF) {
    PTOwn = std::make_unique<LLVMAliasSet>(IRDB);
    PT = PTOwn.asRef();
  }

  auto CGRes = Resolver::create(CGType, IRDB, &VTP, TH, PT);
  initialize(IRDB, *CGRes, EntryPoints, S, IncludeGlobals);
}

LLVMBasedICFG::LLVMBasedICFG(LLVMProjectIRDB *IRDB, Resolver &CGResolver,
                             llvm::ArrayRef<std::string> EntryPoints,
                             Soundness S, bool IncludeGlobals)
    : IRDB(IRDB), VTP(*IRDB) {
  assert(IRDB != nullptr);

  initialize(IRDB, CGResolver, EntryPoints, S, IncludeGlobals);
}

LLVMBasedICFG::LLVMBasedICFG(LLVMProjectIRDB *IRDB, Resolver &CGResolver,
                             LLVMVFTableProvider VTP,
                             llvm::ArrayRef<std::string> EntryPoints,
                             Soundness S, bool IncludeGlobals)
    : IRDB(IRDB), VTP(std::move(VTP)) {
  assert(IRDB != nullptr);
  initialize(IRDB, CGResolver, EntryPoints, S, IncludeGlobals);
}

LLVMBasedICFG::LLVMBasedICFG(CallGraph<n_t, f_t> CG,
                             const LLVMProjectIRDB *IRDB)
    : CG(std::move(CG)), IRDB(IRDB), VTP(*IRDB) {}

LLVMBasedICFG::LLVMBasedICFG(const LLVMProjectIRDB *IRDB,
                             const CallGraphData &SerializedCG)
    : CG(CallGraph<n_t, f_t>::deserialize(
          SerializedCG,
          [IRDB](llvm::StringRef Name) { return IRDB->getFunction(Name); },
          [IRDB](size_t Id) { return IRDB->getInstruction(Id); })),
      IRDB(IRDB), VTP(*IRDB) {}

LLVMBasedICFG::~LLVMBasedICFG() = default;

[[nodiscard]] FunctionRange LLVMBasedICFG::getAllFunctionsImpl() const {
  return IRDB->getAllFunctions();
}

[[nodiscard]] auto LLVMBasedICFG::getFunctionImpl(llvm::StringRef Fun) const
    -> f_t {
  return IRDB->getFunction(Fun);
}

[[nodiscard]] bool LLVMBasedICFG::isIndirectFunctionCallImpl(n_t Inst) const {
  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Inst);
  return CallSite && CallSite->isIndirectCall();
}

[[nodiscard]] bool LLVMBasedICFG::isVirtualFunctionCallImpl(n_t Inst) const {
  return psr::isVirtualCall(Inst, VTP);
}

[[nodiscard]] auto LLVMBasedICFG::allNonCallStartNodesImpl() const
    -> std::vector<n_t> {
  std::vector<n_t> NonCallStartNodes;
  NonCallStartNodes.reserve(2 * IRDB->getNumFunctions());
  for (const auto *Inst : IRDB->getAllInstructions()) {
    if (!llvm::isa<llvm::CallBase>(Inst) && !isStartPoint(Inst)) {
      NonCallStartNodes.push_back(Inst);
    }
  }

  return NonCallStartNodes;
}

[[nodiscard]] auto LLVMBasedICFG::getCallsFromWithinImpl(f_t Fun) const
    -> llvm::SmallVector<n_t> {
  llvm::SmallVector<n_t> CallSites;
  for (const auto &I : llvm::instructions(Fun)) {
    if (llvm::isa<llvm::CallBase>(I)) {
      CallSites.push_back(&I);
    }
  }
  return CallSites;
}

[[nodiscard]] auto LLVMBasedICFG::getReturnSitesOfCallAtImpl(n_t Inst) const
    -> llvm::SmallVector<n_t, 2> {
  /// Currently, we don't distinguish normal-dest and unwind-dest, so we can
  /// just use getSuccsOf

  return getSuccsOf(Inst);
}

void LLVMBasedICFG::printImpl(llvm::raw_ostream &OS) const {
  CG.printAsDot(
      OS, [](f_t Fun) { return Fun->getName(); },
      [](n_t CS) { return CS->getFunction(); },
      [](n_t CS) { return llvmIRToStableString(CS); });
}

void LLVMBasedICFG::printAsJsonImpl(llvm::raw_ostream &OS) const {
  CG.printAsJson(
      OS, [](f_t F) { return F->getName().str(); },
      [this](n_t Inst) { return IRDB->getInstructionId(Inst); });
}

nlohmann::json LLVMBasedICFG::getAsJsonImpl() const {
  return CG.getAsJson(
      [](f_t F) { return F->getName().str(); },
      [this](n_t Inst) { return IRDB->getInstructionId(Inst); });
}

template class ICFGBase<LLVMBasedICFG>;

} // namespace psr
