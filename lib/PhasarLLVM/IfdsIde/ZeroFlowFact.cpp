/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include <phasar/PhasarLLVM/IfdsIde/ZeroFlowFact.h>

using namespace std;
using namespace psr;

namespace psr {

void ZeroFlowFact::print(ostream &os) const { os << "ZeroFlowFact"; }

FlowFact *ZeroFlowFact::getInstance() {
  static ZeroFlowFact ZeroFact;
  return &ZeroFact;
}

} // namespace psr
