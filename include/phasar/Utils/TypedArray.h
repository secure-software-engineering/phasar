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
#include "phasar/Utils/TypeTraits.h"

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

namespace psr {

/// Tag type for the generator constructor of \c TypedArray.
struct generate_tag_t {};
/// Tag value passed to the \c TypedArray generator constructor.
inline constexpr generate_tag_t generate_tag{};

/// Fixed-size array indexed by a strongly-typed id \p IdT instead of
/// \c size_t.
///
/// \tparam IdT    Index type.  Must satisfy \c IdType (fits in \c size_t).
/// \tparam ValueT Element type.
/// \tparam N      Compile-time size.
template <IdType IdT, typename ValueT, unsigned N>
class TypedArray : public std::array<ValueT, N> {
  using Base = std::array<ValueT, N>;

public:
  using std::array<ValueT, N>::array;

  explicit constexpr TypedArray(
      generate_tag_t /*unused*/,
      std::invocable<IdT> auto
          Gen) noexcept(std::is_nothrow_invocable_v<decltype(Gen) &, IdT>) {
    [this, Gen = copyOrRef(Gen)]<size_t... I>(std::index_sequence<I...>) {
      ((this->Base::operator[](I) =
            std::invoke(Gen, std::integral_constant<IdT, IdT(I)>())),
       ...);
    }(std::make_index_sequence<N>());
  }

  [[nodiscard]] constexpr bool inbounds(IdT Id) const noexcept {
    return size_t(Id) < N;
  }

  [[nodiscard]] constexpr const ValueT &operator[](IdT Id) const & {
    assert(inbounds(Id));
    return this->Base::operator[](size_t(Id));
  }

  [[nodiscard]] constexpr ValueT &operator[](IdT Id) & {
    assert(inbounds(Id));
    return this->Base::operator[](size_t(Id));
  }

  [[nodiscard]] constexpr ValueT operator[](IdT Id) && {
    assert(inbounds(Id));
    return std::move(this->Base::operator[](size_t(Id)));
  }

  [[nodiscard]] llvm::ArrayRef<ValueT> asRef() const & noexcept {
    return {this->Base::data(), N};
  }
  [[nodiscard]] llvm::ArrayRef<ValueT> asRef() && noexcept = delete;

  [[nodiscard]] auto enumerate() const noexcept {
    return llvm::map_range(llvm::enumerate(*this), [](const auto &IndexAndVal) {
      return std::pair<IdT, ByConstRef<ValueT>>{IdT(IndexAndVal.index()),
                                                IndexAndVal.value()};
    });
  }
  [[nodiscard]] auto enumerate() noexcept {
    return llvm::map_range(llvm::enumerate(*this), [](auto &IndexAndVal) {
      return std::pair<IdT, ValueT &>{IdT(IndexAndVal.index()),
                                      IndexAndVal.value()};
    });
  }
};
} // namespace psr
