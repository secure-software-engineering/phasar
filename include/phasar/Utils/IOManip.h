/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_IOMANIP_H
#define PHASAR_UTILS_IOMANIP_H

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {
struct BoolAlpha {
  bool Value{};
};
struct Flush {};
static constexpr Flush flush; // NOLINT

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, BoolAlpha BA);
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Flush);

} // namespace psr

#endif // PHASAR_UTILS_IOMANIP_H
