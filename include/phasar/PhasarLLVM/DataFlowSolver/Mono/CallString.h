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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_CALLSTRING_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_CALLSTRING_H

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <stdexcept>

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {

template <typename T, unsigned K> class CallString {
private:
  std::deque<T> CS;
  static const unsigned KLimit = K;

public:
  CallString() = default;
  CallString(std::initializer_list<T> IList) : CS(IList) {
    if (IList.size() > KLimit) {
      throw std::runtime_error(
          "initial call string length exceeds maximal length K");
    }
  }

  void push(T S) {
    if (CS.size() > KLimit - 1) {
      CS.pop_front();
    }
    CS.push_back(S);
  }

  T returnSite() {
    if (!CS.empty()) {
      return CS.back();
    }
    return {};
  }

  void pop() {
    if (!CS.empty()) {
      CS.pop_back();
    }
  }

  size_t size() { return CS.size(); }

  std::deque<T> getInternalCS() const { return CS; }

  friend bool operator==(const CallString &Lhs, const CallString &Rhs) {
    return Lhs.cs == Rhs.cs;
  }

  friend bool operator!=(const CallString &Lhs, const CallString &Rhs) {
    return !(Lhs == Rhs);
  }

  friend bool operator<(const CallString &Lhs, const CallString &Rhs) {
    return Lhs.cs < Rhs.cs;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const CallString &C) {
    llvm::interleave(C.CS, OS, " * ");
    return OS;
  }
};

} // namespace psr

#endif
