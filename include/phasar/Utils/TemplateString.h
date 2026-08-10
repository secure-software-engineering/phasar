#pragma once

#include "llvm/ADT/StringRef.h"

#include <cstddef>

namespace psr {
template <size_t N> struct TemplateString {
  char Buf[N + 1]{};

  constexpr TemplateString(const char (&Str)[N + 1]) noexcept {
    static_assert(N != SIZE_MAX);
    for (size_t I = 0; I != N; ++I) {
      Buf[I] = Str[I];
    }
  }

  constexpr operator llvm::StringRef() const noexcept { return {Buf, N}; }
};
template <size_t N> TemplateString(const char (&)[N]) -> TemplateString<N - 1>;
} // namespace psr
