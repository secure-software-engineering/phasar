#pragma once

#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstrTypes.h"

namespace psr::unittest {
inline bool isDummySrcFun(llvm::StringRef Name) {
  return Name == "_Z6sourcev" || Name == "source";
}
inline bool isDummySinkFun(llvm::StringRef Name) {
  return Name == "_Z4sinki" || Name == "sink";
}
inline LLVMTaintConfig getDefaultConfig() {
  auto SourceCB = [](const llvm::Instruction *Inst) {
    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        isDummySrcFun(Call->getCalledFunction()->getName())) {
      Ret.insert(Call);
    }
    return Ret;
  };
  auto SinkCB = [](const llvm::Instruction *Inst) {
    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        isDummySinkFun(Call->getCalledFunction()->getName())) {
      assert(Call->arg_size() > 0);
      Ret.insert(Call->getArgOperand(0));
    }
    return Ret;
  };
  return LLVMTaintConfig(std::move(SourceCB), std::move(SinkCB));
}

inline LLVMTaintConfig getDoubleFreeConfig() {
  auto SourceCB = [](const llvm::Instruction *Inst) {
    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        Call->getCalledFunction()->getName() == "free") {
      Ret.insert(Call->getArgOperand(0));
    }
    return Ret;
  };

  return LLVMTaintConfig(SourceCB, SourceCB);
}
} // namespace psr::unittest
