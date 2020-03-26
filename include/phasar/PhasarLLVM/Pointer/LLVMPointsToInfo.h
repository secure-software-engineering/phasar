/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H_

#include <map>
#include <memory>
#include <string>

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

#include "nlohmann/json.hpp"

#include "phasar/PhasarLLVM/Pointer/PointsToInfo.h"

namespace llvm {
class Value;
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class PointsToGraph;

enum class PointerAnalysisType {
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  Invalid
};

std::string to_string(const PointerAnalysisType &PA);

PointerAnalysisType to_PointerAnalysisType(const std::string &S);

std::ostream &operator<<(std::ostream &os, const PointerAnalysisType &PA);

class LLVMPointsToInfo
    : public PointsToInfo<const llvm::Value *, const llvm::Instruction *> {
private:
  llvm::PassBuilder PB;
  llvm::FunctionAnalysisManager FAM;
  mutable std::unordered_map<const llvm::Function *, llvm::AAResults *> AAInfos;
  std::map<const llvm::Function *, std::unique_ptr<PointsToGraph>>
      PointsToGraphs;

public:
  LLVMPointsToInfo(ProjectIRDB &IRDB,
                   PointerAnalysisType PAT = PointerAnalysisType::CFLAnders);

  ~LLVMPointsToInfo() override = default;

  AliasResult alias(const llvm::Value *V1, const llvm::Value *V2,
                    const llvm::Instruction *I = nullptr) override;

  std::set<const llvm::Value *>
  getPointsToSet(const llvm::Value *V,
                 const llvm::Instruction *I = nullptr) const override;

  void print(std::ostream &OS = std::cout) const override;

  nlohmann::json getAsJson() const override;

  llvm::AAResults *getAAResults(const llvm::Function *F) const;

  PointsToGraph *getPointsToGraph(const llvm::Function *F) const;
};

} // namespace psr

#endif
