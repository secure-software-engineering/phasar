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

#define PSR_CONCEPT concept

#define PSR_CONSTINIT constinit

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if __has_feature(attribute_deprecated_with_message)
#define PSR_DEPRECATED(MSG, REPLACEMENT)                                       \
  __attribute__((deprecated(MSG, REPLACEMENT)))
#else
#define PSR_DEPRECATED(MSG, REPLACEMENT) [[deprecated(MSG)]]
#endif

#if __has_cpp_attribute(clang::lifetimebound)
#define PSR_LIFETIMEBOUND [[clang::lifetimebound]]
#elif __has_cpp_attribute(lifetimebound)
#define PSR_LIFETIMEBOUND [[lifetimebound]]
#else
#define PSR_LIFETIMEBOUND
#endif

#if __has_cpp_attribute(clang::lifetime_capture_by)
#define PSR_LIFETIME_CAPTURE_BY(...) [[clang::lifetime_capture_by(__VA_ARGS__)]]
#else
#define PSR_LIFETIME_CAPTURE_BY(...)
#endif

#if __has_cpp_attribute(clang::internal_linkage)
#define PSR_INTERNAL_LINKAGE [[clang::internal_linkage]]
#else
#define PSR_INTERNAL_LINKAGE
#endif

#if __has_cpp_attribute(clang::trivial_abi)
#define PSR_TRIVIAL_ABI [[clang::trivial_abi]]
#else
#define PSR_TRIVIAL_ABI
#endif

#if __has_cpp_attribute(clang::require_explicit_initialization)
#define PSR_REQUIRE_EXPLICIT_INITIALIZATION                                    \
  [[clang::require_explicit_initialization]]
#else
#define PSR_REQUIRE_EXPLICIT_INITIALIZATION
#endif

#if __has_cpp_attribute(clang::enum_extensibility)
#define PSR_ENUM_EXTENSIBILITY(...) [[clang::enum_extensibility(__VA_ARGS__)]]
#else
#define PSR_ENUM_EXTENSIBILITY(...)
#endif

#if __has_cpp_attribute(gsl::Owner)
#define PSR_OWNER(...) [[gsl::Owner(__VA_ARGS__)]]
#else
#define PSR_OWNER(...)
#endif

#if __has_cpp_attribute(gsl::Pointer)
#define PSR_POINTER(...) [[gsl::Pointer(__VA_ARGS__)]]
#else
#define PSR_POINTER(...)
#endif

#if __has_cpp_attribute(clang::flag_enum)
#define PSR_FLAG_ENUM [[clang::flag_enum]]
#else
#define PSR_FLAG_ENUM
#endif

#if __has_cpp_attribute(clang::preferred_name)
#define PSR_PREFERRED_NAME(...) [[clang::preferred_name(__VA_ARGS__)]]
#else
#define PSR_PREFERRED_NAME(...)
#endif

#endif // PHASAR_UTILS_MACROS_H
