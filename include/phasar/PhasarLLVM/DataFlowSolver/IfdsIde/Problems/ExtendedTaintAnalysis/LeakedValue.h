/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_LEAKEDVALUE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_LEAKEDVALUE_H_

#include <iosfwd>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"

namespace llvm {
class Value;
}

namespace psr {
/// Unification of leaked-value information in both the classical and the modern
/// variant
template <bool CLASSIC> struct LeakedValue;

namespace detail {
/// Common members that are shared between both variants
struct LeakedValueBase {
  /// The concrete llvm::Value that was leaked
  const llvm::Value *leak;

  LeakedValueBase(const llvm::Value *leak);

  friend std::ostream &operator<<(std::ostream &os, const LeakedValueBase &LV);
  friend bool operator==(const LeakedValueBase &LHS,
                         const LeakedValueBase &RHS);
  friend bool operator<(const LeakedValueBase &LHS, const LeakedValueBase &RHS);
};
} // namespace detail

/// A tuple containing the leaked llvm::Value and the corresponding
/// AbstractMemoryLocation that caused the leak. This is necessary, since the
/// AbstractMemoryLocation itself does not encode the concrete llvm::Value that
/// was leaked, only a description how to get there from some root
template <> struct LeakedValue<true> : public detail::LeakedValueBase {
  AbstractMemoryLocation taint;

  LeakedValue(const llvm::Value *leak, const AbstractMemoryLocation &taint)
      : LeakedValueBase(leak), taint(taint) {}
  LeakedValue(const llvm::Value *leak, AbstractMemoryLocation &&taint)
      : LeakedValueBase(leak), taint(std::move(taint)) {}
};
/// Contains the leaked llvm::Value
template <> struct LeakedValue<false> : public detail::LeakedValueBase {
  using LeakedValueBase::LeakedValueBase;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_LEAKEDVALUE_H_