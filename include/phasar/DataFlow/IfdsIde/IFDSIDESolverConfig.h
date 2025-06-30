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
struct IFDSIDESolverConfig {
  IFDSIDESolverConfig() noexcept = default;
  IFDSIDESolverConfig(SolverConfigOptions Options) noexcept;

  /// Returns whether the solver should handle unbalanced returns (default:
  /// false)
  [[nodiscard]] bool followReturnsPastSeeds() const;
  /// Returns whether the solver should automatically insert an identityFlow
  /// propagation for the special zero value (default: true)
  [[nodiscard]] bool autoAddZero() const;
  /// Returns whether the IDE solver should perform IDE's phase 2 (default:
  /// true). You may want to turn this off for IFDS analyses.
  [[nodiscard]] bool computeValues() const;
  /// Returns, whether the solver should record all ESG edges (default: false)
  /// \note This option may severly hurt the solver's performance
  [[nodiscard]] bool recordEdges() const;
  /// Returns, whether the solver should emit the ESG as DOT graph on the
  /// command-line (default: false)
  [[nodiscard]] bool emitESG() const;
  /// Currently unused
  [[nodiscard]] bool computePersistedSummaries() const;

  /// \see followReturnsPastSeeds
  void setFollowReturnsPastSeeds(bool Set = true);
  /// \see autoAddZero
  void setAutoAddZero(bool Set = true);
  /// \see computeValues
  void setComputeValues(bool Set = true);
  /// \see recordEdges
  void setRecordEdges(bool Set = true);
  /// \see emitESG
  void setEmitESG(bool Set = true);
  /// \see computePersistedSummaries
  void setComputePersistedSummaries(bool Set = true);

  void setConfig(SolverConfigOptions Opt);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const IFDSIDESolverConfig &SC);

private:
  SolverConfigOptions Options =
      SolverConfigOptions::AutoAddZero | SolverConfigOptions::ComputeValues;
};

} // namespace psr

#endif
