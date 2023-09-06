#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

#include "nlohmann/json-schema.hpp"

#include <optional>

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

psr::TaintConfigData psr::parseTaintConfig(const llvm::Twine &Path) {
  auto Ret = parseTaintConfigOrNull(Path);
  if (!Ret) {
    /*TODO: assertion oder error thrown*/
    llvm::errs() << "ERROR: TaintConfigData is null\n";
    abort();
  }
  return std::move(*Ret);
}

void handleFunctions(const nlohmann::json &JSON,
                     std::vector<psr::FunctionData> &Container) {
  if (JSON.contains("functions")) {
    for (const auto &Func : JSON["functions"]) {
      psr::FunctionData Data = psr::FunctionData();
      bool FuncPushBackFlag = false;

      if (Func.contains("name")) {
        Data.Name = Func["name"].get<std::string>();
        FuncPushBackFlag = true;
      }

      if (Func.contains("ret")) {
        Data.ReturnType = Func["ret"].get<std::string>();
        FuncPushBackFlag = true;
      }

      if (Func.contains("params") && Func["params"].contains("source")) {
        for (const auto &Curr : Func["params"]["source"]) {
          Data.SourceValues.push_back(Curr.get<int>());
        }
        FuncPushBackFlag = true;
      }

      if (Func.contains("params") && Func["params"].contains("sink")) {
        for (const auto &Curr : Func["params"]["sink"]) {
          if (Curr.is_number()) {
            Data.SinkValues.push_back(Curr.get<int>());
          } else if (Curr.is_string() && Curr.get<std::string>() == "all") {
            Data.HasAllSinkParam = true;
          } else {
            llvm::errs() << "[TaintConfigData::TaintConfigData()]: "
                            "Unknown sink string parameter!";
          }
        }
        FuncPushBackFlag = true;
      }

      if (Func.contains("params") && Func["params"].contains("sanitizer")) {
        for (const auto &Curr : Func["params"]["sanitizer"]) {
          Data.SanitizerValues.push_back(Curr.get<int>());
        }
        FuncPushBackFlag = true;
      }

      if (FuncPushBackFlag) {
        Container.push_back(std::move(Data));
      }
    }
  }
}

void handleVariables(const nlohmann::json &JSON,
                     std::vector<psr::VariableData> &Container) {
  if (JSON.contains("variables")) {
    for (const auto &Var : JSON["variables"]) {
      psr::VariableData Data = psr::VariableData();
      bool VarPushBackFlag = false;
      if (Var.contains("line")) {
        Data.Line = Var["line"].get<int>();
        VarPushBackFlag = true;
      }

      if (Var.contains("name")) {
        Data.Name = Var["name"].get<std::string>();
        VarPushBackFlag = true;
      }

      if (Var.contains("scope")) {
        Data.Scope = Var["scope"].get<std::string>();
        VarPushBackFlag = true;
      }

      if (Var.contains("cat")) {
        Data.Cat = Var["cat"].get<std::string>();
        VarPushBackFlag = true;
      }
      if (VarPushBackFlag) {
        Container.push_back(std::move(Data));
      }
    }
  }
}

std::optional<psr::TaintConfigData>
psr::parseTaintConfigOrNull(const llvm::Twine &Path) {
  std::optional<nlohmann::json> TaintConfig = readJsonFile(Path);
  nlohmann::json_schema::json_validator Validator;
  try {
    static const nlohmann::json TaintConfigSchema =
#include "../config/TaintConfigSchema.json"
        ;

    Validator.set_root_schema(TaintConfigSchema); // insert root-schema
  } catch (const std::exception &E) {
    PHASAR_LOG_LEVEL(ERROR,
                     "Validation of schema failed, here is why: " << E.what());
    return std::nullopt;
  }

  // a custom error handler
  class CustomJsonErrorHandler
      : public nlohmann::json_schema::basic_error_handler {
    void error(const nlohmann::json::json_pointer &Pointer,
               const nlohmann::json &Instance,
               const std::string &Message) override {
      nlohmann::json_schema::basic_error_handler::error(Pointer, Instance,
                                                        Message);
      PHASAR_LOG_LEVEL(ERROR, Pointer.to_string()
                                  << "' - '" << Instance << "': " << Message);
    }
  };

  CustomJsonErrorHandler Err;
  Validator.validate(*TaintConfig, Err);
  if (Err) {
    TaintConfig.reset();
    return std::nullopt;
  }

  nlohmann::json Config = *TaintConfig;
  TaintConfigData Data;

  std::vector<FunctionData> Functions;
  handleFunctions(Config, Data.Functions);

  std::vector<VariableData> Variables;
  handleVariables(Config, Data.Variables);

  return Data;
}
