/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LatticeDomain.h>

using namespace psr;

namespace psr {

std::ostream &operator<<(std::ostream &OS, [[maybe_unused]] const Top &T) {
  return OS << "Top";
}

std::ostream &operator<<(std::ostream &OS, [[maybe_unused]] const Bottom &B) {
  return OS << "Bottom";
}

} // namespace psr
