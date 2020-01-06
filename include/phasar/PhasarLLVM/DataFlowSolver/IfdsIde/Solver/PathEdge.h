/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PathEdge.h
 *
 *  Created on: 16.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_PATHEDGE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_PATHEDGE_H_

#include <ostream>

namespace psr {

template <typename N, typename D> class PathEdge {
private:
  N target;
  D dSource;
  D dTarget;

public:
  PathEdge(D dSource, N target, D dTarget)
      : target(target), dSource(dSource), dTarget(dTarget) {}

  ~PathEdge() = default;

  PathEdge(const PathEdge &) = default;

  PathEdge &operator=(const PathEdge &) = default;

  PathEdge(PathEdge &&) = default;

  PathEdge &operator=(PathEdge &&) = default;

  N getTarget() { return target; }

  D factAtSource() { return dSource; }

  D factAtTarget() { return dTarget; }

  friend std::ostream &operator<<(std::ostream &os, const PathEdge &pathEdge) {
    return os << "<" << pathEdge.dSource << "> -> <" << pathEdge.target << ","
              << pathEdge.dTarget << ">";
  }
};

} // namespace psr

#endif
