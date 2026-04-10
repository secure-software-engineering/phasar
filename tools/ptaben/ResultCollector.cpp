#include "ResultsCollector.h"

using namespace psr::ptaben;

ResultCollector::ResultCollector(llvm::raw_ostream *OS,
                                 llvm::StringRef ResultName)
    : OS(OS) {
  assert(OS != nullptr);
  *OS << "Query, " << ResultName << '\n';
}

void ResultCollector::handleResult(PTAResult Result) { *OS << Result << '\n'; }
