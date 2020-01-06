/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * JoinHandlingNode.h
 *
 *  Created on: 23.11.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JOINHANDLINGNODE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JOINHANDLINGNODE_H_

#include <vector>

namespace psr {

template <typename T> class JoinHandlingNode {
public:
  virtual ~JoinHandlingNode();
  /**
   *
   * @param joiningNode the node abstraction that was propagated to the same
   * target after {@code this} node.
   * @return true if the join could be handled and no further propagation of the
   * {@code joiningNode} is necessary, otherwise false meaning
   * the node should be propagated by the solver.
   */
  virtual bool handleJoin(T joiningNode) = 0;

  class JoinKey {
  private:
    std::vector<T> elements;

  public:
    /**
     *
     * @param elements Passed elements must be immutable with respect to their
     * hashCode and equals implementations.
     */
    JoinKey(std::vector<T> elems) : elements(elems) {}
    int hash() { return 0; }
    bool equals() { return false; }
  };

  /**
   *
   * @return a JoinKey object used to identify which node abstractions require
   * manual join handling.
   * For nodes with {@code equal} JoinKey instances {@link
   * #handleJoin(JoinHandlingNode)} will be called.
   */
  virtual JoinKey createJoinKey() = 0;
};

} // namespace psr

#endif
