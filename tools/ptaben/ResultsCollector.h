#pragma once

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
