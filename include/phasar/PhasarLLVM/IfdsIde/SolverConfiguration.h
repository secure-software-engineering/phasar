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

namespace psr {

struct SolverConfiguration {
  SolverConfiguration() = default;
  SolverConfiguration(bool followReturnsPastSeeds, bool autoAddZero,
                      bool computeValues, bool recordEdges,
                      bool computePersistedSummaries)
      : followReturnsPastSeeds(followReturnsPastSeeds),
        autoAddZero(autoAddZero), computeValues(computeValues),
        recordEdges(recordEdges),
        computePersistedSummaries(computePersistedSummaries) {}
  ~SolverConfiguration() = default;
  SolverConfiguration(const SolverConfiguration &) = default;
  SolverConfiguration &operator=(const SolverConfiguration &) = default;
  SolverConfiguration(SolverConfiguration &&) = default;
  SolverConfiguration &operator=(SolverConfiguration &&) = default;
  bool followReturnsPastSeeds = false;
  bool autoAddZero = false;
  bool computeValues = false;
  bool recordEdges = false;
  bool computePersistedSummaries = false;
  friend std::ostream &operator<<(std::ostream &os,
                                  const SolverConfiguration &sc);
};

} // namespace psr

#endif
