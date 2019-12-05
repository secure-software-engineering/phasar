/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLAndersAliasAnalysis.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/Passes/PassBuilder.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>

namespace psr {

LLVMPointsToInfo::LLVMPointsToInfo(ProjectIRDB &IRDB) {
  llvm::PassBuilder PB;
  llvm::AAManager AAM = PB.buildDefaultAAPipeline();
  AAM.registerFunctionAnalysis<llvm::CFLAndersAA>();
  // llvm::PassManager<const llvm::Module &> PM;
  llvm::FunctionAnalysisManager FAM;
  for (const llvm::Module *M : IRDB.getAllModules()) {
    llvm::Module *NCM = const_cast<llvm::Module *>(M);
    for (auto &F : *NCM) {
      if (!F.isDeclaration()) {
        llvm::AAResults AAR = AAM.run(F, FAM);
        PointsToGraphs.insert(std::make_pair(
            &F, std::make_unique<PointsToGraph>(std::move(AAR), &F)));
      } else {
        PointsToGraphs.insert(std::make_pair(&F, nullptr));
      }
    }
  }
}

AliasResult LLVMPointsToInfo::alias(const llvm::Value *V1,
                                    const llvm::Value *V2) const {
  return AliasResult::MayAlias;
}

std::set<const llvm::Value *>
LLVMPointsToInfo::getPointsToSet(const llvm::Value *V1) const {
  return {};
}

nlohmann::json LLVMPointsToInfo::getAsJson() const { return ""_json; }

PointsToGraph *
LLVMPointsToInfo::getPointsToGraph(const llvm::Function *F) const {
  return PointsToGraphs.at(F).get();
}

} // namespace psr
