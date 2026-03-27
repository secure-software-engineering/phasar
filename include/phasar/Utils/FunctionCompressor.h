#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/StrongTypeDef.h"

#include "llvm/IR/Function.h"

#include <cstdint>

PHASAR_STRONG_TYPEDEF(psr, uint32_t, FunctionId);

namespace psr {
using FunctionCompressor = Compressor<const llvm::Function *, FunctionId>;

std::string to_string(FunctionId FId);
} // namespace psr
