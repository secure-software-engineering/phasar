#ifndef INCLUDE_PHASAR_UTILS_LLVMCXXSHORTHANDS_H_
#define INCLUDE_PHASAR_UTILS_LLVMCXXSHORTHANDS_H_
// This contains LLVM IR Shorthands specific to C++
// See
// https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/object-oriented-constructs/classes.html
// for examples

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace psr {
bool isTouchVTableInst(const llvm::StoreInst *Store);
}
#endif // INCLUDE_PHASAR_UTILS_LLVMCXXSHORTHANDS_H_
