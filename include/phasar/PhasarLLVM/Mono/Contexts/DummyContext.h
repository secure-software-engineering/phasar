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

#include "ContextBase"

class DummmyContext : ContextBase<void, void, DummyContext> {
public:
  using Node_t    = void;
  using Domain_t  = void;

public:
  virtual void exitFunction(const Node_t src, const Node_t dest, const Domain_t &In) {};
  virtual void enterFunction(const Node_t src, const Node_t dest, const Domain_t &In) {};
  virtual bool isUnsure() { return true; };

  virtual bool isEqual(const ConcreteContext &rhs) const { return true; };
  virtual bool isDifferent(const ConcreteContext &rhs) const { return false; };
  virtual bool isLessThan(const ConcreteContext &rhs) const { return false; };
  virtual void print(std::ostream &os) const {};
};
