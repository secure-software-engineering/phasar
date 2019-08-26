/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>
using namespace psr;
using namespace std;

namespace psr {

ostream &operator<<(ostream &os, const DataFlowAnalysisType &D) {
  return os << wise_enum::to_string(D);
}
} // namespace psr
