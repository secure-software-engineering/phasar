/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"

namespace psr {

const std::set<llvm::StringRef> HeapAllocatingFunctions{
    "malloc", "calloc", "realloc", "_Znwm", "_Znam"};

} // namespace psr
