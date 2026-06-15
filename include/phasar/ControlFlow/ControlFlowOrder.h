#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CFG.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseSet.h"

#include <iterator>

namespace psr {

template <typename N> struct ControlFlowOrder {
  enum class CFGOrderId : uint32_t {};

  Compressor<N, CFGOrderId> Order;

  [[nodiscard]] auto begin() const noexcept {
    return std::make_reverse_iterator(Order.begin());
  }
  [[nodiscard]] auto end() const noexcept {
    return std::make_reverse_iterator(Order.end());
  }
};

template <typename N, typename F, CFGOf<N, F> CFGTy>
void computeCFGOrder(ControlFlowOrder<N> &Into, const CFGTy &CF, const F &Fun) {
  llvm::SmallDenseSet<N> Seen;

  const auto Visit = [&](auto &Visit, ByConstRef<N> Inst) {
    if (!Seen.insert(Inst).second) {
      return;
    }

    scope_exit Push = [&]() { Into.Order.insert(Inst); };

    if constexpr (IsBlockAwareControlFlow<CFGTy>) {
      if (auto Next = CF.getUniqueSuccessor(Inst)) {
        Visit(Visit, unwrapNullable(Next));
        return;
      }
    }
    for (const auto &Succ : CF.getSuccsOf(Inst)) {
      Visit(Visit, Succ);
    }
  };

  for (const auto &SP : CF.getStartPointsOf(Fun)) {
    Visit(Visit, SP);
  }
}

template <typename N, typename F, CFGOf<N, F> CFGTy>
[[nodiscard]] inline ControlFlowOrder<N> computeCFGOrder(const CFGTy &CF,
                                                         const F &Fun) {
  ControlFlowOrder<N> Ret;
  computeCFGOrder(Ret, CF, Fun);
  return Ret;
}

} // namespace psr
