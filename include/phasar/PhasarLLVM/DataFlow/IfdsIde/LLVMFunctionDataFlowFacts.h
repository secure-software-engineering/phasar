#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"
#include "phasar/Utils/DefaultValue.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

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
  LLVMFunctionDataFlowFacts() = default;

  // insert a set of data flow facts
  void insertSet(const llvm::Function *Fun, const llvm::Argument *Arg,
                 std::vector<LLVMDataFlowFact> OutSet) {

    LLVMFdff[Fun].try_emplace(Arg, std::move(OutSet));
  }

  void addElement(const llvm::Function *Fun, const llvm::Argument *Arg,
                  LLVMDataFlowFact Out) {
    LLVMFdff[Fun][Arg].emplace_back(Out);
  }

  bool contains(const llvm::Function *Fn) { return LLVMFdff.count(Fn); }

  [[nodiscard]] const std::vector<LLVMDataFlowFact> &
  getFacts(const llvm::Function *Fun, const llvm::Argument *Arg) {
    auto Iter = LLVMFdff.find(Fun);
    if (Iter != LLVMFdff.end()) {
      return Iter->second[Arg];
    }
    return getDefaultValue<std::vector<LLVMDataFlowFact>>();
  }

  [[nodiscard]] const std::unordered_map<const llvm::Argument *,
                                         std::vector<LLVMDataFlowFact>> &
  getFactsForFunction(const llvm::Function *Fun) {
    auto Iter = LLVMFdff.find(Fun);
    if (Iter != LLVMFdff.end()) {
      return Iter->second;
    }
    return getDefaultValue<std::unordered_map<const llvm::Argument *,
                                              std::vector<LLVMDataFlowFact>>>();
  }

  static LLVMFunctionDataFlowFacts
  readFromFDFF(const FunctionDataFlowFacts &Fdff, const LLVMProjectIRDB &Irdb);

private:
  std::unordered_map<
      const llvm::Function *,
      std::unordered_map<const llvm::Argument *, std::vector<LLVMDataFlowFact>>>
      LLVMFdff;
};
} // namespace psr
