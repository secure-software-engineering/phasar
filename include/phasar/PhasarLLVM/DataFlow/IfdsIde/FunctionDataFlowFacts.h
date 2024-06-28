#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace psr {

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
  FunctionDataFlowFacts();

  // insert a set of data flow facts
  void insertSet(const std::string &FuncKey, const uint32_t &Index, const std::vector<DataFlowFact> &OutSet) {
    if (this->fdff.find (FuncKey) != this->fdff.end ()) {
      this->fdff[FuncKey][Index] = OutSet;
    }
    
    else {
      std::unordered_map<uint32_t, std::vector<DataFlowFact>> Tmp;
      Tmp[Index] = OutSet;
      this->fdff.insert ({FuncKey, Tmp});
    }
  }

  //insert a single data flow fact
  void addElement(const std::string &FuncKey, const uint32_t &Index, const DataFlowFact &Out) {
    if (this->fdff.find (FuncKey) != this->fdff.end ()) {
          this->fdff[FuncKey][Index].emplace_back(Out);
    }
    
    else {
      std::unordered_map<uint32_t, std::vector<DataFlowFact>> Tmp;
      Tmp[Index] = {Out};
      this->fdff.insert ({FuncKey, Tmp});
    }
  }

  //get out set for a function an the parameter index
  const std::vector<DataFlowFact> &getDataFlowFacts(const std::string &FuncKey, uint32_t &Index) const {
    auto const It = this->fdff.find (FuncKey);
    if (It != this->fdff.end()) {
      auto const Itt = It->second.find (Index);
      return Itt->second;
    }

    const static std::vector<DataFlowFact> Empty = {};
    return Empty;
  }

  //prints the data structure of given keys to terminal

private:
  const auto &getDataFlowFactsOrEmpty(const std::string &FuncKey) const {
    auto const it = this->fdff.find(FuncKey);
    if (it != this->fdff.end()) {
      return it->second;
    }

    static std::unordered_map<uint32_t, std::vector<DataFlowFact>> Empty;
    return Empty;
  }

  // TODO: Remove public and use stable API instead!
public:
  std::unordered_map<std::string,
                     std::unordered_map<uint32_t, std::vector<DataFlowFact>>>
      fdff;
};
} // namespace psr
