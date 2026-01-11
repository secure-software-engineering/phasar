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

#include <type_traits>

namespace psr {
template <typename T>
concept IfdsAnalysisDomain = IsAnalysisDomain<T> && requires() {
  typename T::d_t;
  typename T::i_t;

  requires(!std::is_void_v<typename T::d_t>);
  requires ICFG<typename T::i_t>;
};

template <typename T>
concept IdeAnalysisDomain = IfdsAnalysisDomain<T> && requires() {
  typename T::l_t;
  requires(!std::is_void_v<typename T::l_t>);
};
} // namespace psr
