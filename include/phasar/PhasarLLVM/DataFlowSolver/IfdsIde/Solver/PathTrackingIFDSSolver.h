/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PathTrackingIFDSSolver.h
 *
 *  Created on: 23.11.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_PATHTRACKINGIFDSSOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_PATHTRACKINGIFDSSOLVER_H_

namespace psr {

/**
 * An {@link IFDSSolver} that tracks paths for reporting. To do so, it requires
 * that data-flow abstractions implement the LinkedNode interface.
 * The solver implements a cache of data-flow facts for each statement and
 * source value. If for the same statement and source value the same
 * target value is seen again (as determined through a cache hit), then the
 * solver propagates the cached value but at the same time links
 * both target values with one another.
 * @deprecated Use {@link JoinHandlingNodesIFDSSolver} instead.
 */
template <typename N, typename D, typename F, typename I>
class PathTrackingIFDSSolver : public IFDSSolver<N, D, F, I> {
  //
  //	public PathTrackingIFDSSolver(IFDSTabulationProblem<N, D, F, I>
  // ifdsProblem) {
  //		super(ifdsProblem);
  //	}
  //
  //	protected final Map<CacheEntry, LinkedNode<D>> cache =
  // Maps.newHashMap();

  //	@Override
  //	protected void propagate(D sourceVal, N target, D targetVal,
  // EdgeFunction<IFDSSolver.BinaryDomain> f, N relatedCallSite, boolean
  // isUnbalancedReturn) {
  //		CacheEntry currentCacheEntry = new CacheEntry(target, sourceVal,
  // targetVal);
  //
  //		boolean propagate = false;
  //		synchronized (this) {
  //			if (cache.containsKey(currentCacheEntry)) {
  //				LinkedNode<D> existingTargetVal =
  // cache.get(currentCacheEntry);
  //				if (existingTargetVal != targetVal)
  //					existingTargetVal.addNeighbor(targetVal);
  //			} else {
  //				cache.put(currentCacheEntry, targetVal);
  //				propagate = true;
  //			}
  //		}
  //
  //		if (propagate)
  //			super.propagate(sourceVal, target, targetVal, f,
  // relatedCallSite, isUnbalancedReturn);
  //
  //	};
};

} // namespace psr

#endif
