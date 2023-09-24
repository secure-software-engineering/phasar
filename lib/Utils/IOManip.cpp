#include "phasar/Utils/IOManip.h"

#include "llvm/Support/raw_ostream.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, BoolAlpha BA) {
  return OS << (BA.Value ? "true" : "false");
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, Flush) {
  OS.flush();
  return OS;
}
