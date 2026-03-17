#include "phasar/Utils/AliasQueryType.h"
#include "phasar/Utils/PTAUtils.hpp"
#include "phasar/Utils/QuerySer.hpp"

using namespace psr;

QuerySerializer::QuerySerializer(llvm::raw_ostream *OS) : OS(OS) {
  assert(OS != nullptr);
  *OS << "Query, QueryType, FileName, Index\n";
}

void QuerySerializer::handleQuery(const QueryLocation &QueryLoc,
                                  const QuerySrcCodeLocation &QuerySrcLoc) {
  *OS << uint64_t(QueryLoc.Id) << ", " << to_string(QueryLoc.QueryType) << ", "
      << QuerySrcLoc.FileName << ", " << QuerySrcLoc.QueryIndex << '\n';
}
