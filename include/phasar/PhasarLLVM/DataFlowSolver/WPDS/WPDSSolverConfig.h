/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_SOLVERCONFIGURATION_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_SOLVERCONFIGURATION_H_

#include <iosfwd>

#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSOptions.h"

namespace psr {

struct WPDSSolverConfig {
  WPDSSolverConfig() = default;
  WPDSSolverConfig(bool recordWitnesses, WPDSSearchDirection searchDirection,
                   WPDSType wpdsty);
  ~WPDSSolverConfig() = default;
  WPDSSolverConfig(const WPDSSolverConfig &) = default;
  WPDSSolverConfig &operator=(const WPDSSolverConfig &) = default;
  WPDSSolverConfig(WPDSSolverConfig &&) = default;
  WPDSSolverConfig &operator=(WPDSSolverConfig &&) = default;
  bool recordWitnesses = false;
  WPDSSearchDirection searchDirection = WPDSSearchDirection::FORWARD;
  WPDSType wpdsty = WPDSType::FWPDS;
  friend std::ostream &operator<<(std::ostream &os, const WPDSSolverConfig &sc);
};

} // namespace psr

#endif
