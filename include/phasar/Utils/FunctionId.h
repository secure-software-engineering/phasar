#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/StrongTypeDef.h"

#include <cstdint>

PHASAR_STRONG_TYPEDEF(psr, uint32_t, FunctionId);

namespace psr {
std::string to_string(FunctionId FId);

template <typename F> using FunctionCompressor = Compressor<F, FunctionId>;
} // namespace psr
