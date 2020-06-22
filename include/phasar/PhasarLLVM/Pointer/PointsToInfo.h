/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H_
#define PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H_

#include <iostream>
#include <memory>
#include <unordered_set>

#include "nlohmann/json.hpp"

namespace psr {

enum class AliasResult { NoAlias, MayAlias, PartialAlias, MustAlias };

std::string toString(AliasResult AR);

AliasResult toAliasResult(const std::string &S);

std::ostream &operator<<(std::ostream &OS, const AliasResult &AR);

enum class PointerAnalysisType {
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  Invalid
};

std::string toString(const PointerAnalysisType &PA);

PointerAnalysisType toPointerAnalysisType(const std::string &S);

std::ostream &operator<<(std::ostream &os, const PointerAnalysisType &PA);

template <typename V, typename N> class PointsToInfo {
public:
  virtual ~PointsToInfo() = default;

  virtual bool isInterProcedural() const = 0;

  virtual PointerAnalysisType getPointerAnalysistype() const = 0;

  virtual AliasResult alias(V V1, V V2, N I = N{}) = 0;

  virtual std::shared_ptr<std::unordered_set<V>> getPointsToSet(V V1,
                                                                N I = N{}) = 0;

  virtual std::unordered_set<V> getReachableAllocationSites(V V1,
                                                            N I = N{}) = 0;

  virtual void print(std::ostream &OS = std::cout) const = 0;

  virtual nlohmann::json getAsJson() const = 0;

  virtual void printAsJson(std::ostream &OS) const = 0;

  // The following functions are relevent when combining points-to with other
  // pieces of information. For instance, during a call-graph construction (or
  // a data-flow analysis) points-to information may be altered to incorporate
  // novel information.
  virtual void mergeWith(const PointsToInfo &PTI) = 0;

  virtual void introduceAlias(V V1, V V2, N I = N{},
                              AliasResult Kind = AliasResult::MustAlias) = 0;
};

template <typename V, typename N>
static inline std::ostream &operator<<(std::ostream &OS,
                                       const PointsToInfo<V, N> &PTI) {
  PTI.print(OS);
  return OS;
}

} // namespace psr

#endif
