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
      // A functions name should be at the same index in the names array and the
      // functions array
      Functions.push_back(Func);

      findAndAddValue(Func, "name", FunctionNames);
      findAndAddValue(Func, "ret", FunctionRets);
      findAndAddValue(Func["params"], "source", FunctionParamSources);
      findAndAddValue(Func["params"], "sink", FunctionParamSinks);
      findAndAddValue(Func["params"], "sanitizer", FunctionParamSanitizers);
    }
  }

  // handle variables
  if (Config.contains("variables")) {
    for (const auto &Var : Config["variables"]) {
      // A variables name should be at the same index in the names array and the
      // variables array
      Variables.push_back(Var);

      findAndAddValue(Config["variables"], "name", VariableNames);
      findAndAddValue(Config["variables"], "scope", Variables);
      findAndAddValue(Config["variables"], "line", VariableLines);
      findAndAddValue(Config["variables"], "cat", VariableCats);
    }
  }
}

const std::vector<std::string> &TaintConfigData::getAllFunctions() const {
  return Functions;
}
const std::vector<std::string> &TaintConfigData::getAllFunctionNames() const {
  return FunctionNames;
}
const std::vector<std::string> &TaintConfigData::getAllFunctionRets() const {
  return FunctionRets;
}
const std::vector<std::string> &
TaintConfigData::getAllFunctionParamsSources() const {
  return FunctionParamSources;
}
const std::vector<std::string> &
TaintConfigData::getAllFunctionParamsSinks() const {
  return FunctionParamSinks;
}
const std::vector<std::string> &
TaintConfigData::getAllFunctionParamsSanitizers() const {
  return FunctionParamSanitizers;
}
const std::vector<std::string> &TaintConfigData::getAllVariables() const {
  return Variables;
}
const std::vector<std::string> &TaintConfigData::getAllVariableScopes() const {
  return VariableScopes;
}
const std::vector<std::string> &TaintConfigData::getAllVariableLines() const {
  return VariableLines;
}
const std::vector<std::string> &TaintConfigData::getAllVariableCats() const {
  return VariableCats;
}
const std::vector<std::string> &TaintConfigData::getAllVariableNames() const {
  return VariableNames;
}

} // namespace psr