/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * SolverConfiguration.h
 *
 *  Created on: 16.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_DATAFLOW_IFDSIDE_IFDSIDESOLVERCONFIG_H
#define PHASAR_DATAFLOW_IFDSIDE_IFDSIDESOLVERCONFIG_H

#include "phasar/Utils/EnumFlags.h"

#include <cstdint>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

enum class SolverConfigOptions : uint32_t {
  None = 0,
  FollowReturnsPastSeeds = 1,
  AutoAddZero = 2,
  ComputeValues = 4,
  RecordEdges = 8,
  EmitESG = 16,
  ComputePersistedSummaries = 32,

  All = ~0U
};

/// \brief Configuration options for the solving process of IFDS/IDE problems
class IFDSIDESolverConfig {
public:
  constexpr IFDSIDESolverConfig() noexcept = default;
  constexpr IFDSIDESolverConfig(SolverConfigOptions Options) noexcept
      : Options(Options) {}

  /// Returns whether the solver should handle unbalanced returns (default:
  /// false)
  [[nodiscard]] constexpr bool followReturnsPastSeeds() const noexcept {
    return hasFlag(Options, SolverConfigOptions::FollowReturnsPastSeeds);
  }
  /// Returns whether the solver should automatically insert an identityFlow
  /// propagation for the special zero value (default: true)
  [[nodiscard]] constexpr bool autoAddZero() const noexcept {
    return hasFlag(Options, SolverConfigOptions::AutoAddZero);
  }
  /// Returns whether the IDE solver should perform IDE's phase 2 (default:
  /// true). You may want to turn this off for IFDS analyses.
  [[nodiscard]] constexpr bool computeValues() const noexcept {
    return hasFlag(Options, SolverConfigOptions::ComputeValues);
  }
  /// Returns, whether the solver should record all ESG edges (default: false)
  /// \note This option may severly hurt the solver's performance
  [[nodiscard]] constexpr bool recordEdges() const noexcept {
    return hasFlag(Options, SolverConfigOptions::RecordEdges);
  }
  /// Returns, whether the solver should emit the ESG as DOT graph on the
  /// command-line (default: false)
  [[nodiscard]] constexpr bool emitESG() const noexcept {
    return hasFlag(Options, SolverConfigOptions::EmitESG);
  }
  /// Currently unused
  [[nodiscard]] constexpr bool computePersistedSummaries() const noexcept {
    return hasFlag(Options, SolverConfigOptions::ComputePersistedSummaries);
  }

  /// \see followReturnsPastSeeds
  constexpr void setFollowReturnsPastSeeds(bool Set = true) {
    setFlag(Options, SolverConfigOptions::FollowReturnsPastSeeds, Set);
  }
  /// \see autoAddZero
  constexpr void setAutoAddZero(bool Set = true) {
    setFlag(Options, SolverConfigOptions::AutoAddZero, Set);
  }
  /// \see computeValues
  constexpr void setComputeValues(bool Set = true) {
    setFlag(Options, SolverConfigOptions::ComputeValues, Set);
  }
  /// \see recordEdges
  constexpr void setRecordEdges(bool Set = true) {
    setFlag(Options, SolverConfigOptions::RecordEdges, Set);
  }
  /// \see emitESG
  constexpr void setEmitESG(bool Set = true) {
    setFlag(Options, SolverConfigOptions::EmitESG, Set);
  }
  /// \see computePersistedSummaries
  constexpr void setComputePersistedSummaries(bool Set = true) {
    setFlag(Options, SolverConfigOptions::ComputePersistedSummaries, Set);
  }

  constexpr void setConfig(SolverConfigOptions Opt) { Options = Opt; }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       IFDSIDESolverConfig SC);

private:
  SolverConfigOptions Options =
      SolverConfigOptions::AutoAddZero | SolverConfigOptions::ComputeValues;
};

} // namespace psr

#endif
