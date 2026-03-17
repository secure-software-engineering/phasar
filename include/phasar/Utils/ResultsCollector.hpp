#pragma once

#include "phasar/Utils/PTAResult.hpp"

#include "llvm/Support/raw_ostream.h"

namespace psr {
class ResultCollector {
public:
  explicit ResultCollector(llvm::raw_ostream *OS, llvm::StringRef ResultName);

  void handleResult(PTAResult Result);

private:
  llvm::raw_ostream *OS{};
};
}; // namespace psr
