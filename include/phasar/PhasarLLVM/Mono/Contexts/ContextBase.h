/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * ContextBase.h
 *
 *  Created on: 18.06.2018
 *      Author: nicolas
 */

#ifndef CONTEXTBASE_H_
#define CONTEXTBASE_H_

#include <iostream>
#include <phasar/Utils/Macros.h>
#include <phasar/PhasarLLVM/Mono/Values/ValueBase.h>

#include <phasar/Config/ContainerConfiguration.h>

namespace psr {

/*  N = Node in the CFG
 *  V = Values inside the monotone sets (must be a sub class of ValueBase)
 *  ConcreteContext = The class that implement the context (must be a sub class
 *                    of ContextBase)
 */
template <typename N, typename V, typename ConcreteContext>
class ContextBase {
private:
  template<typename T1, typename T2>
  void ValueBase_check() {
    static_assert(std::is_base_of<ContextBase<N, V, ConcreteContext>, ConcreteContext>::value, "Template class ConcreteContext must be a sub class of ContextBase<N, V, ConcreteContext>\n");
    static_assert(std::is_base_of<ValueBase<T1, T2, V>, V>::value, "Template class V must be a sub class of ValueBase<T1, T2, V>\n");
  }
  
public:
  /*
   *
   */
  virtual void exitFunction(N src, N dest, MonoSet<V> &In) = 0;

  /*
   *
   */
  virtual void enterFunction(N src, N dest, MonoSet<V> &In) = 0;

  /*
   *
   */
  virtual bool isTotal() = 0;

  /*
   *
   */
  virtual bool isEqual(const ConcreteContext &rhs) const = 0;
  virtual bool isDifferent(const ConcreteContext &rhs) const = 0;
  virtual bool isLessThan(const ConcreteContext &rhs) const = 0;
  virtual void print(std::ostream &os) const = 0;

  friend bool operator==(const ConcreteContext &lhs, const ConcreteContext &rhs) {
    return lhs.isEqual(rhs);
  }
  friend bool operator!=(const ConcreteContext &lhs, const ConcreteContext &rhs) {
    return lhs.isDifferent(rhs);
  }
  friend bool operator<(const ConcreteContext &lhs, const ConcreteContext &rhs) {
    return lhs.isLessThan(rhs);
  }
  friend std::ostream &operator<<(std::ostream &os, const ConcreteContext &c) {
    c.print(os);
    return os;
  }
};

}
#endif
