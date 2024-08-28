#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFunctionDataFlowFacts.h"

#include <variant>

using namespace psr;

static LLVMFunctionDataFlowFacts readFromFDFF(const FunctionDataFlowFacts &Fdff,
                                              const LLVMProjectIRDB &Irdb) {
  LLVMFunctionDataFlowFacts Llvmfdff;
  for (const auto &It : Fdff) {
    const llvm::Function *Fun = Irdb.getFunction(It.first());
    if (Fun == nullptr) {
      continue;
    }
    for (const auto [ArgIndex, OutSet] : It.second) {
      const llvm::Argument *Arg = Fun->getArg(ArgIndex);
      for (const auto &I : OutSet) {
        if (std::holds_alternative<ReturnValue>(I.Fact)) {
          Llvmfdff.addElement(Fun, Arg, LLVMReturnValue{});
        } else if (const auto *Param = std::get_if<Parameter>(&I.Fact)) {
          if (Param->Index < Fun->arg_size()) {
            LLVMParameter LLVMParam = {Fun->getArg(Param->Index)};
            Llvmfdff.addElement(Fun, Arg, LLVMParam);
          }
        }
      }
    }
  }
  return Llvmfdff;
}
