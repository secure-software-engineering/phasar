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

#include <string>

namespace psr {

TaintConfigData::TaintConfigData(const std::string &Filepath) {
  std::optional<nlohmann::json> TaintConfig = readJsonFile(Filepath);
  nlohmann::json_schema::json_validator Validator;
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
    return;
  }

  if (!TaintConfig) {
    llvm::outs()
        << "[TaintConfigData::TaintConfigData()]: TaintConfigData is null!";
    llvm::outs().flush();
    return;
  };

  nlohmann::json Config = *TaintConfig;

  llvm::outs() << "before functions\n";
  llvm::outs().flush();
  // handle functions
  if (Config.contains("functions")) {
    for (const auto &Func : Config["functions"]) {
      FunctionData Data = FunctionData();
      bool FuncPushBackFlag = false;

      llvm::outs() << "name\n";
      llvm::outs().flush();
      if (Func.contains("name")) {
        Data.Name = Func["name"].get<std::string>();
        FuncPushBackFlag = true;
      }

      llvm::outs() << "ret\n";
      llvm::outs().flush();
      if (Func.contains("ret")) {
        Data.ReturnType = Func["ret"].get<std::string>();
        FuncPushBackFlag = true;
      }

      llvm::outs() << "source\n";
      llvm::outs().flush();
      if (Func.contains("params") && Func["params"].contains("source")) {
        for (const auto &Curr : Func["params"]["source"]) {
          Data.SourceValues.push_back(Curr.get<int>());
        }
        FuncPushBackFlag = true;
      }

      llvm::outs() << "sink\n";
      llvm::outs().flush();
      if (Func.contains("params") && Func["params"].contains("sink")) {
        for (const auto &Curr : Func["params"]["sink"]) {
          if (Curr.is_number()) {
            llvm::outs() << "i am number " << std::to_string(Curr.get<int>())
                         << "\n";
            llvm::outs().flush();
            Data.SinkValues.push_back(Curr.get<int>());
          } else if (Curr.is_string() && Curr.get<std::string>() == "all") {
            Data.HasAllSinkParam = true;
          } else {
            llvm::outs() << "[TaintConfigData::TaintConfigData()]: "
                            "Unknown sink string parameter!";
            llvm::outs().flush();
          }
        }
        FuncPushBackFlag = true;
      }

      llvm::outs() << "sanitizer\n";
      llvm::outs().flush();
      if (Func.contains("params") && Func["params"].contains("sanitizer")) {
        for (const auto &Curr : Func["params"]["sanitizer"]) {
          Data.SanitizerValues.push_back(Curr.get<int>());
        }
        FuncPushBackFlag = true;
      }

      if (FuncPushBackFlag) {
        Functions.push_back(std::move(Data));
      }
    }
  }

  llvm::outs() << "before variables\n";
  llvm::outs().flush();
  // handle variables
  if (Config.contains("variables")) {
    for (const auto &Var : Config["variables"]) {
      VariableData Data = VariableData();
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
        Variables.push_back(std::move(Data));
      }
    }
  }

  llvm::outs() << "all done\n";
  llvm::outs().flush();
}

} // namespace psr
