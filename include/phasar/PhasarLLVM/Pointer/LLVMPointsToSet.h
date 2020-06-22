/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOSET_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOSET_H_

#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "nlohmann/json.hpp"

#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"

namespace llvm {
class Value;
class Module;
class Instruction;
class AAResults;
class GlobalVariable;
class Function;
class Type;
} // namespace llvm

namespace psr {

class LLVMPointsToSet : public LLVMPointsToInfo {
private:
  LLVMBasedPointsToAnalysis PTA;
  std::unordered_set<const llvm::Function *> AnalyzedFunctions;
  std::unordered_map<const llvm::Value *,
                     std::shared_ptr<std::unordered_set<const llvm::Value *>>>
      PointsToSets;

  void computeValuesPointsToSet(const llvm::Value *V);

  void computeFunctionsPointsToSet(llvm::Function *F);

  void addSingletonPointsToSet(const llvm::Value *V);

  void mergePointsToSets(const llvm::Value *V1, const llvm::Value *V2);

public:
  /**
   * Creates points-to set(s) based on the computed alias results.
   *
   * @brief Creates points-to set(s) for a given function.
   * @param AA Contains the computed Alias Results.
   * @param F Points-to set is created for this particular function.
   * @param onlyConsiderMustAlias True, if only Must Aliases should be
   * considered. False, if May and Must Aliases should be considered.
   */
  LLVMPointsToSet(ProjectIRDB &IRDB, bool UseLazyEvaluation = true,
                  PointerAnalysisType PATy = PointerAnalysisType::CFLAnders);

  ~LLVMPointsToSet() override = default;

  [[nodiscard]] inline bool isInterProcedural() const override {
    return false;
  };

  [[nodiscard]] inline PointerAnalysisType
  getPointerAnalysistype() const override {
    return PTA.getPointerAnalysisType();
  };

  [[nodiscard]] AliasResult
  alias(const llvm::Value *V1, const llvm::Value *V2,
        const llvm::Instruction *I = nullptr) override;

  [[nodiscard]] std::shared_ptr<std::unordered_set<const llvm::Value *>>
  getPointsToSet(const llvm::Value *V,
                 const llvm::Instruction *I = nullptr) override;

  [[nodiscard]] std::unordered_set<const llvm::Value *>
  getReachableAllocationSites(const llvm::Value *V,
                              const llvm::Instruction *I = nullptr) override;

  void mergeWith(const PointsToInfo &PTI) override;

  void introduceAlias(const llvm::Value *V1, const llvm::Value *V2,
                      const llvm::Instruction *I = nullptr,
                      AliasResult Kind = AliasResult::MustAlias) override;

  [[nodiscard]] inline bool empty() const { return AnalyzedFunctions.empty(); }

  void print(std::ostream &OS = std::cout) const override;

  [[nodiscard]] nlohmann::json getAsJson() const override;

  void printAsJson(std::ostream &OS = std::cout) const override;
};

} // namespace psr

#endif
