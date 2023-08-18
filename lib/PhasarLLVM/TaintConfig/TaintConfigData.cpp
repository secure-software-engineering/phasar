#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"

#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json-schema.hpp"
#include "nlohmann/json.hpp"

#include <system_error>

namespace psr {

std::optional<psr::TaintConfigData>
parseTaintConfigOrNull(const llvm::Twine &Path) {
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
  }
  return std::optional<TaintConfigData>(Path.str());
}

void findAndAddValue(const nlohmann::json &Config, const std::string &Value,
                     std::vector<std::string> &Container) {
  if (Config.contains(Value)) {
    for (const auto &Curr : Config[Value]) {
      Container.push_back(Curr);
    }
  }
}

void addAllFunctions(const nlohmann::json &Config,
                     std::vector<std::string> &Container) {
  findAndAddValue(Config, "functions", Container);
}

void addAllFunctionNames(const nlohmann::json &Function,
                         std::vector<std::string> &Container) {
  findAndAddValue(Function, "name", Container);
}

void addAllFunctionRets(const nlohmann::json &Function,
                        std::vector<std::string> &Container) {
  findAndAddValue(Function, "ret", Container);
}

void addAllFunctionParamsSources(const nlohmann::json &Param,
                                 std::vector<std::string> &Container) {
  findAndAddValue(Param, "source", Container);
}

void addAllFunctionParamsSinks(const nlohmann::json &Param,
                               std::vector<std::string> &Container) {
  findAndAddValue(Param, "sink", Container);
}

void addAllFunctionParamsSanitizers(const nlohmann::json &Param,
                                    std::vector<std::string> &Container) {
  findAndAddValue(Param, "sanitizer", Container);
}

TaintConfigData::TaintConfigData(const std::string &Filepath) {

  nlohmann::json Config(Filepath);

  // handle functions
  if (Config.contains("functions")) {
    for (const auto &Func : Config["functions"]) {
      FunctionData Data = FunctionData();

      if (Func.contains("name")) {
        Data.Name = Func["name"];
      }

      if (Func.contains("ret")) {
        Data.ReturnType = Func["ret"];
      }

      if (Func.contains("params") && Func["params"].contains("source")) {
        for (const auto &Curr : Func["params"]["source"]) {
          Data.SourceValues.push_back(Curr);
        }
      }

      if (Func.contains("params") && Func["params"].contains("sink")) {
        for (const auto &Curr : Func["params"]["sink"]) {
          Data.SinkValues.push_back(Curr);
        }
      }

      if (Func.contains("params") && Func["params"].contains("sanitizer")) {
        for (const auto &Curr : Func["params"]["sanitizer"]) {
          Data.SanitizerValues.push_back(Curr);
        }
      }

      Functions.push_back(std::move(Data));
    }
  }

  // handle variables
  if (Config.contains("variables")) {
    for (const auto &Var : Config["variables"]) {
      VariableData Data = VariableData();

      if (Var.contains("line")) {
        Data.Line = Var["line"];
      }

      if (Var.contains("name")) {
        Data.Line = Var["name"];
      }

      if (Var.contains("scope")) {
        Data.Line = Var["scope"];
      }

      if (Var.contains("cat")) {
        Data.Line = Var["cat"];
      }

      Variables.push_back(std::move(Data));
    }
  }
}

std::vector<std::string> TaintConfigData::getAllFunctionNames() const {
  std::vector<std::string> FunctionNames;
  FunctionNames.reserve(Functions.size());

  for (const auto &Func : Functions) {
    FunctionNames.push_back(Func.Name);
  }

  return FunctionNames;
}

std::vector<std::string> TaintConfigData::getAllVariableLines() const {
  std::vector<std::string> VariableLines;
  VariableLines.reserve(Variables.size());

  for (const auto &Var : Variables) {
    VariableLines.push_back(Var.Name);
  }

  return VariableLines;
}
std::vector<std::string> TaintConfigData::getAllVariableCats() const {
  std::vector<std::string> VariableCats;
  VariableCats.reserve(Variables.size());

  for (const auto &Var : Variables) {
    VariableCats.push_back(Var.Name);
  }

  return VariableCats;
}

} // namespace psr