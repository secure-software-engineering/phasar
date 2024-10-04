#include "phasar/Utils/DefaultValue.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

namespace psr::library_summary {

struct Parameter {
  uint16_t Index{};
};

struct ReturnValue {};

struct DataFlowFact {
  DataFlowFact(Parameter Param) noexcept : Fact(Param) {}
  DataFlowFact(ReturnValue Ret) noexcept : Fact(Ret) {}

  std::variant<Parameter, ReturnValue> Fact;
};

class FunctionDataFlowFacts {
public:
  using ParamaterMappingTy =
      std::unordered_map<uint32_t, std::vector<DataFlowFact>>;

  FunctionDataFlowFacts() noexcept = default;

  // insert a set of data flow facts
  void insertSet(llvm::StringRef FuncKey, uint32_t Index,
                 std::vector<DataFlowFact> OutSet) {
    Fdff[FuncKey].try_emplace(Index, std::move(OutSet));
  }

  // insert a single data flow fact
  void addElement(llvm::StringRef FuncKey, uint32_t Index, DataFlowFact Out) {
    Fdff[FuncKey][Index].emplace_back(Out);
  }

  // get outset for a function an the parameter index
  [[nodiscard]] const std::vector<DataFlowFact> &
  getDataFlowFacts(llvm::StringRef FuncKey, uint32_t Index) const {
    auto It = Fdff.find(FuncKey);
    if (It != Fdff.end()) {
      auto Itt = It->second.find(Index);
      return Itt->second;
    }

    return getDefaultValue<std::vector<DataFlowFact>>();
  }

  [[nodiscard]] auto begin() const noexcept { return Fdff.begin(); }
  [[nodiscard]] auto end() const noexcept { return Fdff.end(); }

  [[nodiscard]] size_t size() const noexcept { return Fdff.size(); }
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

private:
  [[nodiscard]] const auto &
  getDataFlowFactsOrEmpty(llvm::StringRef FuncKey) const {
    auto It = Fdff.find(FuncKey);
    if (It != Fdff.end()) {
      return It->second;
    }

    return getDefaultValue<ParamaterMappingTy>();
  }

  llvm::StringMap<ParamaterMappingTy> Fdff;
};

} // namespace psr::library_summary
