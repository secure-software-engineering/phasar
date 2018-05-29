/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/Utils/Macros.h>
using namespace psr;
namespace psr{

string cxx_demangle(const string &mangled_name) {
  int status = 0;
  char *demangled =
      abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
  string result((status == 0 && demangled != NULL) ? demangled : mangled_name);
  free(demangled);
  return result;
}

string debasify(const string &name) {
  static const string base = ".base";
  if (boost::algorithm::ends_with(name, base)) {
    return name.substr(0, name.size() - base.size());
  } else {
    return name;
  }
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
}//namespace psr