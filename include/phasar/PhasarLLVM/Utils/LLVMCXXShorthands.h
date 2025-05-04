#ifndef PHASAR_PHASARLLVM_UTILS_LLVMCXXSHORTHANDS_H
#define PHASAR_PHASARLLVM_UTILS_LLVMCXXSHORTHANDS_H
// This contains LLVM IR Shorthands specific to C++
// See
// https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/object-oriented-constructs/classes.html
// for examples

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace psr {
bool isTouchVTableInst(const llvm::StoreInst *Store);
} // namespace psr

#endif
