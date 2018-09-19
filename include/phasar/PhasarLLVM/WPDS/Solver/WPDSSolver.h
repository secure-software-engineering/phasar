/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_SOLVER_WPDSSOLVER_H_
#define PHASAR_PHASARLLVM_WPDS_SOLVER_WPDSSOLVER_H_

namespace psr {

class WPDSProblem;

template <typename N, typename D, typename M, typename V, typename I>
class WPDSSolver {
private:
  WPDSProblem &P;

public:
  WPDSSolver(WPDSProblem &P) : P(P) {}
};

} // namespace psr

#endif
