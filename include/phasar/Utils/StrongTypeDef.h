#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/Support/HashBuilder.h"

#include <limits>
#include <numeric>
#include <type_traits>

#define PHASAR_STRONG_TYPEDEF(NAMESPACE, TYPE, NAME, ...)                      \
  namespace NAMESPACE {                                                        \
  enum class [[clang::enum_extensibility(open)]] NAME : TYPE { __VA_ARGS__ };  \
  }                                                                            \
  namespace llvm {                                                             \
  template <> struct DenseMapInfo<NAMESPACE::NAME> {                           \
    using QUAL_NAME = NAMESPACE::NAME;                                         \
    static constexpr QUAL_NAME getEmptyKey() noexcept {                        \
      return QUAL_NAME(std::is_signed_v<TYPE>                                  \
                           ? std::numeric_limits<TYPE>::min()                  \
                           : std::numeric_limits<TYPE>::max());                \
    }                                                                          \
    static constexpr QUAL_NAME getTombstoneKey() noexcept {                    \
      return QUAL_NAME(std::is_signed_v<TYPE>                                  \
                           ? std::numeric_limits<TYPE>::min() + 1              \
                           : std::numeric_limits<TYPE>::max() - 1);            \
    }                                                                          \
    static auto getHashValue(QUAL_NAME Id) noexcept {                          \
      return llvm::hash_value(TYPE(Id));                                       \
    }                                                                          \
    static constexpr bool isEqual(QUAL_NAME Id1, QUAL_NAME Id2) noexcept {     \
      return Id1 == Id2;                                                       \
    }                                                                          \
  };                                                                           \
  }

#define PHASAR_DERIVE_ENUM_DMI(QUAL_NAME, TYPE, TYPE_PARAM)                    \
  namespace llvm {                                                             \
  template <TYPE_PARAM> struct DenseMapInfo<QUAL_NAME> {                       \
    static constexpr QUAL_NAME getEmptyKey() noexcept {                        \
      return QUAL_NAME(std::is_signed_v<TYPE>                                  \
                           ? std::numeric_limits<TYPE>::min()                  \
                           : std::numeric_limits<TYPE>::max());                \
    }                                                                          \
    static constexpr QUAL_NAME getTombstoneKey() noexcept {                    \
      return QUAL_NAME(std::is_signed_v<TYPE>                                  \
                           ? std::numeric_limits<TYPE>::min() + 1              \
                           : std::numeric_limits<TYPE>::max() - 1);            \
    }                                                                          \
    static auto getHashValue(QUAL_NAME Id) noexcept {                          \
      return llvm::hash_value(TYPE(Id));                                       \
    }                                                                          \
    static constexpr bool isEqual(QUAL_NAME Id1, QUAL_NAME Id2) noexcept {     \
      return Id1 == Id2;                                                       \
    }                                                                          \
  };                                                                           \
  }

#define PHASAR_DERIVE_DMI(QUAL_TYPE)                                           \
  namespace llvm {                                                             \
  template <> struct DenseMapInfo<::QUAL_TYPE> {                               \
    using Type = ::QUAL_TYPE;                                                  \
    static Type getEmptyKey() noexcept { return Type::getEmptyKey(); }         \
    static Type getTombstoneKey() noexcept { return Type::getTombstoneKey(); } \
    static bool isEqual(::psr::ByConstRef<Type> Lhs,                           \
                        ::psr::ByConstRef<Type> Rhs) noexcept {                \
      return Lhs == Rhs;                                                       \
    }                                                                          \
    static auto getHashValue(::psr::ByConstRef<Type> Val) noexcept {           \
      using llvm::hash_value;                                                  \
      return hash_value(Val);                                                  \
    }                                                                          \
  };                                                                           \
  }

namespace psr {
template <typename EnumT>
// NOLINTNEXTLINE(readability-identifier-naming)
[[nodiscard]] constexpr auto to_underlying(EnumT Val) noexcept
  requires(std::is_enum_v<EnumT>)
{
  return static_cast<std::underlying_type_t<EnumT>>(Val);
}
template <typename EnumT>
// NOLINTNEXTLINE(readability-identifier-naming)
[[nodiscard]] constexpr auto to_underlying(EnumT Val) noexcept
  requires(std::is_integral_v<EnumT>)
{
  return Val;
}
} // namespace psr
