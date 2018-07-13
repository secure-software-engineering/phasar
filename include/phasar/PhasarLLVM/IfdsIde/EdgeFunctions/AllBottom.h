/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AllBottom.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_EDGE_FUNC_ALLBOTTOM_H_
#define ANALYSIS_IFDS_IDE_EDGE_FUNC_ALLBOTTOM_H_

#include <iostream>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/Utils/Macros.h>
#include <stdexcept>
#include <string>

namespace psr {

template <typename V> class EdgeIdentity;

template <typename V> class AllTop;

template <typename V>
class AllBottom : public EdgeFunction<V>,
                  public std::enable_shared_from_this<AllBottom<V>> {
private:
  const V bottomElement;

public:
  AllBottom(V bottomElement) : bottomElement(bottomElement) {}

  virtual ~AllBottom() = default;

  V computeTarget(V source) override { return bottomElement; }

  virtual std::shared_ptr<EdgeFunction<V>>
  composeWith(std::shared_ptr<EdgeFunction<V>> secondFunction) override {
    if (EdgeIdentity<V> *ei =
            dynamic_cast<EdgeIdentity<V> *>(secondFunction.get()))
      return this->shared_from_this();
    return secondFunction;
  }

  virtual std::shared_ptr<EdgeFunction<V>>
  joinWith(std::shared_ptr<EdgeFunction<V>> otherFunction) override {
    if (otherFunction.get() == this ||
        otherFunction->equal_to(this->shared_from_this()))
      return this->shared_from_this();
    if (AllTop<V> *alltop = dynamic_cast<AllTop<V> *>(otherFunction.get()))
      return this->shared_from_this();
    if (EdgeIdentity<V> *ei =
            dynamic_cast<EdgeIdentity<V> *>(otherFunction.get()))
      return this->shared_from_this();
    throw std::runtime_error("UNEXPECTED EDGE FUNCTION");
  }

  virtual bool equal_to(std::shared_ptr<EdgeFunction<V>> other) const override {
    if (AllBottom<V> *allbottom = dynamic_cast<AllBottom<V> *>(other.get())) {
      return (allbottom->bottomElement == bottomElement);
    }
    return false;
  }

  virtual void print(std::ostream &OS, bool isForDebug = false) const override { 
    OS << "all_bottom";  
  }
};

} // namespace psr

#endif /* ANALYSIS_IFDS_IDE_EDGE_FUNC_ALLBOTTOM_HH_ */
