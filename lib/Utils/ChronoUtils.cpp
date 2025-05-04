#include "phasar/Utils/ChronoUtils.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, const hms &HMS) {
  return OS << llvm::format("%.2ld:%.2ld:%.2ld:%.6ld", HMS.Hours.count(),
                            HMS.Minutes.count(), HMS.Seconds.count(),
                            HMS.Micros.count());
}
