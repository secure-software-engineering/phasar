/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * VTable.h
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_VTABLE_H_
#define ANALYSIS_VTABLE_H_

#include "json.hpp"
#include <algorithm>
#include <iostream>
#include <llvm/IR/Type.h>
#include <string>
#include <vector>
using namespace std;
using json = nlohmann::json;

class VTable {
private:
  vector<string> vtbl;

public:
  VTable() = default;
  virtual ~VTable() = default;
  string getFunctionByIdx(unsigned i);
  int getEntryByFunctionName(string fname) const;
  void addEntry(string entry);
  bool empty();
  vector<string>::iterator begin();
  vector<string>::const_iterator begin() const;
  vector<string>::iterator end();
  vector<string>::const_iterator end() const;
  vector<string> getVTable() const;
  friend ostream &operator<<(ostream &os, const VTable &t);
  json exportPATBCJSON();
};

#endif /* ANALYSIS_VTABLE_HH_ */
