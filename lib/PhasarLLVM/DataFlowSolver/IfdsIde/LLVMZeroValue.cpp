#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"

namespace psr {
LLVMZeroValue::LLVMZeroValue(llvm::Module &Mod)
    : llvm::GlobalVariable(Mod, llvm::Type::getIntNTy(Mod.getContext(), 2),
                           true,
                           llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                           llvm::ConstantInt::get(Mod.getContext(),
                                                  llvm::APInt(/*nbits*/ 2,
                                                              /*value*/ 0,
                                                              /*signed*/ true)),
                           LLVMZeroValueInternalName) {
  setAlignment(llvm::MaybeAlign(4));
}
} // namespace psr
