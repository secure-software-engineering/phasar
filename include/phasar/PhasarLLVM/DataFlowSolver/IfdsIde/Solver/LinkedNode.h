/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LinkedNode.h
 *
 *  Created on: 23.11.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_LINKEDNODE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_LINKEDNODE_H

namespace psr {

/**
 * A data-flow fact that can be linked with other equal facts.
 * Equality and hash-code operations must <i>not</i> take the linking data
 * structures into account!
 *
 * @deprecated Use {@link JoinHandlingNode} instead.
 */
template <typename D> class LinkedNode {
public:
  virtual ~LinkedNode() = default;
  /**
   * Links this node to a neighbor node, i.e., to an abstraction that would have
   * been merged
   * with this one of paths were not being tracked.
   */
  virtual void addNeighbor(D OriginalAbstraction) = 0;
  virtual void setCallingContext(D CallingContext) = 0;
};

} // namespace psr

#endif
