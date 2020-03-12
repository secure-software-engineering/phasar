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

namespace psr {

struct IFDSIDESolverConfig {
  IFDSIDESolverConfig() = default;
  IFDSIDESolverConfig(bool followReturnsPastSeeds, bool autoAddZero,
                      bool computeValues, bool recordEdges, bool emitESG,
                      bool computePersistedSummaries);
  ~IFDSIDESolverConfig() = default;
  IFDSIDESolverConfig(const IFDSIDESolverConfig &) = default;
  IFDSIDESolverConfig &operator=(const IFDSIDESolverConfig &) = default;
  IFDSIDESolverConfig(IFDSIDESolverConfig &&) = default;
  IFDSIDESolverConfig &operator=(IFDSIDESolverConfig &&) = default;
  bool followReturnsPastSeeds = false;
  bool autoAddZero = true;
  bool computeValues = true;
  bool recordEdges = true;
  bool emitESG =
      (PhasarConfig::VariablesMap().count("emit-esg-as-dot"))
          ? PhasarConfig::VariablesMap()["emit-esg-as-dot"].as<bool>()
          : false;
  bool computePersistedSummaries = false;
  friend std::ostream &operator<<(std::ostream &os,
                                  const IFDSIDESolverConfig &sc);
};

} // namespace psr

#endif
