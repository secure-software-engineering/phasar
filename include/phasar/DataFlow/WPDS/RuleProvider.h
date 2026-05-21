#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/ArrayRef.h"

#include <concepts>

namespace psr::wpds {
template <typename T>
concept RuleProvider = requires(T &RP, typename T::control_location_type CL,
                                typename T::stack_element_type SE) {
  typename T::control_location_type;
  typename T::stack_element_type;
  typename T::weight_type;
  {
    RP.getNormalRules(CL, SE)
  } -> psr::is_iterable_over_v<
      std::tuple<typename T::control_location_type,
                 typename T::stack_element_type, typename T::weight_type>>;

  {
    RP.getPushRules(CL, SE)
  } -> psr::is_iterable_over_v<std::tuple<
      typename T::control_location_type, typename T::stack_element_type,
      typename T::stack_element_type, typename T::weight_type>>;

  {
    // CurrCL, ExitSE, RetSiteSE, EntrySE
    RP.getPopRules(CL, SE, SE, SE)
  } -> psr::is_iterable_over_v<
      std::tuple<typename T::control_location_type, typename T::weight_type>>;

  { RP.hasPopRules(CL, SE) } -> std::convertible_to<bool>;

  {
    RP.initialSeeds()
  } -> psr::is_iterable_over_v<
      std::tuple<typename T::control_location_type,
                 typename T::stack_element_type, typename T::weight_type>>;
};

template <typename T>
concept CanInjectAdditionalPushEdges =
    requires(T &RP, typename T::control_location_type CL,
             typename T::stack_element_type SE) {
      RP.injectAdditionalPushEdges(
          CL, SE,
          [](llvm::ArrayRef<std::tuple<typename T::control_location_type,
                                       typename T::stack_element_type,
                                       typename T::stack_element_type>>
                 Edges,
             typename T::weight_type Weight) {});
    };
} // namespace psr::wpds
