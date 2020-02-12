/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * JoinHandlingNodeIFDSSolver.h
 *
 *  Created on: 23.11.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JOINHANDLINGNODEIFDSSOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JOINHANDLINGNODEIFDSSOLVER_H_

namespace psr {

/**
 * An {@link IFDSSolver} that tracks paths for reporting. To do so, it requires
 * that data-flow abstractions implement the LinkedNode interface.
 * The solver implements a cache of data-flow facts for each statement and
 * source value. If for the same statement and source value the same
 * target value is seen again (as determined through a cache hit), then the
 * solver propagates the cached value but at the same time links
 * both target values with one another.
 */
template <typename N, typename D, typename F, typename I>
class JoinHandlingNodesIFDSSolver {
  // class JoinHandlingNodesIFDSSolver<N, D extends JoinHandlingNode<D>, F, I
  // extends InterproceduralCFG<N, F>> extends IFDSSolver<N, D, F, I> {
  //
  //	public JoinHandlingNodesIFDSSolver(IFDSTabulationProblem<N, D, F I>
  // ifdsProblem) {
  //		super(ifdsProblem);
  //	}
};

} // namespace psr

#endif
