#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Pointer/AliasResult.h"

#include "llvm/Support/raw_ostream.h"

#include "QueryId.h"

namespace psr::ptaben {

struct PTAResult {
  QueryId Query{};
  AliasResult Result{};

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, PTAResult Res) {
    return OS << uint64_t(Res.Query) << ", " << to_string(Res.Result);
  }
};

} // namespace psr::ptaben
