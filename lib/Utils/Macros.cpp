/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iterator>
#include <ostream>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <llvm/IR/DerivedTypes.h>

#include <cxxabi.h>

#include <phasar/Utils/Macros.h>
using namespace std;
using namespace psr;

namespace psr {

string cxx_demangle(const string &mangled_name) {
  int status = 0;
  char *demangled =
      abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
  string result((status == 0 && demangled != NULL) ? demangled : mangled_name);
  free(demangled);
  return result;
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

string debasify(const string &name) {
  static const string base = ".base";
  if (boost::algorithm::ends_with(name, base)) {
    return name.substr(0, name.size() - base.size());
  } else {
    return name;
  }
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
} // namespace psr
