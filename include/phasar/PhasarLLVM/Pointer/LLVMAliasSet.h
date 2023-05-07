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

#include "phasar/PhasarLLVM/Pointer/LLVMBasedAliasAnalysis.h"
#include "phasar/Pointer/AliasInfoBase.h"
#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/AliasSetOwner.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/StableVector.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

#include "nlohmann/json.hpp"

#include <utility>

namespace llvm {
class Value;
class Instruction;
class GlobalVariable;
class Function;
} // namespace llvm

namespace psr {

class LLVMAliasSet;
class LLVMProjectIRDB;

template <>
struct AliasInfoTraits<LLVMAliasSet>
    : DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {};

class LLVMAliasSet : public AnalysisPropertiesMixin<LLVMAliasSet>,
                     public AliasInfoBaseUtils {

public:
  using traits_t = AliasInfoTraits<LLVMAliasSet>;
  using n_t = traits_t::n_t;
  using v_t = traits_t::v_t;
  using AliasSetTy = traits_t::AliasSetTy;
  using AliasSetPtrTy = traits_t::AliasSetPtrTy;
  using AllocationSiteSetPtrTy = traits_t::AllocationSiteSetPtrTy;
  using AliasSetMap = llvm::DenseMap<const llvm::Value *, BoxedPtr<AliasSetTy>>;

  /**
   * Creates points-to set(s) for all functions in the IRDB. If
   * UseLazyEvaluation is true, computes points-to-sets for functions that do
   * not use global variables on the fly
   */
  explicit LLVMAliasSet(LLVMProjectIRDB *IRDB, bool UseLazyEvaluation = true,
                        AliasAnalysisType PATy = AliasAnalysisType::CFLAnders);

  explicit LLVMAliasSet(LLVMProjectIRDB *IRDB,
                        const nlohmann::json &SerializedPTS);

  [[nodiscard]] inline bool isInterProcedural() const noexcept {
    return false;
  };

  [[nodiscard]] inline AliasAnalysisType getAliasAnalysisType() const noexcept {
    return PTA.getPointerAnalysisType();
  };

  [[nodiscard]] AliasResult alias(const llvm::Value *V1, const llvm::Value *V2,
                                  const llvm::Instruction *I = nullptr);

  [[nodiscard]] AliasSetPtrTy getAliasSet(const llvm::Value *V,
                                          const llvm::Instruction *I = nullptr);

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSites(const llvm::Value *V, bool IntraProcOnly = false,
                              const llvm::Instruction *I = nullptr);

  // Checks if PotentialValue is in the reachable allocation sites of V.
  [[nodiscard]] bool isInReachableAllocationSites(
      const llvm::Value *V, const llvm::Value *PotentialValue,
      bool IntraProcOnly = false, const llvm::Instruction *I = nullptr);

  void mergeWith(const LLVMAliasSet &OtherPTI);

  void introduceAlias(const llvm::Value *V1, const llvm::Value *V2,
                      const llvm::Instruction *I = nullptr,
                      AliasResult Kind = AliasResult::MustAlias);

  void print(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] nlohmann::json getAsJson() const;

  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] AnalysisProperties getAnalysisProperties() const noexcept {
    return AnalysisProperties::None;
  }

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
  void computeValuesAliasSet(const llvm::Value *V);

  void computeFunctionsAliasSet(llvm::Function *F);

  void addSingletonAliasSet(const llvm::Value *V);

  void mergeAliasSets(const llvm::Value *V1, const llvm::Value *V2);

  void mergeAliasSets(BoxedPtr<AliasSetTy> PTS1, BoxedPtr<AliasSetTy> PTS2);

  bool interIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P) const;

  bool intraIsReachableAllocationSiteTy(const llvm::Value *V,
                                        const llvm::Value *P,
                                        const llvm::Function *VFun,
                                        const llvm::GlobalObject *VG) const;

  /// Utility function used by computeFunctionsAliasSet(...)
  void addPointer(llvm::AAResults &AA, const llvm::DataLayout &DL,
                  const llvm::Value *V, std::vector<const llvm::Value *> &Reps);

  [[nodiscard]] static BoxedPtr<AliasSetTy> getEmptyAliasSet();

  LLVMBasedAliasAnalysis PTA;
  llvm::DenseSet<const llvm::Function *> AnalyzedFunctions;

  AliasSetOwner<AliasSetTy>::memory_resource_type MRes;
  AliasSetOwner<AliasSetTy> Owner{&MRes};

  AliasSetMap AliasSets;
};

static_assert(IsAliasInfo<LLVMAliasSet>);

} // namespace psr

#endif
