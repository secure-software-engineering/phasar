#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"
// #include nlohman.json oder YAML
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm-14/llvm/ADT/StringMap.h>
#include <llvm-14/llvm/ADT/StringRef.h>
#include <llvm-14/llvm/Support/Path.h>
#include <llvm-14/llvm/Support/raw_ostream.h>

using namespace psr;
using llvm::yaml::Input;
using llvm::yaml::IO;
using llvm::yaml::MappingTraits;
using llvm::yaml::Output;
using llvm::yaml::SequenceTraits;

template <>
struct MappingTraits<
    std::unordered_map<uint32_t, std::vector<DataFlowFact>>::iterator> {
  static void
  mapping(IO &Io,
          std::unordered_map<uint32_t, std::vector<DataFlowFact>>::iterator
              InnerElemPtr) {
    for (const auto &Dff : InnerElemPtr->second) {
      if (const Parameter *Param = std::get_if<Parameter>(&Dff.Fact)) {
        Io.mapOptional(std::to_string(InnerElemPtr->first).c_str(),
                       Param->Index);
      } else {
        Io.mapOptional(std::to_string(InnerElemPtr->first).c_str(),
                       "ReturnValue");
      }
    }
  }
};

template <>
struct MappingTraits<llvm::StringMapConstIterator<
    std::unordered_map<uint32_t, std::vector<DataFlowFact>>>> {
  static void
  mapping(IO &Io, llvm::StringMapConstIterator<
                      std::unordered_map<uint32_t, std::vector<DataFlowFact>>>
                      ElemPtr) {
    struct MappingTraits<
        std::unordered_map<uint32_t, std::vector<DataFlowFact>>>
        InnerElem; //{ElemPtr->second.begin()};
    Io.mapOptional(((ElemPtr->first()).str()).c_str(), "");
    Io.mapOptional(std::to_string(ElemPtr->second.begin()->first).c_str(), "");
    // InnerElem.mapping(Io, ElemPtr->second.begin());
  }
};

template <> struct SequenceTraits<std::vector<DataFlowFact>> {
  static size_t size(IO &Io, std::vector<DataFlowFact> &List) {
    return List.size();
  }
  static DataFlowFact &element(IO &Io, std::vector<DataFlowFact> &List,
                               size_t Index) {
    if (Index < List.size()) {
      return List[Index];
    }
    List.emplace_back();
    return List[List.size() - 1];
  }
};

void serialize(const FunctionDataFlowFacts &Fdff, llvm::raw_ostream &OS) {
  /*std::string Yaml;
  for (const auto &ItOuter : Fdff) {
    OS << "- "
       << "ItOuter.first"
       << ":"
       << "\n";
    for (const auto &ItInner : ItOuter.second) {
      OS << "    "
         << "- " << std::to_string(ItInner.first) << ":"
         << "\n";
      for (DataFlowFact DFF : ItInner.second) {
        if (const Parameter *Param = std::get_if<Parameter>(&DFF.Fact)) {
          OS << "        "
             << "- " << std::to_string(Param->Index) << "\n";
        } else {
          OS << "        "
             << "- "
             << "ReturnValue"
             << "\n";
        }
        // Output yout(OS);
      }
    }
  }*/
}

[[nodiscard]] FunctionDataFlowFacts deserialize(llvm::raw_ostream &OS) {}
