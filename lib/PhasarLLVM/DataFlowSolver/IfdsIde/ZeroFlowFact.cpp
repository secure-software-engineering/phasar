/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ZeroFlowFact.h>

using namespace std;
using namespace psr;

namespace psr {

void ZeroFlowFact::print(ostream &os) const { os << "ZeroFlowFact"; }

bool ZeroFlowFact::equal_to(const FlowFact &FF) const { return false; }

bool ZeroFlowFact::less(const FlowFact &FF) const { return false; }

FlowFact *ZeroFlowFact::getInstance() {
  static ZeroFlowFact ZeroFact;
  return &ZeroFact;
}

} // namespace psr
