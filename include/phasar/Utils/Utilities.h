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

std::string cxx_demangle(const std::string &mangled_name);

bool isConstructor(const std::string &mangled_name);

std::string debasify(const std::string &name);

const llvm::Type *stripPointer(const llvm::Type *pointer);

bool isMangled(const std::string &name);

std::vector<std::string> splitString(const std::string &str,
                                     const std::string &delimiter);

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
  std::set<std::set<T>> powerset;
  for (std::size_t i = 0; i < (1 << s.size()); ++i) {
    std::set<T> subset;
    for (std::size_t j = 0; j < s.size(); ++j) {
      if ((i & (1 << j)) > 0) {
        auto it = s.begin();
        advance(it, j);
        subset.insert(*it);
      }
      powerset.insert(subset);
    }
  }
  return powerset;
}

std::ostream &operator<<(std::ostream &os, const std::vector<bool> &bits);

struct stringIDLess {
  bool operator()(const std::string &lhs, const std::string &rhs) const;
};

} // namespace psr

#endif
