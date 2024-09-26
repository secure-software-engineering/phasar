#include "phasar/DataFlow/IfdsIde/Solver/FlowFunctionCacheStats.h"

#include "llvm/Support/raw_ostream.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const FlowFunctionCacheStats &S) {
  OS << "FlowFunctionCache Sizes:\n";
  OS << "> NormalFFCache:\t" << S.NormalFFCacheSize << '\n';
  OS << "> CallFFCache:\t\t" << S.CallFFCacheSize << '\n';
  OS << "> ReturnFFCache:\t" << S.ReturnFFCacheSize << '\n';
  OS << "> SimpleRetFFCache:\t" << S.SimpleRetFFCacheSize << '\n';
  OS << "> CallToRetFFCache:\t" << S.CallToRetFFCacheSize << '\n';
  OS << "> SummaryFFCache:\t" << S.SummaryFFCacheSize << '\n';
  return OS;
}
