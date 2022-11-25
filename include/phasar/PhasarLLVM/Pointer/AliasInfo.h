/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_ALIASINFO_H_
#define PHASAR_PHASARLLVM_POINTER_ALIASINFO_H_

#include "phasar/PhasarLLVM/Pointer/AliasAnalysisType.h"
#include "phasar/PhasarLLVM/Pointer/DynamicAliasSetPtr.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

namespace psr {

enum class AliasResult { NoAlias, MayAlias, PartialAlias, MustAlias };

std::string toString(AliasResult AR);

AliasResult toAliasResult(const std::string &S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const AliasResult &AR);

template <typename V, typename N> class AliasInfo {
public:
  using AliasSetTy = llvm::DenseSet<V>;
  using AliasSetPtrTy = DynamicAliasSetConstPtr<AliasSetTy>;
  using AllocationSiteSetPtrTy = std::unique_ptr<AliasSetTy>;

  virtual ~AliasInfo() = default;

  [[nodiscard]] virtual bool isInterProcedural() const = 0;

  [[nodiscard]] virtual AliasAnalysisType getAliasAnalysisType() const = 0;

  [[nodiscard]] virtual AliasResult alias(V V1, V V2, N I = N{}) = 0;

  [[nodiscard]] virtual AliasSetPtrTy getAliasSet(V V1, N I = N{}) = 0;

  [[nodiscard]] virtual AllocationSiteSetPtrTy
  getReachableAllocationSites(V V1, bool IntraProcOnly = false, N I = N{}) = 0;

  // Checks if V2 is a reachable allocation in the points to set of V1.
  [[nodiscard]] virtual bool
  isInReachableAllocationSites(V V1, V V2, bool IntraProcOnly = false,
                               N I = N{}) = 0;

  virtual void print(llvm::raw_ostream &OS = llvm::outs()) const = 0;

  [[nodiscard]] virtual nlohmann::json getAsJson() const = 0;

  virtual void printAsJson(llvm::raw_ostream &OS) const = 0;

  // The following functions are relevent when combining points-to with other
  // pieces of information. For instance, during a call-graph construction (or
  // a data-flow analysis) points-to information may be altered to incorporate
  // novel information.
  virtual void mergeWith(const AliasInfo &PTI) = 0;

  virtual void introduceAlias(V V1, V V2, N I = N{},
                              AliasResult Kind = AliasResult::MustAlias) = 0;
};

template <typename V, typename N>
static inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                            const AliasInfo<V, N> &PTI) {
  PTI.print(OS);
  return OS;
}

} // namespace psr

#endif
