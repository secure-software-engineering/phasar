/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/LLVMBasedAliasAnalysis.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/Pointer/AliasAnalysisType.h"

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

struct LLVMBasedAliasAnalysis::Impl {
  llvm::PassBuilder PB{};
  llvm::FunctionAnalysisManager FAM{};
  llvm::FunctionPassManager FPM{};
};

bool LLVMBasedAliasAnalysis::hasAliasInfo(const llvm::Function &Fun) const {
  return AAInfos.find(&Fun) != AAInfos.end();
}

void LLVMBasedAliasAnalysis::computeAliasInfo(llvm::Function &Fun) {
  assert(PImpl != nullptr);
  llvm::PreservedAnalyses PA = PImpl->FPM.run(Fun, PImpl->FAM);
  llvm::AAResults &AAR = PImpl->FAM.getResult<llvm::AAManager>(Fun);
  AAInfos.insert(std::make_pair(&Fun, &AAR));
}

void LLVMBasedAliasAnalysis::erase(llvm::Function *F) noexcept {
  // after we clear all stuff, we need to set it up for the next function-wise
  // analysis
  AAInfos.erase(F);
  PImpl->FAM.clear(*F, F->getName());
}

void LLVMBasedAliasAnalysis::clear() noexcept {
  AAInfos.clear();
  PImpl->FAM.clear();
}

LLVMBasedAliasAnalysis::LLVMBasedAliasAnalysis(LLVMProjectIRDB &IRDB,
                                               bool UseLazyEvaluation,
                                               AliasAnalysisType PATy)
    : PImpl(new Impl{}), PATy(PATy) {

  PImpl->FAM.registerPass([&] {
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
  PImpl->PB.registerFunctionAnalyses(PImpl->FAM);

  if (!UseLazyEvaluation) {
    for (auto &F : *IRDB.getModule()) {
      if (!F.isDeclaration()) {
        computeAliasInfo(F);
      }
    }
  }
}

LLVMBasedAliasAnalysis::~LLVMBasedAliasAnalysis() = default;

} // namespace psr
