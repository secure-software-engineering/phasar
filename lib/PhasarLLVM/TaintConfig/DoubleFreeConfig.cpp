
#include "phasar/PhasarLLVM/TaintConfig/DoubleFreeConfig.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/InstrTypes.h"

psr::LLVMTaintConfig psr::getDoubleFreeConfig() {
  auto SourceSinkCB = [](const llvm::Instruction *Inst) {
    static constexpr llvm::StringLiteral FreeFunNames[] = {
        "free",
        "_ZdlPv",
        "_ZdaPv",
    };

    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        llvm::is_contained(FreeFunNames,
                           Call->getCalledFunction()->getName())) {
      Ret.insert(Call->getArgOperand(0));
    }
    return Ret;
  };

  return LLVMTaintConfig(SourceSinkCB, SourceSinkCB);
}
