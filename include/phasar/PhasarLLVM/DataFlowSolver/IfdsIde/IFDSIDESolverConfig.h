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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSIDESOLVERCONFIG_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSIDESOLVERCONFIG_H_

#include <iosfwd>

#include "phasar/Config/Configuration.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/Logger.h"

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
  ~IFDSIDESolverConfig() = default;
  IFDSIDESolverConfig(const IFDSIDESolverConfig &) noexcept = default;
  IFDSIDESolverConfig &
  operator=(const IFDSIDESolverConfig &) noexcept = default;
  IFDSIDESolverConfig(IFDSIDESolverConfig &&) noexcept = default;
  IFDSIDESolverConfig &operator=(IFDSIDESolverConfig &&) noexcept = default;

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

  friend std::ostream &operator<<(std::ostream &OS,
                                  const IFDSIDESolverConfig &SC);

private:
  SolverConfigOptions Options = SolverConfigOptions::AutoAddZero |
                                SolverConfigOptions::ComputeValues |
                                SolverConfigOptions::RecordEdges;
};

} // namespace psr

#endif
