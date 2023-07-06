/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * BinaryDomain.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_DOMAIN_BINARYDOMAIN_H
#define PHASAR_DOMAIN_BINARYDOMAIN_H

#include "phasar/Utils/JoinLattice.h"

#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

enum class BinaryDomain { BOTTOM = 0, TOP = 1 };

[[nodiscard]] std::string to_string(BinaryDomain B);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, BinaryDomain B);

template <> struct JoinLatticeTraits<BinaryDomain> {
  static constexpr BinaryDomain top() noexcept { return BinaryDomain::TOP; }
  static constexpr BinaryDomain bottom() noexcept {
    return BinaryDomain::BOTTOM;
  }
  static constexpr BinaryDomain join(BinaryDomain LHS,
                                     BinaryDomain RHS) noexcept {
    return LHS == RHS ? LHS : BinaryDomain::BOTTOM;
  }
};

} // namespace psr

#endif
