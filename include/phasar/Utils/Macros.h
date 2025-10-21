/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_MACROS_H
#define PHASAR_UTILS_MACROS_H

#define PSR_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#if __cplusplus < 202002L
#define PSR_CONCEPT static constexpr bool
#else
#define PSR_CONCEPT concept
#endif

#if __cpp_constinit >= 201907L
#define PSR_CONSTINIT constinit
#elif __clang__
#define PSR_CONSTINIT [[clang::require_constant_initialization]]
#else
#define PSR_CONSTINIT
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if __has_feature(attribute_deprecated_with_message)
#define PSR_DEPRECATED(MSG, REPLACEMENT)                                       \
  __attribute__((deprecated(MSG, REPLACEMENT)))
#else
#define PSR_DEPRECATED(MSG, REPLACEMENT) [[deprecated(MSG)]]
#endif

#endif // PHASAR_UTILS_MACROS_H
