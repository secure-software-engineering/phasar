#pragma once

#include "llvm/Support/raw_ostream.h"

namespace psr {
struct QueryLocation;
struct QuerySrcCodeLocation;
class QuerySerializer {
public:
  explicit QuerySerializer(llvm::raw_ostream *OS);

  void handleQuery(const QueryLocation &QueryLoc,
                   const QuerySrcCodeLocation &QuerySrcLoc);

private:
  llvm::raw_ostream *OS{};
};
} // namespace psr
