#pragma once

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
};
} // namespace psr::ptaben
