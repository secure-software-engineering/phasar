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

#include <set>
#include <iosfwd>

#include <json.hpp>

namespace psr {

enum class AliasResult { NoAlias, MayAlias, MustAlias };

std::string to_string(AliasResult AR);

AliasResult to_AliasResult(const std::string &S);

std::ostream &operator<<(std::ostream &OS, const AliasResult& AR);

template <typename V> class PointsToInfo {
public:
  virtual ~PointsToInfo() = default;

  virtual AliasResult alias(V V1, V V2) const = 0;

  virtual std::set<V> getPointsToSet(V V1) const = 0;

  virtual nlohmann::json getAsJson() const = 0;
};

} // namespace psr

#endif
