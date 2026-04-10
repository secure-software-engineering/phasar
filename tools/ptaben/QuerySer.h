#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/Support/raw_ostream.h"

#include "PTAUtils.h"
#include "QueryLocation.h"

namespace psr::ptaben {

class QuerySerializer {
public:
  explicit QuerySerializer(llvm::raw_ostream *OS);

  void handleQuery(const QueryLocation &QueryLoc,
                   const QuerySrcCodeLocation &QuerySrcLoc);

private:
  llvm::raw_ostream *OS{};
};
} // namespace psr::ptaben
