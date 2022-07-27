/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_TYPETRAITSLLVM_H
#define PHASAR_UTILS_TYPETRAITSLLVM_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"

#include <type_traits>

namespace psr {

namespace detail {
template <typename T, typename = void>
struct has_setIFDSIDESolverConfig : std::false_type {};
template <typename T>
struct has_setIFDSIDESolverConfig<
    T, decltype(std::declval<T>().setIFDSIDESolverConfig(
           std::declval<IFDSIDESolverConfig>()))> : std::true_type {};
} // namespace detail

template <typename T>
constexpr bool has_setIFDSIDESolverConfig_v = // NOLINT
    detail::has_setIFDSIDESolverConfig<T>::value;

} // namespace psr
#endif
