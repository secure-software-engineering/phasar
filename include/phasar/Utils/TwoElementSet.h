/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * TwoElementSet.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_UTILS_TWOELEMENTSET_H_
#define PHASAR_UTILS_TWOELEMENTSET_H_

#include <cstddef>

namespace psr {

template <typename E> class TwoElementSet {
private:
  const E first, second;

public:
  TwoElementSet(E first, E second) : first(first), second(second){};
  std::size_t size() { return 2; }
  virtual ~TwoElementSet() = default;
};

} // namespace psr

#endif
