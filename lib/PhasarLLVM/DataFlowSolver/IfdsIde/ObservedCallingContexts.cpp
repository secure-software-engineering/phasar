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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ObservedCallingContexts.h"

using namespace psr;
using namespace std;
namespace psr {

void ObservedCallingContexts::addObservedCTX(const string &FName,
                                             const vector<bool> &CTX) {
  ObservedCTX[FName].insert(CTX);
}

bool ObservedCallingContexts::containsCTX(const string &FName) {
  return ObservedCTX.find(FName) != ObservedCTX.end();
}

set<vector<bool>> ObservedCallingContexts::getObservedCTX(const string &FName) {
  return ObservedCTX[FName];
}

void ObservedCallingContexts::print() {
  for (auto &Entry : ObservedCTX) {
    cout << Entry.first << "\n";
    for (const auto &Ctx : Entry.second) {
      for_each(Ctx.begin(), Ctx.end(), [](bool B) { cout << B; });
    }
  }
}
} // namespace psr
