/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * BiDiICFG.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_BIDIICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_BIDIICFG_H_

#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"

namespace psr {

template <typename N, typename F> class BiDiICFG : public ICFG<N, F> {
public:
  virtual ~BiDiICFG() = default;

  virtual std::vector<N> getPredsOf(N u) = 0;

  virtual std::set<N> getEndPointsOf(F f) = 0;

  virtual std::vector<N> getPredsOfCallAt(N u) = 0;

  virtual std::set<N> allNonCallEndNodes() = 0;

  // also exposed to some clients who need it
  // virtual DirectedGraph<N> getOrCreateUnitGraph(F body) = 0;

  virtual std::vector<N> getParameterRefs(F f) = 0;

  /**
   * Gets whether the given statement is a return site of at least one call
   * @param n The statement to check
   * @return True if the given statement is a return site, otherwise false
   */
  virtual bool isReturnSite(N n) = 0;

  /**
   * Checks whether the given statement is reachable from the entry point
   * @param u The statement to check
   * @return True if there is a control flow path from the entry point of the
   * program to the given statement, otherwise false
   */
  virtual bool isReachable(N u) = 0;
};

} // namespace psr

#endif
