#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <cstdint>

namespace psr::ptaben {
enum class [[clang::enum_extensibility(open)]] QueryId : uint64_t {};

} // namespace psr::ptaben
