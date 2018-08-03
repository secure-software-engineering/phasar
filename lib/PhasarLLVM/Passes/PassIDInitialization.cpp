/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/Passes/ExampleModulePass.h>
#include <phasar/PhasarLLVM/Passes/GeneralStatisticsPass.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>

using namespace std;
using namespace psr;

namespace psr {

// Initialize the module passes ID's that we are using
char ExampleModulePass::ID = 0;
char GeneralStatisticsPass::ID = 42;
char ValueAnnotationPass::ID = 13;

} // namespace psr
