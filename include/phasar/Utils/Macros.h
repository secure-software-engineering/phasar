/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef UTILS_H_
#define UTILS_H_

#include "IO.h"
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <cxxabi.h>
#include <fstream>
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <sstream>
#include <string>
using namespace std;

namespace psr{

#define MYDEBUG

#define HEREANDNOW                                                             \
  cerr << "file: " << __FILE__ << " line: " << __LINE__                        \
       << " function: " << __func__ << endl;

#define DIE_HARD exit(-1);

#define UNRECOVERABLE_CXX_ERROR_COND(BOOL, STRING)                             \
  if (!BOOL) {                                                                 \
    HEREANDNOW;                                                                \
    cerr << STRING << endl;                                                    \
    exit(-1);                                                                  \
  }

#define UNRECOVERABLE_CXX_ERROR_UNCOND(STRING)                                 \
  HEREANDNOW;                                                                  \
  cerr << STRING << endl;                                                      \
  exit(-1);

string cxx_demangle(const string &mangled_name);

string debasify(const string &name);

bool isMangled(const string &name);

vector<string> splitString(const string &str, const string &delimiter);

template <typename T> set<set<T>> computePowerSet(const set<T> &s) {
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
  set<set<T>> powerset;
  for (size_t i = 0; i < (1 << s.size()); ++i) {
    set<T> subset;
    for (size_t j = 0; j < s.size(); ++j) {
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

ostream &operator<<(ostream &os, const vector<bool> &bits);
}//namespace psr

#endif
