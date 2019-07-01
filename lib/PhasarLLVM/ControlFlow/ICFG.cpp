/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ICFG.cpp
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#include <iostream>
#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>

using namespace psr;
using namespace std;

namespace psr {

ostream &operator<<(ostream &os, const CallGraphAnalysisType &CGA) {
  return os << wise_enum::to_string(CGA);
}
} // namespace psr
