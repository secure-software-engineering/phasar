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

#include "PTAResult.h"

namespace psr::ptaben {
class ResultCollector {
public:
  explicit ResultCollector(llvm::raw_ostream *OS, llvm::StringRef ResultName);

  void handleResult(PTAResult Result);

private:
  llvm::raw_ostream *OS{};
};
}; // namespace psr::ptaben
