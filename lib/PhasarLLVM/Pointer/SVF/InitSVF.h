#pragma once

/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/Support/Compiler.h"

namespace psr {
class LLVMProjectIRDB;

LLVM_LIBRARY_VISIBILITY void initializeSVF();
LLVM_LIBRARY_VISIBILITY void initSVFModule(psr::LLVMProjectIRDB &IRDB);
} // namespace psr
