/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/CFLAndersAliasAnalysis.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>

namespace psr {

LLVMPointsToInfo::LLVMPointsToInfo(ProjectIRDB &IRDB) {
  llvm::PassBuilder PB;
  // Create the analysis manager
  // llvm::AAManager AA = PB.buildDefaultAAPipeline();
  llvm::AAManager AA;
  AA.registerFunctionAnalysis<llvm::BasicAA>();
  // AA.registerFunctionAnalysis<llvm::CFLAndersAA>();
  llvm::FunctionAnalysisManager FAM;
  FAM.registerPass([&] { return std::move(AA); });
  PB.registerFunctionAnalyses(FAM);
  llvm::FunctionPassManager FPM;
  // Always verify the input.
  FPM.addPass(llvm::VerifierPass());
  for (llvm::Module *M : IRDB.getAllModules()) {
    for (auto &F : *M) {
      if (!F.isDeclaration()) {
        llvm::PreservedAnalyses PA = FPM.run(F, FAM);
        llvm::AAResults AAR(std::move(FAM.getResult<llvm::AAManager>(F)));
        PointsToGraphs.insert(
            std::make_pair(&F, std::make_unique<PointsToGraph>(&F, AAR)));
        AAInfos.insert(std::make_pair(&F, std::move(AAR)));
      }
    }
  }
}

AliasResult LLVMPointsToInfo::alias(const llvm::Value *V1,
                                    const llvm::Value *V2) {
  // delegate to LLVM's intra-procedural alias analysis results
  const llvm::Function *V1F;
  const llvm::Function *V2F;
  // determine V1F
  if (auto T = llvm::dyn_cast<llvm::Instruction>(V1)) {
    V1F = T->getFunction();
  }
  if (auto T = llvm::dyn_cast<llvm::BasicBlock>(V1)) {
    V1F = T->getParent();
  }
  if (auto T = llvm::dyn_cast<llvm::Argument>(V1)) {
    V1F = T->getParent();
  }
  // determine V2F
  if (auto T = llvm::dyn_cast<llvm::Instruction>(V2)) {
    V2F = T->getFunction();
  }
  if (auto T = llvm::dyn_cast<llvm::BasicBlock>(V2)) {
    V2F = T->getParent();
  }
  if (auto T = llvm::dyn_cast<llvm::Argument>(V2)) {
    V2F = T->getParent();
  }
  if (V1F != V2F) {
    // don't know, used intra-procedural information for inter-procedural query
    return AliasResult::MayAlias;
  }
  switch (AAInfos.at(V1F).alias(V1, V2)) {
  case llvm::NoAlias:
    return AliasResult::NoAlias;
    break;
  case llvm::MayAlias:
    return AliasResult::MayAlias;
    break;
  case llvm::PartialAlias:
    return AliasResult::PartialAlias;
    break;
  case llvm::MustAlias:
    return AliasResult::MustAlias;
    break;
  }
}

std::set<const llvm::Value *>
LLVMPointsToInfo::getPointsToSet(const llvm::Value *V) const {
  // Value can be one of
  //  -Argument
  //  -BasicBlock
  //  -InlineAsm
  //  -MetadataAsValue
  //  -User
  if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return getPointsToGraph(Arg->getParent())->getPointsToSet(V);
  }
  if (auto Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    return getPointsToGraph(Inst->getFunction())->getPointsToSet(V);
  }
  return {};
}

nlohmann::json LLVMPointsToInfo::getAsJson() const { return ""_json; }

PointsToGraph *
LLVMPointsToInfo::getPointsToGraph(const llvm::Function *F) const {
  if (PointsToGraphs.count(F)) {
    return PointsToGraphs.at(F).get();
  }
  return nullptr;
}

} // namespace psr
