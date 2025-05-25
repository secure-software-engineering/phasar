#pragma once

#include "llvm/Support/Compiler.h"

namespace SVF {
class SVFModule;
} // namespace SVF

namespace psr {
class LLVMProjectIRDB;

LLVM_LIBRARY_VISIBILITY void initializeSVF();
LLVM_LIBRARY_VISIBILITY SVF::SVFModule *
initSVFModule(psr::LLVMProjectIRDB &IRDB);
} // namespace psr
