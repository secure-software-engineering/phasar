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
                     std::unordered_set<std::string> &Container) {
  if (Config.contains(Value)) {
    for (const auto &Curr : Config[Value]) {
      Container.insert(Curr);
    }
  }
}

void addAllFunctionRets(const nlohmann::json &Function,
                        std::unordered_set<std::string> &Container) {
  findAndAddValue(Function, "ret", Container);
}

void addAllFunctionParamsSources(const nlohmann::json &Param,
                                 std::unordered_set<std::string> &Container) {
  findAndAddValue(Param, "source", Container);
}

void addAllFunctionParamsSinks(const nlohmann::json &Param,
                               std::unordered_set<std::string> &Container) {
  findAndAddValue(Param, "sink", Container);
}

void addAllFunctionParamsSanitizers(
    const nlohmann::json &Param, std::unordered_set<std::string> &Container) {
  findAndAddValue(Param, "sanitizer", Container);
}

void addAllVariableScopes(const nlohmann::json &Variable,
                          std::unordered_set<std::string> &Container) {
  findAndAddValue(Variable, "scope", Container);
}

void addAllVariableLines(const nlohmann::json &Variable,
                         std::unordered_set<std::string> &Container) {
  findAndAddValue(Variable, "line", Container);
}

void addAllVariableCats(const nlohmann::json &Variable,
                        std::unordered_set<std::string> &Container) {
  findAndAddValue(Variable, "cat", Container);
}

void addAllVariableNames(const nlohmann::json &Variable,
                         std::unordered_set<std::string> &Container) {
  findAndAddValue(Variable, "name", Container);
}

TaintConfigData::TaintConfigData(const std::string &Filepath) {

  nlohmann::json Config(Filepath);

  // handle functions
  if (Config.contains("functions")) {
    for (auto &Function : Config["functions"]) {
      addAllFunctionRets(Function, FunctionRets);

      if (Function.contains("params")) {
        addAllFunctionParamsSources(Function["params"], FunctionParamsSources);
        addAllFunctionParamsSinks(Function["params"], FunctionParamsSinks);
        addAllFunctionParamsSanitizers(Function["params"],
                                       FunctionParamsSanitizers);
      }
    }
  }

  // handle variables
  if (Config.contains("variables")) {
    for (auto &Variable : Config["variables"]) {
      addAllVariableScopes(Variable, VariableScopes);
      addAllVariableLines(Variable, VariableLines);
      addAllVariableCats(Variable, VariableCats);
      addAllVariableNames(Variable, VariableNames);
    }
  }
}

const std::unordered_set<std::string> &
TaintConfigData::getAllFunctions() const {
  return Functions;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllFunctionRets() const {
  return FunctionRets;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllFunctionParamsSources() const {
  return FunctionParamsSources;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllFunctionParamsSinks() const {
  return FunctionParamsSinks;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllFunctionParamsSanitizers() const {
  return FunctionParamsSanitizers;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllVariables() const {
  return Variables;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllVariableScopes() const {
  return VariableScopes;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllVariableLines() const {
  return VariableLines;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllVariableCats() const {
  return VariableCats;
}
const std::unordered_set<std::string> &
TaintConfigData::getAllVariableNames() const {
  return VariableNames;
}

} // namespace psr