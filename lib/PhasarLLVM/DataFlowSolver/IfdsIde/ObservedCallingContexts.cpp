/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <iostream>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ObservedCallingContexts.h>

using namespace psr;
using namespace std;
namespace psr {

void ObservedCallingContexts::addObservedCTX(string FName, vector<bool> CTX) {
  ObservedCTX[FName].insert(CTX);
}

bool ObservedCallingContexts::containsCTX(string FName) {
  return ObservedCTX.find(FName) != ObservedCTX.end();
}

set<vector<bool>> ObservedCallingContexts::getObservedCTX(string FName) {
  return ObservedCTX[FName];
}

void ObservedCallingContexts::print() {
  for (auto &entry : ObservedCTX) {
    cout << entry.first << "\n";
    for (auto &ctx : entry.second) {
      for_each(ctx.begin(), ctx.end(), [](bool b) { cout << b; });
    }
  }
}
} // namespace psr
