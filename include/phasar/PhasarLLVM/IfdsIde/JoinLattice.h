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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_JOINLATTICE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_JOINLATTICE_H_

namespace psr {

template <typename V> class JoinLattice {
public:
  virtual ~JoinLattice() = default;
  virtual V topElement() = 0;
  virtual V bottomElement() = 0;
  virtual V join(V lhs, V rhs) = 0;
};
} // namespace psr

#endif
