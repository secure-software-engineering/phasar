/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/
#pragma once

#include "phasar/ControlFlow/ICFG.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/Utils/TypeTraits.h"

#include <type_traits>

namespace psr {

// XXX: More constraints to be added
template <typename T>
concept IsDataFlowFact =
    std::is_copy_constructible_v<T> && psr::IsEqualityComparable<T> &&
    psr::is_std_hashable_v<T>;

// XXX: More constraints to be added
template <typename T>
concept IsEdgeValue =
    std::is_move_constructible_v<T> && psr::IsEqualityComparable<T>;

template <typename T>
concept IfdsAnalysisDomain = IsAnalysisDomain<T> && requires() {
  typename T::d_t;
  typename T::i_t;

  requires IsDataFlowFact<typename T::d_t>;
  requires ICFG<typename T::i_t>;
};

template <typename T>
concept IdeAnalysisDomain = IfdsAnalysisDomain<T> && requires() {
  typename T::l_t;
  requires IsEdgeValue<typename T::l_t>;
};
} // namespace psr
