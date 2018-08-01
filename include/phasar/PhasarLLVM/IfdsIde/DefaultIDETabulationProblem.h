/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DefaultIDETabulationProblem.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_DEFAULTIDETABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_DEFAULTIDETABULATIONPROBLEM_H_

#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class DefaultIDETabulationProblem : public IDETabulationProblem<N, D, M, V, I> {
protected:
  I icfg;
  virtual D createZeroValue() = 0;
  D zerovalue;

public:
  DefaultIDETabulationProblem(I icfg) : icfg(icfg) {
    this->solver_config.followReturnsPastSeeds = false;
    this->solver_config.autoAddZero = true;
    this->solver_config.computeValues = true;
    this->solver_config.recordEdges = true;
    this->solver_config.computePersistedSummaries = true;
  }

  virtual ~DefaultIDETabulationProblem() = default;

  I interproceduralCFG() override { return icfg; }

  D zeroValue() override { return zerovalue; }
};

} // namespace psr

#endif
