#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/ICFG.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/SCCGeneric.h"

#include <concepts>

namespace psr {
template <typename T>
concept CanGetICFG = requires(T &HA) {
  { HA.getICFG() } -> ICFG;
};

template <typename T, typename N, typename F>
concept CanGetICFGOf = requires(T &HA) {
  { HA.getICFG() } -> ICFGOf<N, F>;
};

template <typename T, typename F>
concept CanGetCompressedFunctionsOf = requires(T &HA) {
  {
    HA.getCompressedFunctions()
  } -> std::convertible_to<FunctionCompressor<F> &>;
};

template <typename T>
concept CanGetCGSCCs = requires(T &HA) {
  { &HA.getCGSCCs() } -> std::convertible_to<const SCCHolder<FunctionId> *>;
};

} // namespace psr
