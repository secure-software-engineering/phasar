/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * ValueBase.h
 *
 *  Created on: 19.06.2018
 *      Author: nicolas
 */

#ifndef SRC_ANALYSIS_MONOTONE_VALUEBASE_H_
#define SRC_ANALYSIS_MONOTONE_VALUEBASE_H_

#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>
#include <phasar/Config/ContainerConfiguration.h>

namespace psr {

/*  Id = Identifier for the value
 *  V = Value itself associated to an identifier
 */
template <typename Id, typename V, typename ConcreteValue>
class ValueBase {
public:
  virtual Id getId() const = 0;
  virtual V getValue() const = 0;

  virtual bool isEqual(const ConcreteValue &lhs) = 0; 

  friend bool operator==(const ConcreteValue &lhs, const ConcreteValue &rhs) {
    return lhs.isEqual(rhs);
  }
};
}

#endif
