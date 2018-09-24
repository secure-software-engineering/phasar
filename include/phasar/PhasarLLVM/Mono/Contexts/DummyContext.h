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

#ifndef PHASAR_PHASARLLVM_MONO_CONTEXTS_DUMMYCONTEXT_H_
#define PHASAR_PHASARLLVM_MONO_CONTEXTS_DUMMYCONTEXT_H_

#include <ostream>

#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>

namespace psr {

/**
 * @brief Dummy context that does not stock anything if you do not need
 * a context.
 */
class DummyContext : ContextBase<void *, void *, DummyContext> {
public:
  using Node_t = ContextBase::Node_t;
  using Domain_t = ContextBase::Domain_t;
  using ConcreteContext = DummyContext;

public:
  void exitFunction(const Node_t src, const Node_t dest,
                    const Domain_t &In) override{};
  void enterFunction(const Node_t src, const Node_t dest,
                     const Domain_t &In) override{};
  bool isUnsure() override { return true; };
  bool isEqual(const ConcreteContext &rhs) const override { return true; };
  bool isDifferent(const ConcreteContext &rhs) const override { return false; };
  bool isLessThan(const ConcreteContext &rhs) const override { return false; };
  void print(std::ostream &os) const override{};
};

} // namespace psr

#endif
