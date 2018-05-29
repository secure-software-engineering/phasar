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

#ifndef ANALYSIS_IFDS_IDE_EDGEFUNCTION_H_
#define ANALYSIS_IFDS_IDE_EDGEFUNCTION_H_

#include <iostream>
#include <memory>
#include <string>
using namespace std;
namespace psr{

template <typename V> class EdgeFunction {
public:
  virtual ~EdgeFunction() = default;

  virtual V computeTarget(V source) = 0;

  virtual shared_ptr<EdgeFunction<V>>
  composeWith(shared_ptr<EdgeFunction<V>> secondFunction) = 0;

  virtual shared_ptr<EdgeFunction<V>>
  joinWith(shared_ptr<EdgeFunction<V>> otherFunction) = 0;

  virtual bool equalTo(shared_ptr<EdgeFunction<V>> other) = 0;

  virtual void dump() { std::cout << "edge function\n"; }

  virtual string toString() { return "edge function"; }
};

}//namespace psr

#endif /* ANALYSIS_IFDS_IDE_EDGEFUNCTION_HH_ */
