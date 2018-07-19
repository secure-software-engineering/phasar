/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * DummyContext.h
 *
 *  Created on: 19.07.2018
 *      Author: nicolas
 */

/*
 * Dummy context that does not stock anything if you do not need context.
 */
#pragma once

#include <ostream>

#include "ContextBase.h"


namespace psr {

class DummyContext : ContextBase<void*, void*, DummyContext> {
public:
  using Node_t    = ContextBase::Node_t;
  using Domain_t  = ContextBase::Domain_t;
  using ConcreteContext = DummyContext;

public:
  virtual void exitFunction(const Node_t src, const Node_t dest, const Domain_t &In) override {};
  virtual void enterFunction(const Node_t src, const Node_t dest, const Domain_t &In) override {};
  virtual bool isUnsure() override { return true; };

  virtual bool isEqual(const ConcreteContext &rhs) const override { return true; };
  virtual bool isDifferent(const ConcreteContext &rhs) const override { return false; };
  virtual bool isLessThan(const ConcreteContext &rhs) const override { return false; };
  virtual void print(std::ostream &os) const override {};
};

} // namespace psr
