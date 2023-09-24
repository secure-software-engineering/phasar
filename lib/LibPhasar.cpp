/******************************************************************************
 * Copyright (c) 2023 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

// Make sure, clangd always picks the right source file to infer the
// compile-commands for IDESolverImpl.h. Otherwise this leads to strange eror
// squiggles
#include "phasar/DataFlow/IfdsIde/Solver/detail/IDESolverImpl.h"

#include "phasar.h"
