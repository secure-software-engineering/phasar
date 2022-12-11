/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMALIASSET_H
#define PHASAR_PHASARLLVM_POINTER_LLVMALIASSET_H

#include "phasar/PhasarLLVM/Pointer/AliasInfoBase.h"
#include "phasar/PhasarLLVM/Pointer/AliasSetOwner.h"
#include "phasar/PhasarLLVM/Pointer/DynamicAliasSetPtr.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedAliasAnalysis.h"
#include "phasar/Utils/StableVector.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

#include "nlohmann/json.hpp"

namespace llvm {
class Value;
class Instruction;
class GlobalVariable;
class Function;
} // namespace llvm

namespace psr {

class LLVMAliasSet;
class ProjectIRDB;

template <>
struct AliasInfoTraits<LLVMAliasSet>
    : DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {};

class LLVMAliasSet : public AliasInfoBase<LLVMAliasSet> {
  friend AliasInfoBase;

private:
  using AliasSetMap =
      llvm::DenseMap<const llvm::Value *, DynamicAliasSetPtr<AliasSetTy>>;

  LLVMBasedAliasAnalysis PTA;
  llvm::DenseSet<const llvm::Function *> AnalyzedFunctions;

  AliasSetOwner<AliasSetTy>::memory_resource_type MRes;
  AliasSetOwner<AliasSetTy> Owner{&MRes};

  AliasSetMap AliasSets;

  void computeValuesAliasSet(const llvm::Value *V);

  void computeFunctionsAliasSet(llvm::Function *F);

  void addSingletonAliasSet(const llvm::Value *V);

  void mergeAliasSets(const llvm::Value *V1, const llvm::Value *V2);

  void mergeAliasSets(DynamicAliasSetPtr<AliasSetTy> PTS1,
                      DynamicAliasSetPtr<AliasSetTy> PTS2);

  bool interIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P) const;

  bool intraIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P,
                                        const llvm::Function *VFun,
                                        const llvm::GlobalObject *VG) const;

  /// Utility function used by computeFunctionsAliasSet(...)
  void addPointer(llvm::AAResults &AA, const llvm::DataLayout &DL,
                  const llvm::Value *V, std::vector<const llvm::Value *> &Reps);

  [[nodiscard]] static DynamicAliasSetPtr<AliasSetTy> getEmptyAliasSet();

public:
  /**
   * Creates points-to set(s) for all functions in the IRDB. If
   * UseLazyEvaluation is true, computes points-to-sets for functions that do
   * not use global variables on the fly
   */
  explicit LLVMAliasSet(ProjectIRDB &IRDB, bool UseLazyEvaluation = true,
                        AliasAnalysisType PATy = AliasAnalysisType::CFLAnders);

  explicit LLVMAliasSet(ProjectIRDB &IRDB, const nlohmann::json &SerializedPTS);

  /**
   * Shows a parts of an alias set. Good for debugging when one wants to peak
   * into a points to set.
   *
   * @param ValueSetPair a pair on an Value* and the corresponding points to set
   * @param Peak the amount of instrutions shown from the points to set
   */
  static void peakIntoAliasSet(const AliasSetMap::value_type &ValueSetPair,
                               int Peak);

  /**
   * Prints out the size distribution for all points to sets.
   *
   * @param Peak the amount of instrutions shown from one of the biggest points
   * to sets, use 0 show nothing.
   */
  void drawAliasSetsDistribution(int Peak = 10) const;

  [[nodiscard]] inline bool empty() const { return AnalyzedFunctions.empty(); }

private:
  [[nodiscard]] inline bool isInterProceduralImpl() const noexcept {
    return false;
  };

  [[nodiscard]] inline AliasAnalysisType
  getAliasAnalysisTypeImpl() const noexcept {
    return PTA.getPointerAnalysisType();
  };

  [[nodiscard]] AliasResult aliasImpl(const llvm::Value *V1,
                                      const llvm::Value *V2,
                                      const llvm::Instruction *I = nullptr);

  [[nodiscard]] AliasSetPtrTy
  getAliasSetImpl(const llvm::Value *V, const llvm::Instruction *I = nullptr);

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSitesImpl(const llvm::Value *V,
                                  bool IntraProcOnly = false,
                                  const llvm::Instruction *I = nullptr);

  // Checks if PotentialValue is in the reachable allocation sites of V.
  [[nodiscard]] bool isInReachableAllocationSitesImpl(
      const llvm::Value *V, const llvm::Value *PotentialValue,
      bool IntraProcOnly = false, const llvm::Instruction *I = nullptr);

  void mergeWithImpl(const LLVMAliasSet &OtherPTI);

  void introduceAliasImpl(const llvm::Value *V1, const llvm::Value *V2,
                          const llvm::Instruction *I = nullptr,
                          AliasResult Kind = AliasResult::MustAlias);

  void printImpl(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] nlohmann::json getAsJsonImpl() const;

  void printAsJsonImpl(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] AnalysisProperties getAnalysisPropertiesImpl() const noexcept {
    return AnalysisProperties::None;
  }
};

extern template class AliasInfoBase<LLVMAliasSet>;

} // namespace psr

#endif
