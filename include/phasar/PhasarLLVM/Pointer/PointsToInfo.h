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

#include <iosfwd>
#include <set>

#include "nlohmann/json.hpp"

namespace psr {

enum class AliasResult { NoAlias, MayAlias, PartialAlias, MustAlias };

std::string to_string(AliasResult AR);

AliasResult to_AliasResult(const std::string &S);

std::ostream &operator<<(std::ostream &OS, const AliasResult &AR);

template <typename V, typename N> class PointsToInfo {
public:
  virtual ~PointsToInfo() = default;

  virtual AliasResult alias(V V1, V V2, N I = N{}) = 0;

  virtual std::set<V> getPointsToSet(V V1, N I = N{}) const = 0;

  virtual void print(std::ostream &OS) const = 0;

  virtual nlohmann::json getAsJson() const = 0;

  virtual void printAsJson(std::ostream &OS) const = 0;
};

} // namespace psr

#endif
