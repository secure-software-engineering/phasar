#pragma once

#include "llvm/Support/Compiler.h"

namespace psr {
class LLVMProjectIRDB;

LLVM_LIBRARY_VISIBILITY void initializeSVF();
LLVM_LIBRARY_VISIBILITY void initSVFModule(psr::LLVMProjectIRDB &IRDB);
} // namespace psr
