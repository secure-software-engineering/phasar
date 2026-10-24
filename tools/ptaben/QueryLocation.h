#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "AliasQueryType.h"
#include "QueryId.h"

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr::ptaben {

struct QueryLocation {
  QueryId Id{};
  const llvm::Instruction *Inst{};
  AliasQueryType QueryType{};

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const QueryLocation &Result);
};
} // namespace psr::ptaben
