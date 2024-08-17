#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"

#include "llvm/IR/Function.h"

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

#include <llvm-14/llvm/ADT/StringRef.h>
#include <llvm-14/llvm/IR/Argument.h>

namespace psr {

struct LLVMParameter {
  const llvm::Argument *Index{};
};

struct LLVMReturnValue {};

struct LLVMDataFlowFact {
  LLVMDataFlowFact(LLVMParameter Param) noexcept : Fact(Param) {}
  LLVMDataFlowFact(LLVMReturnValue Ret) noexcept : Fact(Ret) {}

  std::variant<LLVMParameter, LLVMReturnValue> Fact;
};

class LLVMFunctionDataFlowFacts {
public:
  LLVMFunctionDataFlowFacts();

  // insert a set of data flow facts
  void insertSet(const llvm::Function *Fun, const llvm::Argument *Arg,
                 std::vector<LLVMDataFlowFact> OutSet) {

    LLVMFdff[Fun].try_emplace(Arg, std::move(OutSet));
  }

  void addElement(const llvm::Function *Fun, const llvm::Argument *Arg,
                  LLVMDataFlowFact Out) {
    LLVMFdff[Fun][Arg].emplace_back(Out);
  }

  bool contains(const llvm::Function *Fn) {
    return LLVMFdff.end() != LLVMFdff.find(Fn);
  }

  std::vector<LLVMDataFlowFact> getFacts(const llvm::Function *Fun,
                                         const llvm::Argument *Arg) {
    if (contains(Fun)) {
      return LLVMFdff[Fun][Arg];
    }
    std::vector<LLVMDataFlowFact> Empty;
    return Empty;
  }

  static LLVMFunctionDataFlowFacts
  readFromFDFF(const FunctionDataFlowFacts &Fdff, const LLVMProjectIRDB &Irdb) {
    LLVMFunctionDataFlowFacts Llvmfdff;
    for (const auto &It : Fdff) {
      const llvm::Function *Fun = Irdb.getFunction(It.first());
      for (auto [ArgIndex, OutSet] : It.second) {
        // for (const auto &Itt : It.second) {
        const llvm::Argument *Arg = Fun->getArg(ArgIndex);
        for (const auto &I : OutSet) {
          if (std::get_if<ReturnValue>(&I.Fact)) {
            LLVMReturnValue Ret;
            Llvmfdff.addElement(Fun, Arg, Ret);
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

private:
  std::unordered_map<
      const llvm::Function *,
      std::unordered_map<const llvm::Argument *, std::vector<LLVMDataFlowFact>>>
      LLVMFdff;
};
} // namespace psr
