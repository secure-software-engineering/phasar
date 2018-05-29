/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ObservedCallingContexts.h
 *
 *  Created on: 14.06.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_OBSERVEDCALLINGCONTEXTS_H_
#define ANALYSIS_IFDS_IDE_OBSERVEDCALLINGCONTEXTS_H_

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using namespace std;
namespace psr{

class ObservedCallingContexts {
private:
  // Maps a function to the set of contexts that have been recognized so far
  map<string, set<vector<bool>>> ObservedCTX;

public:
  ObservedCallingContexts() = default;
  ~ObservedCallingContexts() = default;
  void addObservedCTX(string FName, vector<bool> CTX);
  bool containsCTX(string FName);
  set<vector<bool>> getObservedCTX(string FName);
  void print();
};
}//namespace psr

#endif
