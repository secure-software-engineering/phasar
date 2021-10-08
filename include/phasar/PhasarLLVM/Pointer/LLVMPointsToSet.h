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
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "nlohmann/json.hpp"

#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Pointer/PointsToSetOwner.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/FormatVariadic.h"

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
  using PointsToSetMap = llvm::DenseMap<const llvm::Value *, PointsToSetTy *>;

  LLVMBasedPointsToAnalysis PTA;
  llvm::DenseSet<const llvm::Function *> AnalyzedFunctions;

  PointsToSetOwner<PointsToSetTy> Owner;
  PointsToSetMap PointsToSets;

  void computeValuesPointsToSet(const llvm::Value *V);

  void computeFunctionsPointsToSet(llvm::Function *F);

  PointsToSetPtrTy addSingletonPointsToSet(const llvm::Value *V);

  void mergePointsToSets(const llvm::Value *V1, const llvm::Value *V2);

  PointsToSetTy *mergePointsToSets(PointsToSetTy *PTS1, PointsToSetTy *PTS2);

  bool interIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P);

  bool intraIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P,
                                        const llvm::Function *VFun,
                                        const llvm::GlobalObject *VG);

  /// Utility function used by computeFunctionsPointsToSet(...)
  void addPointer(llvm::AAResults &AA, const llvm::DataLayout &DL,
                  const llvm::Value *V, std::vector<const llvm::Value *> &Reps);

  [[nodiscard]] static PointsToSetPtrTy getEmptyPointsToSet();

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

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSet(const llvm::Value *V,
                 const llvm::Instruction *I = nullptr) override;

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSites(const llvm::Value *V, bool IntraProcOnly = false,
                              const llvm::Instruction *I = nullptr) override;

  // Checks if PotentialValue is in the reachable allocation sites of V.
  [[nodiscard]] bool
  isInReachableAllocationSites(const llvm::Value *V,
                               const llvm::Value *PotentialValue,
                               bool IntraProcOnly = false,
                               const llvm::Instruction *I = nullptr) override;

  void mergeWith(const PointsToInfo &PTI) override;

  void introduceAlias(const llvm::Value *V1, const llvm::Value *V2,
                      const llvm::Instruction *I = nullptr,
                      AliasResult Kind = AliasResult::MustAlias) override;

  [[nodiscard]] inline bool empty() const { return AnalyzedFunctions.empty(); }

  void print(std::ostream &OS = std::cout) const override;

  [[nodiscard]] nlohmann::json getAsJson() const override;

  void printAsJson(std::ostream &OS = std::cout) const override;

  /**
   * Shows a parts of an alias set. Good for debugging when one wants to peak
   * into a points to set.
   *
   * @param ValueSetPair a pair on an Value* and the corresponding points to set
   * @param Peak the amount of instrutions shown from the points to set
   */
  static void
  peakIntoPointsToSet(const PointsToSetMap::value_type &ValueSetPair, int Peak);

  /**
   * Prints out the size distribution for all points to sets.
   *
   * @param Peak the amount of instrutions shown from one of the biggest points
   * to sets, use 0 show nothing.
   */
  void drawPointsToSetsDistribution(int Peak = 10) const;
};

} // namespace psr

#endif
