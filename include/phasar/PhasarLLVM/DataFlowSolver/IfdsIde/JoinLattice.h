/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AbstractJoinLattice.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_JOINLATTICE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_JOINLATTICE_H

namespace psr {

template <typename AnalysisDomainTy> class JoinLattice {
public:
  using l_t = typename AnalysisDomainTy::l_t;

  virtual ~JoinLattice() = default;
  virtual l_t topElement() = 0;
  virtual l_t bottomElement() = 0;
  virtual l_t join(l_t Lhs, l_t Rhs) = 0;
};
} // namespace psr

#endif
