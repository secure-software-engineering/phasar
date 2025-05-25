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

static llvm::Type *getPointeeTypeOrNull(const llvm::Value *Ptr) {
  assert(Ptr->getType()->isPointerTy());

  if (!Ptr->getType()->isOpaquePointerTy()) {
    return Ptr->getType()->getNonOpaquePointerElementType();
  }

  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Ptr)) {
    if (auto *Ty = Arg->getParamByValType()) {
      return Ty;
    }
    if (auto *Ty = Arg->getParamStructRetType()) {
      return Ty;
    }
  }
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Ptr)) {
    return Alloca->getAllocatedType();
  }
  return nullptr;
}

AliasResult LLVMBasedAliasAnalysis::aliasImpl(llvm::AAResults *AA,
                                              const llvm::Value *V,
                                              const llvm::Value *Rep,
                                              const llvm::DataLayout &DL) {

  assert(V->getType()->isPointerTy());
  assert(Rep->getType()->isPointerTy());

  auto *ElTy = getPointeeTypeOrNull(V);
  auto *RepElTy = getPointeeTypeOrNull(Rep);

  auto VSize = ElTy && ElTy->isSized()
                   ? llvm::LocationSize::precise(DL.getTypeStoreSize(ElTy))
                   : llvm::LocationSize::precise(1);

  auto RepSize = RepElTy && RepElTy->isSized()
                     ? llvm::LocationSize::precise(DL.getTypeStoreSize(RepElTy))
                     : llvm::LocationSize::precise(1);

  return translateAAResult(AA->alias(V, VSize, Rep, RepSize));
}

std::unique_ptr<AliasAnalysisView> AliasAnalysisView::createLLVMBasedAnalysis(
    LLVMProjectIRDB &IRDB, bool UseLazyEvaluation, AliasAnalysisType PATy) {
  return std::make_unique<LLVMBasedAliasAnalysis>(IRDB, UseLazyEvaluation,
                                                  PATy);
}

} // namespace psr
