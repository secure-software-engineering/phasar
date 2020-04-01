/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <chrono>
#include <iterator>
#include <ostream>

#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/find.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/core/demangle.hpp"

#include "llvm/IR/DerivedTypes.h"

#include "cxxabi.h"

#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

std::string createTimeStamp() {
  auto Now = std::chrono::system_clock::now();
  auto NowTime = std::chrono::system_clock::to_time_t(Now);
  std::string TimeStr(std::ctime(&NowTime));
  std::replace(TimeStr.begin(), TimeStr.end(), ' ', '-');
  TimeStr.erase(std::remove(TimeStr.begin(), TimeStr.end(), '\n'),
                TimeStr.end());
  return TimeStr;
}

string cxx_demangle(const string &MangledName) {
  return boost::core::demangle(MangledName.c_str());
}

bool isConstructor(const string &MangledName) {
  // WARNING: Doesn't work for templated classes, should
  // the best way to do it I can think of is to use a lexer
  // on the name to detect the constructor point explained
  // in the Itanium C++ ABI:
  // see https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling

  // This version will not work in some edge cases
  auto constructor = boost::algorithm::find_last(MangledName, "C2E");

  if (constructor.begin() != constructor.end())
    return true;

  constructor = boost::algorithm::find_last(MangledName, "C1E");

  if (constructor.begin() != constructor.end())
    return true;

  constructor = boost::algorithm::find_last(MangledName, "C2E");

  if (constructor.begin() != constructor.end())
    return true;

  return false;
}

const llvm::Type *stripPointer(const llvm::Type *Pointer) {
  auto next = llvm::dyn_cast<llvm::PointerType>(Pointer);
  while (next) {
    Pointer = next->getElementType();
    next = llvm::dyn_cast<llvm::PointerType>(Pointer);
  }

  return Pointer;
}

bool isMangled(const string &Name) { return Name != cxx_demangle(Name); }

vector<string> splitString(const string &Str, const string &Delimiter) {
  vector<string> split_strings;
  boost::split(split_strings, Str, boost::is_any_of(Delimiter),
               boost::token_compress_on);
  return split_strings;
}

ostream &operator<<(ostream &OS, const vector<bool> &Bits) {
  for (auto bit : Bits) {
    OS << bit;
  }
  return OS;
}

bool stringIDLess::operator()(const std::string &Lhs,
                              const std::string &Rhs) const {
  char *endptr1, *endptr2;
  long lhsVal = strtol(Lhs.c_str(), &endptr1, 10);
  long rhsVal = strtol(Rhs.c_str(), &endptr2, 10);
  if (Lhs.c_str() == endptr1 && Lhs.c_str() == endptr2) {
    return Lhs < Rhs;
  } else if (Lhs.c_str() == endptr1 && Rhs.c_str() != endptr2) {
    return false;
  } else if (Lhs.c_str() != endptr1 && Rhs.c_str() == endptr2) {
    return true;
  } else {
    return lhsVal < rhsVal;
  }
}

} // namespace psr
