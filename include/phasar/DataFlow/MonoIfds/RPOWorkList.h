#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/ControlFlowOrder.h"
#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/BitVector.h"

namespace psr::monoifds {
/// See SootUp's
/// [UniverseSortedPriorityQueue](https://github.com/soot-oss/SootUp/blob/develop/sootup.analysis.intraprocedural/src/main/java/sootup/analysis/intraprocedural/UniverseSortedPriorityQueue.java)
template <typename ItemT> class TopoFixpointDriver {
public:
  TopoFixpointDriver() noexcept = default;

  TopoFixpointDriver(ControlFlowOrder<ItemT> &&CFO) : CFO(std::move(CFO)) {
    WorkList.resize(this->CFO.Order.size());
  }

  void push(ByConstRef<ItemT> Item) {
    auto IId = CFO.Order.get(Item);
    WorkList.set(uint32_t(IId));

    if (int(IId) > Max) {
      Max = int(IId);
    }
  }

  std::optional<ItemT> pop() {
    if (Max < 0) {
      return std::nullopt;
    }

    auto IId = typename ControlFlowOrder<ItemT>::CFGOrderId(Max);
    Max = WorkList.find_prev(Max);
    return CFO.Order[IId];
  }

  [[nodiscard]] bool empty() const noexcept { return Max < 0; }

  LLVM_ATTRIBUTE_ALWAYS_INLINE void run(std::invocable<ItemT> auto Handler) {
    while (auto Inst = pop()) {
      std::invoke(Handler, *Inst);
    }
  }

  [[nodiscard]] constexpr const auto &getCFO() const noexcept { return CFO; }

private:
  ControlFlowOrder<ItemT> CFO;
  llvm::BitVector WorkList;
  int Max = -1;
};
} // namespace psr::monoifds
