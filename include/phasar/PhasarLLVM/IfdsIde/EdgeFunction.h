/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AbstractEdgeFunction.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#pragma once

#include <iostream> // std::cout in dump, better replace it with a ostream
#include <memory>
#include <string>

namespace psr {

template <typename V> class EdgeFunction {
public:
  virtual ~EdgeFunction() = default;

  virtual V computeTarget(V source) = 0;

  virtual std::shared_ptr<EdgeFunction<V>>
  composeWith(std::shared_ptr<EdgeFunction<V>> secondFunction) = 0;

  virtual std::shared_ptr<EdgeFunction<V>>
  joinWith(std::shared_ptr<EdgeFunction<V>> otherFunction) = 0;

  virtual bool equalTo(std::shared_ptr<EdgeFunction<V>> other) = 0;

  virtual void dump() { std::cout << "edge function\n"; }

  virtual std::string toString() { return "edge function"; }
};

} // namespace psr
