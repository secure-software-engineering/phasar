/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_ALIGNNUM_H
#define PHASAR_UTILS_ALIGNNUM_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {

template <typename T, unsigned NumOffs = 32> struct AlignNum {
  llvm::StringRef Name;
  T Num;

  constexpr AlignNum(llvm::StringRef Name, T Num) noexcept
      : Name(Name), Num(Num) {}
  constexpr AlignNum(llvm::StringRef Name, size_t Numerator,
                     size_t Denominator) noexcept
      : Name(Name), Num(double(Numerator) / double(Denominator)) {}

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const AlignNum &AN) {
    auto Len = AN.Name.size() + 1;
    auto Diff = -(Len < NumOffs) & (NumOffs - Len);

    OS << AN.Name << ':';
    // Default is two fixed-point decimal places, so shift the output by three
    // spaces
    OS.indent(Diff + std::is_floating_point_v<T> * 3);
    OS << llvm::formatv("{0,+7}\n", AN.Num);

    return OS;
  }
};
template <typename T> AlignNum(llvm::StringRef, T) -> AlignNum<T>;
AlignNum(llvm::StringRef, size_t, size_t) -> AlignNum<double>;

template <unsigned NumOffs = 32> struct AlignStr {
  llvm::StringRef Name;
  llvm::StringRef Value;

  constexpr AlignStr(llvm::StringRef Name, llvm::StringRef Value) noexcept
      : Name(Name), Value(Value) {}

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const AlignStr &AS) {
    auto Len = AS.Name.size();
    auto Diff = -(Len < NumOffs) & (NumOffs - Len);

    OS << AS.Name << ':';
    OS.indent(Diff);
    return OS << AS.Value << '\n';
  }
};
} // namespace psr

#endif // PHASAR_UTILS_ALIGNNUM_H
