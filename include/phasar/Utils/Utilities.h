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
#include <vector>

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
std::set<std::set<T>> computePowerSet(const std::set<T> &S) {
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
  for (std::size_t I = 0; I < (1 << S.size()); ++I) {
    std::set<T> Subset;
    for (std::size_t J = 0; J < S.size(); ++J) {
      if ((I & (1 << J)) > 0) {
        auto It = S.begin();
        advance(It, J);
        Subset.insert(*It);
      }
      Powerset.insert(Subset);
    }
  }
  return Powerset;
}

std::ostream &operator<<(std::ostream &Os, const std::vector<bool> &Bits);

struct StringIdLess {
  bool operator()(const std::string &Lhs, const std::string &Rhs) const;
};

} // namespace psr

#endif
