#include "phasar/DataFlow/IfdsIde/Solver/EdgeFunctionCacheStats.h"

#include "llvm/Support/raw_ostream.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const EdgeFunctionCacheStats &S) {
  OS << "EdgeFunctionCache Sizes:\n";
  OS << "> NormalAndCtrEFCache:\t" << S.NormalAndCtrEFCacheCumulSize << '\n';
  OS << ">> With EquivalenceClassMap:\t"
     << S.NormalAndCtrEFCacheCumulSizeReduced << '\n';
  OS << ">> Avg EF per CFG Edge:\t" << S.AvgEFPerEdge << '\n';
  OS << ">> Med EF per CFG Edge:\t" << S.MedianEFPerEdge << '\n';
  OS << ">> Max EF per CFG Edge:\t" << S.MaxEFPerEdge << '\n';
  OS << "> CallEFCache:\t\t" << S.CallEFCacheCumulSize << '\n';
  OS << "> RetEFCache:\t\t" << S.RetEFCacheCumulSize << '\n';
  OS << "> SummaryEFCache:\t" << S.SummaryEFCacheCumulSize << '\n';
  return OS;
}
