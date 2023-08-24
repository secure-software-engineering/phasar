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

#include <nlohmann/json_fwd.hpp>

namespace psr {

TaintConfigData::TaintConfigData(const std::string &Filepath) {
  llvm::outs() << "Constructor 0\n";
  llvm::outs().flush();
  std::optional<nlohmann::json> TaintConfig = readJsonFile(Filepath);
  llvm::outs() << "Constructor 1\n";
  llvm::outs().flush();
  nlohmann::json_schema::json_validator Validator;
  llvm::outs() << "Constructor 2\n";
  llvm::outs().flush();
  try {
    static const nlohmann::json TaintConfigSchema =
#include "../config/TaintConfigSchema.json"
        ;

    Validator.set_root_schema(TaintConfigSchema); // insert root-schema
  } catch (const std::exception &E) {
    PHASAR_LOG_LEVEL(ERROR,
                     "Validation of schema failed, here is why: " << E.what());
    return;
  }
  llvm::outs() << "Constructor 3\n";
  llvm::outs().flush();

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

  llvm::outs() << "Constructor 4\n";
  llvm::outs().flush();
  CustomJsonErrorHandler Err;
  Validator.validate(*TaintConfig, Err);
  llvm::outs() << "Constructor 5\n";
  llvm::outs().flush();
  if (Err) {
    llvm::outs() << "[TaintConfigData::TaintConfigData()]: if (Err) {\n";
    llvm::outs().flush();
    TaintConfig.reset();
    return;
  }

  llvm::outs() << "Constructor 6\n";
  llvm::outs().flush();
  if (!TaintConfig) {
    llvm::outs()
        << "[TaintConfigData::TaintConfigData()]: TaintConfigData is null";
    llvm::outs().flush();
    return;
  };

  llvm::outs() << "Constructor 7\n";
  llvm::outs().flush();
  nlohmann::json Config = *TaintConfig;
  // llvm::outs() << Config;
  // llvm::outs().flush();

  llvm::outs() << "Constructor 8\n";
  llvm::outs().flush();
  // handle functions
  if (Config.contains("functions")) {
    for (const auto &Func : Config["functions"]) {
      FunctionData Data = FunctionData();
      bool FuncPushBackFlag = false;

      if (Func.contains("name")) {
        llvm::outs() << "[TaintConfigData::TaintConfigData()]: name test\n";
        llvm::outs().flush();
        Data.Name = Func["name"];
        FuncPushBackFlag = true;
      }

      if (Func.contains("ret")) {
        llvm::outs() << "[TaintConfigData::TaintConfigData()]: ret test\n";
        llvm::outs().flush();
        Data.ReturnType = Func["ret"];
        FuncPushBackFlag = true;
      }

      if (Func.contains("params") && Func["params"].contains("source")) {
        for (const auto &Curr : Func["params"]["source"]) {
          llvm::outs() << "[TaintConfigData::TaintConfigData()]: source test: "
                       << Curr.get<int>() << "\n";
          llvm::outs().flush();
          Data.SourceValues.push_back(Curr.get<int>());
        }
        FuncPushBackFlag = true;
      }

      if (Func.contains("params") && Func["params"].contains("sink")) {
        for (const auto &Curr : Func["params"]["sink"]) {
          Data.SinkValues.push_back(Curr.get<int>());
          llvm::outs() << "[TaintConfigData::TaintConfigData()]: sink test"
                       << Curr.get<int>() << "\n";
          llvm::outs().flush();
        }
        FuncPushBackFlag = true;
      }

      if (Func.contains("params") && Func["params"].contains("sanitizer")) {
        for (const auto &Curr : Func["params"]["sanitizer"]) {
          llvm::outs()
              << "[TaintConfigData::TaintConfigData()]: sanitizer test: "
              << Curr.get<int>() << "\n";
          llvm::outs().flush();
          Data.SanitizerValues.push_back(Curr.get<int>());
        }
        FuncPushBackFlag = true;
      }

      if (FuncPushBackFlag) {
        Functions.push_back(std::move(Data));
      }
    }
  }

  // handle variables
  if (Config.contains("variables")) {
    for (const auto &Var : Config["variables"]) {
      VariableData Data = VariableData();
      bool VarPushBackFlag = false;

      if (Var.contains("line")) {
        Data.Line = Var["line"].get<int>();
        VarPushBackFlag = true;
        llvm::outs() << "line test: " << Var["line"].get<int>() << "\n";
        llvm::outs().flush();
      }

      if (Var.contains("name")) {
        Data.Name = Var["name"].get<std::string>();
        VarPushBackFlag = true;
        llvm::outs() << "name test: " << Var.contains("name") << "\n";
        llvm::outs().flush();
      }

      if (Var.contains("scope")) {
        Data.Scope = Var["scope"].get<std::string>();
        VarPushBackFlag = true;
        llvm::outs() << "scope test: " << Var["scope"].get<std::string>()
                     << "\n";
        llvm::outs().flush();
      }

      if (Var.contains("cat")) {
        Data.Cat = Var["cat"].get<std::string>();
        VarPushBackFlag = true;
        llvm::outs() << "cat test: " << Var["cat"].get<std::string>() << "\n";
        llvm::outs().flush();
      }
      if (VarPushBackFlag) {
        Variables.push_back(std::move(Data));
      }
    }
  }

  llvm::outs() << "Funcsize: " << Functions.size()
               << " - Varsize: " << Variables.size() << "\n";
  llvm::outs().flush();
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