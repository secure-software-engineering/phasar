/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_BYREF_H
#define PHASAR_PHASARLLVM_UTILS_BYREF_H

#include <type_traits>

namespace psr {
template <typename T>
using ByConstRef = std::conditional_t<sizeof(T) <= 2 * sizeof(void *) &&
                                          std::is_trivially_copyable_v<T>,
                                      T, const T &>;
template <typename T>
using ByMoveRef = std::conditional_t<sizeof(T) <= 2 * sizeof(void *) &&
                                         std::is_trivially_copyable_v<T>,
                                     T, T &&>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_BYREF_H
