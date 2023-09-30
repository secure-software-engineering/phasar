#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

#include "nlohmann/json-schema.hpp"
#include "nlohmann/json.hpp"

#include <optional>
#include <stdexcept>

llvm::StringRef psr::to_string(TaintCategory Cat) noexcept {
  switch (Cat) {
  case TaintCategory::Source:
    return "Source";
  case TaintCategory::Sink:
    return "Sink";
  case TaintCategory::Sanitizer:
    return "Sanitizer";
  case TaintCategory::None:
    return "None";
  }
  llvm_unreachable("Invalid TaintCategory");
}

psr::TaintCategory psr::toTaintCategory(llvm::StringRef Str) noexcept {
  return llvm::StringSwitch<TaintCategory>(Str)
      .CaseLower("source", TaintCategory::Source)
      .CaseLower("sink", TaintCategory::Sink)
      .CaseLower("sanitizer", TaintCategory::Sanitizer)
      .Default(TaintCategory::None);
}

static std::optional<psr::FunctionData> loadFunc(const nlohmann::json &Func) {
  std::optional<psr::FunctionData> Data;
  Data.emplace();
  bool FunctionNonEmpty = false;

  if (auto NameIt = Func.find("name"); NameIt != Func.end()) {
    Data->Name = NameIt->get<std::string>();
    FunctionNonEmpty = true;
  }

  if (auto RetIt = Func.find("ret"); RetIt != Func.end()) {
    Data->ReturnCat = psr::toTaintCategory(RetIt->get<std::string>());
    if (Data->ReturnCat == psr::TaintCategory::None) {
      throw std::runtime_error(
          "Invalid taint category: '" + RetIt->get<std::string>() +
          "'; Must be one of 'source', 'sink' or 'sanitizer'");
    }
    FunctionNonEmpty = true;
  }

  auto ParamsIt = Func.find("params");
  if (ParamsIt != Func.end()) {
    const auto &Params = *ParamsIt;
    if (auto SrcIt = Params.find("source"); SrcIt != Params.end()) {
      for (const auto &Curr : *SrcIt) {
        Data->SourceValues.push_back(Curr.get<uint32_t>());
        FunctionNonEmpty = true;
      }
    }

    if (auto SinkIt = Params.find("sink"); SinkIt != Params.end()) {
      for (const auto &Curr : Func["params"]["sink"]) {
        if (Curr.is_number()) {
          Data->SinkValues.push_back(Curr.get<uint32_t>());
        } else if (Curr == "all") {
          Data->HasAllSinkParam = true;
        } else {
          throw std::runtime_error("[TaintConfigData::TaintConfigData()]: "
                                   "Unknown sink string parameter!");
        }
        FunctionNonEmpty = true;
      }
    }

    if (auto SanIt = Params.find("sanitizer"); SanIt != Params.end()) {
      for (const auto &Curr : *SanIt) {
        Data->SanitizerValues.push_back(Curr.get<uint32_t>());
        FunctionNonEmpty = true;
      }
    }
  }

  if (!FunctionNonEmpty) {
    Data.reset();
  }

  return Data;
}

static std::optional<psr::VariableData> loadVar(const nlohmann::json &Var) {
  std::optional<psr::VariableData> Data;
  Data.emplace();

  bool VarNonEmpty = false;
  if (auto LineIt = Var.find("line"); LineIt != Var.end()) {
    Data->Line = LineIt->get<int>();
    VarNonEmpty = true;
  }

  if (auto NameIt = Var.find("name"); NameIt != Var.end()) {
    Data->Name = NameIt->get<std::string>();
    VarNonEmpty = true;
  }

  if (auto ScopeIt = Var.find("scope"); ScopeIt != Var.end()) {
    Data->Scope = ScopeIt->get<std::string>();
    VarNonEmpty = true;
  }

  if (auto CatIt = Var.find("cat"); CatIt != Var.end()) {
    Data->Cat = psr::toTaintCategory(CatIt->get<std::string>());
    if (Data->Cat == psr::TaintCategory::None) {
      throw std::runtime_error(
          "Invalid taint category: '" + CatIt->get<std::string>() +
          "'; Must be one of 'source', 'sink' or 'sanitizer'");
    }
    VarNonEmpty = true;
  }

  if (!VarNonEmpty) {
    Data.reset();
  }

  return Data;
}

static void loadFunctions(const nlohmann::json &JSON,
                          std::vector<psr::FunctionData> &Into) {
  auto It = JSON.find("functions");
  if (It == JSON.end()) {
    return;
  }

  for (const auto &Func : *It) {
    if (auto FuncData = loadFunc(Func)) {
      Into.push_back(std::move(*FuncData));
    }
  }
}

static void loadVariables(const nlohmann::json &JSON,
                          std::vector<psr::VariableData> &Into) {

  auto It = JSON.find("variables");
  if (It == JSON.end()) {
    return;
  }

  for (const auto &Var : *It) {
    if (auto VarData = loadVar(Var)) {
      Into.push_back(std::move(*VarData));
    }
  }
}

psr::TaintConfigData psr::parseTaintConfig(const llvm::Twine &Path) {
  static const nlohmann::json TaintConfigSchema =
#include "../config/TaintConfigSchema.json"
      ;

  std::optional<nlohmann::json> TaintConfig = readJsonFile(Path);
  nlohmann::json_schema::json_validator Validator;

  Validator.set_root_schema(TaintConfigSchema);
  Validator.validate(*TaintConfig);

  nlohmann::json Config = *TaintConfig;

  TaintConfigData Data{};
  loadFunctions(Config, Data.Functions);
  loadVariables(Config, Data.Variables);

  return Data;
}

std::optional<psr::TaintConfigData>
psr::parseTaintConfigOrNull(const llvm::Twine &Path) noexcept {
  try {
    return parseTaintConfig(Path);
  } catch (std::exception &Exc) {
    PHASAR_LOG_LEVEL(ERROR, "parseTaintConfig failed: " << Exc.what());
    return std::nullopt;
  } catch (...) {
    PHASAR_LOG_LEVEL(ERROR, "parseTaintConfig failed with unknown error");
    return std::nullopt;
  }
}
