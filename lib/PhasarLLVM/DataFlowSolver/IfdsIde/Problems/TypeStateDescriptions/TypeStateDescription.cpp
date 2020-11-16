#include "llvm/ADT/DenseMap.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

namespace psr {

llvm::StringRef TypeStateDescription::stateToUnownedString(State S) const {
  static llvm::SmallDenseMap<State, std::string, 8> cache;
  auto it = cache.find(S);
  if (it == cache.end()) {
    auto [it_, unused] = cache.insert({S, stateToString(S)});
    it = std::move(it_);
  }
  return it->getSecond();
}

} // namespace psr
