/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOSET_H
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOSET_H

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/DynamicPointsToSetPtr.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Pointer/PointsToSetOwner.h"
#include "phasar/Utils/StableVector.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

#include "nlohmann/json.hpp"

#include <memory_resource>

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
  using PointsToSetMap =
      llvm::DenseMap<const llvm::Value *, DynamicPointsToSetPtr<PointsToSetTy>>;

  LLVMBasedPointsToAnalysis PTA;
  llvm::DenseSet<const llvm::Function *> AnalyzedFunctions;

  PointsToSetOwner<PointsToSetTy>::memory_resource_type MRes;
  PointsToSetOwner<PointsToSetTy> Owner{&MRes};

  PointsToSetMap PointsToSets;

  void computeValuesPointsToSet(const llvm::Value *V);

  void computeFunctionsPointsToSet(llvm::Function *F);

  void addSingletonPointsToSet(const llvm::Value *V);

  void mergePointsToSets(const llvm::Value *V1, const llvm::Value *V2);

  void mergePointsToSets(DynamicPointsToSetPtr<PointsToSetTy> PTS1,
                         DynamicPointsToSetPtr<PointsToSetTy> PTS2);

  bool interIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P);

  bool intraIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P,
                                        const llvm::Function *VFun,
                                        const llvm::GlobalObject *VG);

  /// Utility function used by computeFunctionsPointsToSet(...)
  void addPointer(llvm::AAResults &AA, const llvm::DataLayout &DL,
                  const llvm::Value *V, std::vector<const llvm::Value *> &Reps);

  [[nodiscard]] static DynamicPointsToSetPtr<PointsToSetTy>
  getEmptyPointsToSet();

public:
  /**
   * Creates points-to set(s) for all functions in the IRDB. If
   * UseLazyEvaluation is true, computes points-to-sets for functions that do
   * not use global variables on the fly
   */
  explicit LLVMPointsToSet(
      ProjectIRDB &IRDB, bool UseLazyEvaluation = true,
      PointerAnalysisType PATy = PointerAnalysisType::CFLAnders);

  explicit LLVMPointsToSet(ProjectIRDB &IRDB,
                           const nlohmann::json &SerializedPTS);

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

  void print(llvm::raw_ostream &OS = llvm::outs()) const override;

  [[nodiscard]] nlohmann::json getAsJson() const override;

  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const override;

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
