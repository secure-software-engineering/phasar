/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_NLOHMANNLOGGING_H
#define PHASAR_UTILS_NLOHMANNLOGGING_H

#include "llvm/Support/Format.h"         // -- ---------v
#include "llvm/Support/FormatVariadic.h" // -- for overload resolution
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

namespace psr {
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const nlohmann::json &J);
} // namespace psr

#endif /* PHASAR_UTILS_NLOHMANNLOGGING_H */
