/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_NULLABLE_H
#define PHASAR_UTILS_NULLABLE_H

#include <optional>
#include <type_traits>

namespace psr {

template <typename T>
using Nullable =
    std::conditional_t<std::is_convertible_v<T, bool>, T, std::optional<T>>;
} // namespace psr

#endif // PHASAR_UTILS_NULLABLE_H
