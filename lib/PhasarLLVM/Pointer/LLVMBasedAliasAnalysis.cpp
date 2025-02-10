/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "LLVMBasedAliasAnalysis.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"

#include "external/llvm/CFLAndersAliasAnalysis.h"
#include "external/llvm/CFLSteensAliasAnalysis.h"

using namespace psr;

namespace psr {

bool LLVMBasedAliasAnalysis::hasAliasInfo(const llvm::Function &Fun) const {
  return AAInfos.find(&Fun) != AAInfos.end();
}

void LLVMBasedAliasAnalysis::computeAliasInfo(llvm::Function &Fun) {
  llvm::PreservedAnalyses PA = FPM.run(Fun, FAM);
  llvm::AAResults &AAR = FAM.getResult<llvm::AAManager>(Fun);
  AAInfos.insert(std::make_pair(&Fun, &AAR));
}

void LLVMBasedAliasAnalysis::doErase(llvm::Function *F) noexcept {
  // after we clear all stuff, we need to set it up for the next function-wise
  // analysis
  AAInfos.erase(F);
  FAM.clear(*F, F->getName());
}

void LLVMBasedAliasAnalysis::doClear() noexcept {
  AAInfos.clear();
  FAM.clear();
}

LLVMBasedAliasAnalysis::LLVMBasedAliasAnalysis(LLVMProjectIRDB &IRDB,
                                               bool UseLazyEvaluation,
                                               AliasAnalysisType PATy)
    : AliasAnalysisView(PATy) {

  FAM.registerPass([&] {
    llvm::AAManager AA;
    switch (PATy) {
    case AliasAnalysisType::CFLAnders:
      AA.registerFunctionAnalysis<llvm::CFLAndersAA>();
      break;
    case AliasAnalysisType::CFLSteens:
      AA.registerFunctionAnalysis<llvm::CFLSteensAA>();
      break;
    case AliasAnalysisType::Basic:
      [[fallthrough]];
    default:
      break;
    }
    // Note: The order of the alias analyses is important. See LLVM's source
    // code for reference (e.g. registerAAAnalyses() in
    // llvm/CodeGen/CodeGenPassBuilder.h)
    //
    AA.registerFunctionAnalysis<llvm::TypeBasedAA>();
    AA.registerFunctionAnalysis<llvm::ScopedNoAliasAA>();
    AA.registerFunctionAnalysis<llvm::BasicAA>();
    return AA;
  });
  PB.registerFunctionAnalyses(FAM);

  if (!UseLazyEvaluation) {
    for (auto &F : *IRDB.getModule()) {
      if (!F.isDeclaration()) {
        computeAliasInfo(F);
      }
    }
  }
}

LLVMBasedAliasAnalysis::~LLVMBasedAliasAnalysis() = default;

static AliasResult translateAAResult(llvm::AliasResult Res) noexcept {
  switch (Res) {
  case llvm::AliasResult::NoAlias:
    return AliasResult::NoAlias;
  case llvm::AliasResult::MayAlias:
    return AliasResult::MayAlias;
  case llvm::AliasResult::PartialAlias:
    return AliasResult::PartialAlias;
  case llvm::AliasResult::MustAlias:
    return AliasResult::MustAlias;
  }
}

AliasResult LLVMBasedAliasAnalysis::aliasImpl(void *AACtx, const llvm::Value *V,
                                              const llvm::Value *Rep,
                                              const llvm::DataLayout &DL) {

  assert(V->getType()->isPointerTy());
  assert(Rep->getType()->isPointerTy());

  auto *AA = static_cast<llvm::AAResults *>(AACtx);
  auto *ElTy = !V->getType()->isOpaquePointerTy()
                   ? V->getType()->getNonOpaquePointerElementType()
                   : nullptr;
  auto *RepElTy = !Rep->getType()->isOpaquePointerTy()
                      ? Rep->getType()->getNonOpaquePointerElementType()
                      : nullptr;

  auto VSize = ElTy && ElTy->isSized() ? DL.getTypeStoreSize(ElTy)
                                       : llvm::MemoryLocation::UnknownSize;

  auto RepSize = RepElTy && RepElTy->isSized()
                     ? DL.getTypeStoreSize(RepElTy)
                     : llvm::MemoryLocation::UnknownSize;

  return translateAAResult(AA->alias(V, VSize, Rep, RepSize));
}

} // namespace psr
