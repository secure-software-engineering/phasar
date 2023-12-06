/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONSTATS_H
#define PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONSTATS_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <cstddef>
#include <numeric>

namespace psr {
enum class EdgeFunctionKind;
enum class EdgeFunctionAllocationPolicy;

namespace detail {
struct EdgeFunctionStatsData {
  static constexpr size_t NumEFKinds = 5;
  static constexpr size_t NumAllocPolicies = 3;

  std::array<size_t, NumEFKinds> UniqueEFCount{};
  std::array<size_t, NumEFKinds> TotalEFCount{};
  std::array<size_t, NumAllocPolicies> PerAllocCount{};
};
} // namespace detail

class EdgeFunctionStats : public detail::EdgeFunctionStatsData {
public:
  [[nodiscard]] size_t getNumUniqueEFs() const noexcept {
    return std::reduce(UniqueEFCount.begin(), UniqueEFCount.end());
  }
  [[nodiscard]] size_t getNumUniqueEFs(EdgeFunctionKind Kind) const noexcept {
    assert(size_t(Kind) < NumEFKinds);
    return UniqueEFCount[size_t(Kind)]; // NOLINT
  }

  [[nodiscard]] size_t getNumEFs() const noexcept {
    return std::reduce(TotalEFCount.begin(), TotalEFCount.end());
  }
  [[nodiscard]] size_t getNumEFs(EdgeFunctionKind Kind) const noexcept {
    assert(size_t(Kind) < NumEFKinds);
    return TotalEFCount[size_t(Kind)]; // NOLINT
  }

  [[nodiscard]] size_t getNumEFsPerAllocationPolicy(
      EdgeFunctionAllocationPolicy Policy) const noexcept {
    assert(size_t(Policy) < NumAllocPolicies);
    return PerAllocCount[size_t(Policy)]; // NOLINT
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const EdgeFunctionStats &S);

private:
  template <typename AnalysisDomainTy, typename Container>
  friend class IDESolver;

  constexpr EdgeFunctionStats(
      const detail::EdgeFunctionStatsData &Data) noexcept
      : detail::EdgeFunctionStatsData(Data) {}
};
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONSTATS_H
