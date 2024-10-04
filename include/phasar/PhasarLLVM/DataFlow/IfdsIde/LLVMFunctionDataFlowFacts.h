#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"
#include "phasar/Utils/DefaultValue.h"

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"

#include <unordered_map>
#include <vector>

namespace psr::library_summary {

class LLVMFunctionDataFlowFacts;
[[nodiscard]] LLVMFunctionDataFlowFacts
readFromFDFF(const FunctionDataFlowFacts &Fdff, const LLVMProjectIRDB &Irdb);

class LLVMFunctionDataFlowFacts {
public:
  LLVMFunctionDataFlowFacts() noexcept = default;
  using ParamaterMappingTy = FunctionDataFlowFacts::ParamaterMappingTy;

  /// insert a set of data flow facts
  void insertSet(const llvm::Function *Fun, uint32_t Index,
                 std::vector<DataFlowFact> OutSet) {

    LLVMFdff[Fun].try_emplace(Index, std::move(OutSet));
  }
  void insertSet(const llvm::Function *Fun, const llvm::Argument *Arg,
                 std::vector<DataFlowFact> OutSet) {

    insertSet(Fun, Arg->getArgNo(), std::move(OutSet));
  }

  void addElement(const llvm::Function *Fun, uint32_t Index, DataFlowFact Out) {
    LLVMFdff[Fun][Index].emplace_back(Out);
  }
  void addElement(const llvm::Function *Fun, const llvm::Argument *Arg,
                  DataFlowFact Out) {
    addElement(Fun, Arg->getArgNo(), Out);
  }

  [[nodiscard]] bool contains(const llvm::Function *Fn) {
    return LLVMFdff.count(Fn);
  }

  [[nodiscard]] const std::vector<DataFlowFact> &
  getFacts(const llvm::Function *Fun, uint32_t Index) {
    auto Iter = LLVMFdff.find(Fun);
    if (Iter != LLVMFdff.end()) {
      return Iter->second[Index];
    }
    return getDefaultValue<std::vector<DataFlowFact>>();
  }
  [[nodiscard]] const std::vector<DataFlowFact> &
  getFacts(const llvm::Function *Fun, const llvm::Argument *Arg) {
    return getFacts(Fun, Arg->getArgNo());
  }

  [[nodiscard]] const ParamaterMappingTy &
  getFactsForFunction(const llvm::Function *Fun) {
    auto Iter = LLVMFdff.find(Fun);
    if (Iter != LLVMFdff.end()) {
      return Iter->second;
    }
    return getDefaultValue<ParamaterMappingTy>();
  }

  friend LLVMFunctionDataFlowFacts
  readFromFDFF(const FunctionDataFlowFacts &Fdff, const LLVMProjectIRDB &Irdb);

private:
  std::unordered_map<const llvm::Function *, ParamaterMappingTy> LLVMFdff;
};
} // namespace psr::library_summary
