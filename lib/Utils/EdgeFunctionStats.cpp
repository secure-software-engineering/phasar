#include "phasar/DataFlow/IfdsIde/EdgeFunctionStats.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Format.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const EdgeFunctionStats &S) {
  static constexpr std::array<llvm::StringRef, 5> EFKind{
      "Normal", "Call", "Return", "CallToReturn", "Summary"};
  static constexpr std::array<llvm::StringRef, 3> AllocKind{
      "SmallObjectOptimized", "DefaultHeapAllocated", "CustomHeapAllocated"};

  OS << "Cached Edge Functions:\n";

  size_t Ctr = 0;
  for (const auto &[UEF, Count] : llvm::zip(S.UniqueEFCount, S.TotalEFCount)) {
    OS << "  Kind: " << EFKind[Ctr] << ":\n";
    Ctr++;

    OS << "    Total #EdgeFunctions:\t" << Count << '\n';
    OS << "    Unique EdgeFunctions:\t" << UEF << '\n';
  }

  OS << "  AllocationPolicy:\n";
  for (auto [Count, Kind] : llvm::zip(S.PerAllocCount, AllocKind)) {
    OS << "    " << Kind << ":\t" << Count << '\n';
  }

  OS << "  Depth:\n";
  OS << "    Max Depth:\t\t\t" << S.MaxDepth << '\n';
  OS << "    Avg Depth:\t\t\t" << llvm::format("%g\n", S.AvgDepth);
  OS << "    Avg Unique Depth:\t\t" << llvm::format("%g\n", S.AvgUniqueDepth);

  OS << "Jump Functions:\n";
  OS << "  Total #JumpFunctions:\t\t" << S.TotalNumJF << '\n';
  OS << "  Unique JumpFunctions:\t\t" << S.UniqueNumJF << '\n';
  OS << "  JumpFunctions Objects:\t" << S.NumJFObjects << '\n';

  OS << "  AllocationPolicy:\n";
  for (auto [Count, Kind] : llvm::zip(S.PerAllocJFCount, AllocKind)) {
    OS << "    " << Kind << ":\t" << Count << '\n';
  }

  OS << "  Depth:\n";
  OS << "    Max Depth:\t\t\t" << S.MaxJFDepth << '\n';
  OS << "    Avg Depth:\t\t\t" << llvm::format("%g\n", S.AvgJFDepth);
  OS << "    Avg Unique Depth:\t\t" << llvm::format("%g\n", S.AvgUniqueJFDepth);
  OS << "    Avg JF Object Depth:\t" << llvm::format("%g\n", S.AvgJFObjDepth);

  return OS;
}
