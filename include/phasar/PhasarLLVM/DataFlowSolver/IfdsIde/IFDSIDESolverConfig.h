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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVERCONFIGURATION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVERCONFIGURATION_H_

#include <iosfwd>

#include "phasar/Config/Configuration.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

namespace psr {

enum class SolverConfigOptions : uint32_t {
  None = 0,
  FollowReturnsPastSeeds = 1,
  AutoAddZero = 2,
  ComputeValues = 4,
  RecordEdges = 8,
  EmitESG = 16,
  ComputePersistedSummaries = 32,

  All = ~0u
};

struct IFDSIDESolverConfig {
  IFDSIDESolverConfig();
  IFDSIDESolverConfig(SolverConfigOptions Options);
  ~IFDSIDESolverConfig() = default;
  IFDSIDESolverConfig(const IFDSIDESolverConfig &) = default;
  IFDSIDESolverConfig &operator=(const IFDSIDESolverConfig &) = default;
  IFDSIDESolverConfig(IFDSIDESolverConfig &&) = default;
  IFDSIDESolverConfig &operator=(IFDSIDESolverConfig &&) = default;

  bool followReturnsPastSeeds() const;
  bool autoAddZero() const;
  bool computeValues() const;
  bool recordEdges() const;
  bool emitESG() const;
  bool computePersistedSummaries() const;

  void setFollowReturnsPastSeeds(bool Set = true);
  void setAutoAddZero(bool Set = true);
  void setComputeValues(bool Set = true);
  void setRecordEdges(bool Set = true);
  void setEmitESG(bool Set = true);
  void setComputePersistedSummaries(bool Set = true);

  friend std::ostream &operator<<(std::ostream &OS,
                                  const IFDSIDESolverConfig &SC);

private:
  SolverConfigOptions Options = SolverConfigOptions::AutoAddZero |
                                SolverConfigOptions::ComputeValues |
                                SolverConfigOptions::RecordEdges;
};

} // namespace psr

#endif
