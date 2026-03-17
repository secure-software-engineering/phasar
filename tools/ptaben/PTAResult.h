#pragma once

#include "phasar/Pointer/AliasResult.h"

#include "llvm/Support/raw_ostream.h"

#include "QueryId.h"

namespace psr::ptaben {

struct PTAResult {
  QueryId Query{};
  AliasResult Result{};
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, PTAResult Res);

} // namespace psr::ptaben
