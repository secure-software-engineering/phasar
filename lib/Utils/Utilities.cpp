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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/core/demangle.hpp>

#include <llvm/IR/DerivedTypes.h>

#include <cxxabi.h>

#include <phasar/Utils/Utilities.h>

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

string cxx_demangle(const string &mangled_name) {
  return boost::core::demangle(mangled_name.c_str());
}

bool isConstructor(const string &mangled_name) {
  // WARNING: Doesn't work for templated classes, should
  // the best way to do it I can think of is to use a lexer
  // on the name to detect the constructor point explained
  // in the Itanium C++ ABI:
  // see https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling

  // This version will not work in some edge cases
  auto constructor = boost::algorithm::find_last(mangled_name, "C2E");

  if (constructor.begin() != constructor.end())
    return true;

  constructor = boost::algorithm::find_last(mangled_name, "C1E");

  if (constructor.begin() != constructor.end())
    return true;

  constructor = boost::algorithm::find_last(mangled_name, "C2E");

  if (constructor.begin() != constructor.end())
    return true;

  return false;
}

const llvm::Type *stripPointer(const llvm::Type *pointer) {
  auto next = llvm::dyn_cast<llvm::PointerType>(pointer);
  while (next) {
    pointer = next->getElementType();
    next = llvm::dyn_cast<llvm::PointerType>(pointer);
  }

  return pointer;
}

bool isMangled(const string &name) { return name != cxx_demangle(name); }

vector<string> splitString(const string &str, const string &delimiter) {
  vector<string> split_strings;
  boost::split(split_strings, str, boost::is_any_of(delimiter),
               boost::token_compress_on);
  return split_strings;
}

ostream &operator<<(ostream &os, const vector<bool> &bits) {
  for (auto bit : bits) {
    os << bit;
  }
  return os;
}

bool stringIDLess::operator()(const std::string &lhs,
                              const std::string &rhs) const {
  char *endptr1, *endptr2;
  long lhs_val = strtol(lhs.c_str(), &endptr1, 10);
  long rhs_val = strtol(rhs.c_str(), &endptr2, 10);
  if (lhs.c_str() == endptr1 && lhs.c_str() == endptr2) {
    return lhs < rhs;
  } else if (lhs.c_str() == endptr1 && rhs.c_str() != endptr2) {
    return false;
  } else if (lhs.c_str() != endptr1 && rhs.c_str() == endptr2) {
    return true;
  } else {
    return lhs_val < rhs_val;
  }
}

} // namespace psr
