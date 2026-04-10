#include "PTAResult.h"

llvm::raw_ostream &psr::ptaben::operator<<(llvm::raw_ostream &OS,
                                           PTAResult Res) {
  return OS << uint64_t(Res.Query) << ", " << to_string(Res.Result);
}
