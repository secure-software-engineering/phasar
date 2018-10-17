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

#ifndef PHASAR_PHASARLLVM_MONO_CONTEXTS_CONTEXTBASE_H_
#define PHASAR_PHASARLLVM_MONO_CONTEXTS_CONTEXTBASE_H_

#include <ostream>
#include <phasar/PhasarLLVM/Utils/Printer.h>

namespace psr {

/**
 * Base class for function contexts used in the monotone framework. A function
 * context describes the state of the analyzed function.
 * @tparam N in the ICFG
 * @tparam D domain of the analysis
 * @tparam ConcreteContext class that implements the context (must be a sub
 * class of ContextBase<N,D,ConcreteContext>)
 */
template <typename N, typename D, typename ConcreteContext> class ContextBase {
public:
  using Node_t = N;
  using Domain_t = D;
  const NodePrinter<N> *NP;
  const DataFlowFactPrinter<D> *DP;

private:
  void ValueBase_check() {
    static_assert(
        std::is_base_of<ContextBase<Node_t, Domain_t, ConcreteContext>,
                        ConcreteContext>::value,
        "Template class ConcreteContext must be a sub class of ContextBase<N, "
        "V, ConcreteContext>\n");
  }

public:
  ContextBase(const NodePrinter<N> *np, const DataFlowFactPrinter<D> *dp)
      : NP(np), DP(dp) {}

  /*
   * Update the context at the exit of a function
   */
  virtual void exitFunction(const Node_t src, const Node_t dest,
                            const Domain_t &In) = 0;

  /*
   *
   */
  virtual void enterFunction(const Node_t src, const Node_t dest,
                             const Domain_t &In) = 0;

  /*
   *
   */
  virtual bool isUnsure() = 0;

  /*
   *
   */
  virtual bool isEqual(const ConcreteContext &rhs) const = 0;
  virtual bool isDifferent(const ConcreteContext &rhs) const = 0;
  virtual bool isLessThan(const ConcreteContext &rhs) const = 0;
  virtual void print(std::ostream &os) const = 0;

  friend bool operator==(const ConcreteContext &lhs,
                         const ConcreteContext &rhs) {
    return lhs.isEqual(rhs);
  }
  friend bool operator!=(const ConcreteContext &lhs,
                         const ConcreteContext &rhs) {
    return lhs.isDifferent(rhs);
  }
  friend bool operator<(const ConcreteContext &lhs,
                        const ConcreteContext &rhs) {
    return lhs.isLessThan(rhs);
  }
  friend std::ostream &operator<<(std::ostream &os, const ConcreteContext &c) {
    c.print(os);
    return os;
  }
};

} // namespace psr

#endif
