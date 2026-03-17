#pragma once

#include "phasar/Pointer/AliasResult.h"

#include "llvm/Support/raw_ostream.h"

#include "QueryId.hpp"

namespace psr {

struct PTAResult {
  QueryId Query{};
  AliasResult Result{};
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, PTAResult Res);

} // namespace psr
