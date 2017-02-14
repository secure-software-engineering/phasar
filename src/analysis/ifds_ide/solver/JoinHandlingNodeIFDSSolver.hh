/*
 * JoinHandlingNodeIFDSSolver.hh
 *
 *  Created on: 23.11.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_JOINHANDLINGNODEIFDSSOLVER_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_JOINHANDLINGNODEIFDSSOLVER_HH_


/**
 * An {@link IFDSSolver} that tracks paths for reporting. To do so, it requires that data-flow abstractions implement the LinkedNode interface.
 * The solver implements a cache of data-flow facts for each statement and source value. If for the same statement and source value the same
 * target value is seen again (as determined through a cache hit), then the solver propagates the cached value but at the same time links
 * both target values with one another.
 */
template<class N, class D, class M, class I>
class JoinHandlingNodesIFDSSolver {
//class JoinHandlingNodesIFDSSolver<N, D extends JoinHandlingNode<D>, M, I extends InterproceduralCFG<N, M>> extends IFDSSolver<N, D, M, I> {
//
//	public JoinHandlingNodesIFDSSolver(IFDSTabulationProblem<N, D, M, I> ifdsProblem) {
//		super(ifdsProblem);
//	}


};


#endif /* ANALYSIS_IFDS_IDE_SOLVER_JOINHANDLINGNODEIFDSSOLVER_HH_ */
