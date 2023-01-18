#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"

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

const LLVMZeroValue *LLVMZeroValue::getInstance() {
  auto GetZeroMod = [] {
    static llvm::LLVMContext Ctx;
    static llvm::Module Mod("zero_module", Ctx);
    ModulesToSlotTracker::setMSTForModule(&Mod);
    return &Mod;
  };
  static auto *ZV = new LLVMZeroValue(*GetZeroMod());
  return ZV;
}

} // namespace psr
