#include "phasar/PhasarLLVM/ControlFlow/FunctionCompressor.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"

using namespace psr;

Compressor<const llvm::Function *, FunctionId>
psr::compressFunctions(const LLVMBasedCallGraph &CG,
                       llvm::ArrayRef<const llvm::Function *> EntryPoints) {
  Compressor<const llvm::Function *, FunctionId> Functions;
  Functions.reserve(CG.getNumVertexFunctions());
  llvm::SmallVector<const llvm::Function *> WL;
  WL.append(EntryPoints.begin(), EntryPoints.end());

  while (!WL.empty()) {
    const auto *Fn = WL.pop_back_val();

    auto Inserted = Functions.insert(Fn).second;
    if (!Inserted) {
      continue;
    }

    for (const auto &I : llvm::instructions(Fn)) {
      const auto *CS = llvm::dyn_cast<llvm::CallBase>(&I);
      if (!CS) {
        continue;
      }

      auto Callees = CG.getCalleesOfCallAt(CS);
      WL.append(Callees.begin(), Callees.end());
    }
  }

  return Functions;
}
