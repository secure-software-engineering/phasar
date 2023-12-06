#include "phasar/DataFlow/IfdsIde/EdgeFunctionStats.h"

#include "llvm/ADT/STLExtras.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const EdgeFunctionStats &S) {
  static constexpr std::array<llvm::StringRef, 5> EFKind{
      "Normal", "Call", "Return", "CallToReturn", "Summary"};
  static constexpr std::array<llvm::StringRef, 3> AllocKind{
      "SmallObjectOptimized", "DefaultHeapAllocated", "CustomHeapAllocated"};

  size_t Ctr = 0;
  for (const auto &[UEF, Count] : llvm::zip(S.UniqueEFCount, S.TotalEFCount)) {
    OS << "Kind: " << EFKind[Ctr] << ":\n";
    Ctr++;

    OS << "  Total # EdgeFunctions: " << Count << '\n';
    OS << "  Unique EdgeFunctions: " << UEF << '\n';
  }

  for (auto [Count, Kind] : llvm::zip(S.PerAllocCount, AllocKind)) {
    OS << Kind << ": " << Count << '\n';
  }

  return OS;
}
