/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CallString.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_CONTEXTS_CALLSTRING_H_
#define PHASAR_PHASARLLVM_MONO_CONTEXTS_CALLSTRING_H_

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iterator>

#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>

namespace psr {

/**
 * A call string stores a finite length chain of calls that lead to the
 * function call.
 * @tparam N node in the ICFG
 * @tparam D domain of the analysis
 * @tparam K maximum depth of the call string
 */
template <typename N, typename D, unsigned K>
class CallString : public ContextBase<N, D, CallString<N, D, K>> {
public:
  using Node_t = N;
  using Domain_t = D;

protected:
  std::deque<Node_t> cs;
  static const unsigned k = K;

public:
  CallString(const NodePrinter<N> *np, const DataFlowFactPrinter<D> *dp)
      : ContextBase<N, D, CallString<N, D, K>>(np, dp) {}

  CallString(const NodePrinter<N> *np, const DataFlowFactPrinter<D> *dp,
             std::initializer_list<N> ilist)
      : ContextBase<N, D, CallString<N, D, K>>(np, dp), cs(ilist) {
    if (ilist.size() > k) {
      throw std::runtime_error(
          "initial call std::string length exceeds maximal length K");
    }
  }

  void enterFunction(Node_t src, Node_t dest, const Domain_t &In) override {
    if (k == 0)
      return;
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(src);
  }

  void exitFunction(Node_t src, Node_t dest, const Domain_t &In) override {
    if (cs.size() > 0) {
      cs.pop_back();
    }
  }

  bool isUnsure() override {
    // We may be a bit more precise in the future
    if (cs.size() == k)
      return true;
    else
      return false;
  }

  bool isEqual(const CallString &rhs) const override {
    return cs == rhs.cs || (cs.size() == 0) || (rhs.cs.size() == 0);
  }

  bool isDifferent(const CallString &rhs) const override {
    return !isEqual(rhs);
  }

  bool isLessThan(const CallString &rhs) const override {
    // Base : lhs.cs < rhs.cs
    // Addition : (lhs.cs.size() != 0) && (rhs.cs.size() != 0)
    // Enable that every empty call-string context match every context
    // That allows an output of a retFlow with an empty callString context
    // to be join with every analysis results at the arrival node.
    return cs < rhs.cs && (cs.size() != 0) && (rhs.cs.size() != 0);
  }

  void print(std::ostream &os) const override {
    os << "Call string [" << cs.size() << "]: ";
    for (auto C : cs) {
      os << this->NP->NtoString(C) << " * ";
    }
  }

  std::size_t size() const { return cs.size(); }
  std::deque<Node_t> getInternalCS() const { return cs; }
};

} // namespace psr

#endif
