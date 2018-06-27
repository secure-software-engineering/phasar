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

#pragma once

#include <phasar/Utils/Macros.h>

namespace psr {

/*  Id = Identifier for the value
 *  V = Value itself associated to an identifier
 */
template <typename Id, typename V, typename ConcreteValue>
class ValueBase {
private:
  void ValueBase_check() {
    static_assert(std::is_base_of<ValueBase<Id, V, ConcreteValue>, ConcreteValue>::value, "Template class ConcreteValue must be a sub class of ValueBase<Id, V, ConcreteValue>\n");
  }

public:
  virtual Id getId() const = 0;
  virtual V getValue() const = 0;

  virtual ~ValueBase() = default;

  virtual bool isEqual(const ConcreteValue &lhs) = 0;

  friend bool operator==(const ConcreteValue &lhs, const ConcreteValue &rhs) {
    return lhs.isEqual(rhs);
  }
};
}
