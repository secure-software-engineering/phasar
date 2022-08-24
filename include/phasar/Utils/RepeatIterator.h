#pragma once

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/iterator_range.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <type_traits>

namespace psr {
/// An iterator that iterates over the same value a specified number of times
template <typename T> class RepeatIterator {
public:
  using value_type = T;
  using reference = const T &;
  using pointer = const T *;
  using difference_type = ptrdiff_t;
  using iterator_category = std::input_iterator_tag;

  reference operator*() const noexcept {
    assert(elem.has_value() && "Dereferencing end()-iterator");
    return *Elem;
  }
  pointer operator->() const noexcept {
    assert(elem.has_value() && "Dereferencing end()-iterator");
    return &*Elem;
  }

  RepeatIterator &operator++() noexcept {
    ++Index;
    return *this;
  }
  RepeatIterator operator++(int) noexcept {
    auto Ret = *this;
    ++*this;
    return Ret;
  }

  bool operator==(const RepeatIterator &Other) const noexcept {
    return Other.Index == Index;
  }
  bool operator!=(const RepeatIterator &Other) const noexcept {
    return !(*this == Other);
  }

  template <typename TT,
            typename = std::enable_if_t<std::is_same_v<T, std::decay_t<TT>>>>
  explicit RepeatIterator(TT &&Elem) : Elem(std::forward<TT>(Elem)) {}
  explicit RepeatIterator(size_t Index, std::true_type /*AsEndIterator*/)
      : Index(Index), Elem(std::nullopt) {}

  RepeatIterator() noexcept = default;

private:
  size_t Index{};
  std::optional<T> Elem{};
};

template <typename T>
using RepeatRangeType = llvm::iterator_range<RepeatIterator<T>>;
template <typename T> auto repeat(T &&Elem, size_t Num) {
  using iterator_type = RepeatIterator<std::decay_t<T>>;
  auto Ret = llvm::make_range(iterator_type(std::forward<T>(Elem)),
                              iterator_type(Num, std::true_type{}));
  return Ret;
}

static_assert(is_iterable_over_v<RepeatRangeType<int>, int>);

} // namespace psr
