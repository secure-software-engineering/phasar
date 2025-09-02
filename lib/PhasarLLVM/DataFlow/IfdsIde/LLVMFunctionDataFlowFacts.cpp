#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFunctionDataFlowFacts.h"

using namespace psr;
using namespace psr::library_summary;

LLVMFunctionDataFlowFacts
library_summary::readFromFDFF(const FunctionDataFlowFacts &Fdff,
                              const LLVMProjectIRDB &Irdb) {
  LLVMFunctionDataFlowFacts Llvmfdff;
  Llvmfdff.LLVMFdff.reserve(Fdff.size());

  for (const auto &It : Fdff) {
    if (const llvm::Function *Fun = Irdb.getFunction(It.first())) {
      Llvmfdff.LLVMFdff.try_emplace(Fun, It.second);
    }
  }
  return Llvmfdff;
}
