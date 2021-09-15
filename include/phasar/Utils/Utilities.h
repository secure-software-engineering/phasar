/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_UTILITIES_H_
#define PHASAR_UTILS_UTILITIES_H_

#include <iosfwd>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"

#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/TypeTraits.h"

namespace llvm {
class Type;
} // namespace llvm

namespace psr {

std::string createTimeStamp();

bool isConstructor(const std::string &MangledName);

std::string debasify(const std::string &Name);

const llvm::Type *stripPointer(const llvm::Type *Pointer);

bool isMangled(const std::string &Name);

std::vector<std::string> splitString(const std::string &Str,
                                     const std::string &Delimiter);

template <typename T>
std::set<std::set<T>> computePowerSet(const std::set<T> &s) {
  // compute all subsets of {a, b, c, d}
  //  bit-pattern - {d, c, b, a}
  //  0000  {}
  //  0001  {a}
  //  0010  {b}
  //  0011  {a, b}
  //  0100  {c}
  //  0101  {a, c}
  //  0110  {b, c}
  //  0111  {a, b, c}
  //  1000  {d}
  //  1001  {a, d}
  //  1010  {b, d}
  //  1011  {a, b, d}
  //  1100  {c, d}
  //  1101  {a, c, d}
  //  1110  {b, c, d}
  //  1111  {a, b, c, d}
  std::set<std::set<T>> Powerset;
  for (std::size_t i = 0; i < (1 << s.size()); ++i) {
    std::set<T> Subset;
    for (std::size_t j = 0; j < s.size(); ++j) {
      if ((i & (1 << j)) > 0) {
        auto It = s.begin();
        advance(It, j);
        Subset.insert(*It);
      }
      Powerset.insert(Subset);
    }
  }
  return Powerset;
}

/// \brief Computes the set-intersection of the potentially unordered sets
/// Dest and Src and stores the result back in Dest.
///
/// This function should work on all types of sets as long as they provide the
/// type value_type specifying, which type their elements have.
/// By-reference iteration is required, but the elements do not have any
/// requirements, although the performance is probably higher for small
/// elements that are trivially copyable.
template <typename ContainerTy, typename OtherContainerTy>
std::enable_if_t<!has_erase_iterator_v<ContainerTy>>
intersectWith(ContainerTy &Dest, const OtherContainerTy &Src) {
  static_assert(std::is_same_v<typename ContainerTy::value_type,
                               typename OtherContainerTy::value_type>,
                "The containers Src and Dest must be compatible");
  using ValueTy = typename ContainerTy::value_type;
  using ElementTy =
      std::conditional_t<std::is_trivially_copy_constructible_v<ValueTy> &&
                             sizeof(ValueTy) <= sizeof(void *),
                         ValueTy, ValueTy *>;

  auto removeFrom = [](auto &Dst, auto &&Elem) {
    if constexpr (std::is_same_v<ValueTy, ElementTy>) {
      Dst.erase(Elem);
    } else {
      Dst.erase(*Elem);
    }
  };

  /// This whole functionality is only for computing the set-intersection of
  /// Dest and Src storing the result in-place in Dest. It would be a big win,
  /// if the STL would provide us with in-place set operations and those that
  /// do not require the sets to be sorted...

  llvm::SmallVector<ElementTy, 16> Buffer;

  if constexpr (std::is_same_v<ValueTy, ElementTy>) {
    for (auto &&Elem : Dest) {
      if (!Src.count(Elem)) {
        Buffer.push_back(Elem);
      }
    }
  } else {
    for (auto &Elem : Dest) {
      if (!Src.count(Elem)) {
        Buffer.insert(&Elem);
      }
    }
  }

  for (auto &&Elem : Buffer) {
    removeFrom(Dest, Elem);
  }
}

template <typename ContainerTy, typename OtherContainerTy>
std::enable_if_t<has_erase_iterator_v<ContainerTy>>
intersectWith(ContainerTy &Dest, const OtherContainerTy &Src) {
  static_assert(std::is_same_v<typename ContainerTy::value_type,
                               typename OtherContainerTy::value_type>,
                "The containers Src and Dest must be compatible");

  for (auto It = Dest.begin(), End = Dest.end(); It != End;) {
    if (Src.count(*It)) {
      ++It;
    } else {
      It = Dest.erase(It);
    }
  }
}

template <typename T>
void intersectWith(BitVectorSet<T> &Dest, const BitVectorSet<T> &Src) {
  Dest.setIntersectWith(Src);
}

std::ostream &operator<<(std::ostream &OS, const std::vector<bool> &Bits);

struct stringIDLess {
  bool operator()(const std::string &LHS, const std::string &RHS) const;
};

/// See "https://en.cppreference.com/w/cpp/experimental/scope_exit/scope_exit"
template <typename Fn> class scope_exit {
public:
  template <typename FFn, typename = decltype(std::declval<FFn>()())>
  explicit scope_exit(FFn &&F) noexcept(
      std::is_nothrow_constructible_v<Fn, FFn> ||
      std::is_nothrow_constructible_v<Fn, FFn &>)
      : F(std::forward<FFn>(F)) {}

  ~scope_exit() { F(); }

  scope_exit(const scope_exit &) = delete;
  scope_exit(scope_exit &&) = delete;

  scope_exit &operator=(const scope_exit &) = delete;
  scope_exit &operator=(scope_exit &&) = delete;

private:
  Fn F;
};

template <typename Fn> scope_exit(Fn) -> scope_exit<Fn>;

// Copied from "https://en.cppreference.com/w/cpp/utility/variant/visit"
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace psr

#endif
