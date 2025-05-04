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

struct IFDSIDESolverConfig {
  IFDSIDESolverConfig() noexcept = default;
  IFDSIDESolverConfig(SolverConfigOptions Options) noexcept;

  [[nodiscard]] bool followReturnsPastSeeds() const;
  [[nodiscard]] bool autoAddZero() const;
  [[nodiscard]] bool computeValues() const;
  [[nodiscard]] bool recordEdges() const;
  [[nodiscard]] bool emitESG() const;
  [[nodiscard]] bool computePersistedSummaries() const;

  void setFollowReturnsPastSeeds(bool Set = true);
  void setAutoAddZero(bool Set = true);
  void setComputeValues(bool Set = true);
  void setRecordEdges(bool Set = true);
  void setEmitESG(bool Set = true);
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
