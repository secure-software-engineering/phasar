#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/ArraySet.h"

namespace psr::monoifds {

/// \brief Simple worklist, based on a de-duplicating queue
template <typename ItemT> class ArraySetDriver {
public:
  void push(ItemT Item) { WL.insert(std::move(Item)); }

  template <typename HandlerT>
  LLVM_ATTRIBUTE_ALWAYS_INLINE void run(HandlerT Handler) {
    WL.foreach (std::move(Handler));
  }

  [[nodiscard]] constexpr bool empty() const noexcept { return WL.empty(); }

private:
  ArraySet<ItemT> WL;
};
} // namespace psr::monoifds
