#pragma once

#include <cstdint>

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr {
enum class QueryId : uint64_t;
enum class AliasQueryType;

struct QueryLocation {
  QueryId Id{};
  const llvm::Instruction *Inst{};
  AliasQueryType QueryType{};
};
} // namespace psr
