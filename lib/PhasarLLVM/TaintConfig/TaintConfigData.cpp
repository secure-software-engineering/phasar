#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/IR/DebugInfo.h"
#include "llvm/Support/raw_ostream.h"

#include <system_error>

namespace psr {

TaintConfigData::TaintConfigData(const llvm::Twine &Path,
                                 const LLVMProjectIRDB &IRDB) {
  loadDataFromFileForThis(Path, IRDB);
}

static llvm::SmallVector<const llvm::Function *>
findAllFunctionDefs(const LLVMProjectIRDB &IRDB, llvm::StringRef Name) {
  llvm::SmallVector<const llvm::Function *> FnDefs;
  llvm::DebugInfoFinder DIF;
  const auto *M = IRDB.getModule();

  DIF.processModule(*M);
  for (const auto &SubProgram : DIF.subprograms()) {
    if (SubProgram->isDistinct() && !SubProgram->getLinkageName().empty() &&
        (SubProgram->getName() == Name ||
         SubProgram->getLinkageName() == Name)) {
      FnDefs.push_back(IRDB.getFunction(SubProgram->getLinkageName()));
    }
  }
  DIF.reset();

  if (FnDefs.empty()) {
    const auto *F = IRDB.getFunction(Name);
    if (F) {
      FnDefs.push_back(F);
    }
  } else if (FnDefs.size() > 1) {
    llvm::errs() << "The function name '" << Name
                 << "' is ambiguous. Possible candidates are:\n";
    for (const auto *F : FnDefs) {
      llvm::errs() << "> " << F->getName() << "\n";
    }
    llvm::errs() << "Please further specify the function's name, such that it "
                    "becomes unambiguous\n";
  }

  return FnDefs;
}

TaintConfigData::TaintConfigData(const nlohmann::json &JSON,
                                 const LLVMProjectIRDB &IRDB) {
  for (const auto &FunDesc : JSON["functions"]) {
    auto Name = FunDesc["name"].get<std::string>();

    auto FnDefs = findAllFunctionDefs(IRDB, Name);

    if (FnDefs.empty()) {
      llvm::errs() << "WARNING: Cannot retrieve function " << Name << "\n";
      continue;
    }

    const auto *Fun = FnDefs[0];

    // handle a function's parameters
    if (FunDesc.contains("params")) {
      auto Params = FunDesc["params"];
      if (Params.contains("source")) {
        for (unsigned Idx : Params["source"]) {
          if (Idx >= Fun->arg_size()) {
            llvm::errs()
                << "ERROR: The source-function parameter index is out of "
                   "bounds: "
                << Idx << "\n";
            // Use 'continue' instead of 'break' to get error messages for the
            // remaining parameters as well
            continue;
          }
          SourceValues.insert(Fun->getArg(Idx)->getName().str());
        }
      }
      if (Params.contains("sink")) {
        for (const auto &Idx : Params["sink"]) {
          if (Idx.is_number()) {
            if (Idx >= Fun->arg_size()) {
              llvm::errs()
                  << "ERROR: The source-function parameter index is out of "
                     "bounds: "
                  << Idx << "\n";
              continue;
            }
            SinkValues.insert(Fun->getArg(Idx)->getName().str());
          } else if (Idx.is_string()) {
            const auto Sinks = Idx.get<std::string>();
            if (Sinks == "all") {
              for (const auto &Arg : Fun->args()) {
                SinkValues.insert(Fun->getArg(Idx)->getName().str());
              }
            }
          }
        }
      }
      if (Params.contains("sanitizer")) {
        for (unsigned Idx : Params["sanitizer"]) {
          if (Idx >= Fun->arg_size()) {
            llvm::errs()
                << "ERROR: The source-function parameter index is out of "
                   "bounds: "
                << Idx << "\n";
            continue;
          }
          SanitizerValues.insert(Fun->getArg(Idx)->getName().str());
        }
      }
    }
    // handle a function's return value
    /*
    if (FunDesc.contains("ret")) {
      for (const auto &User : Fun->users()) {
        Data.addTaintValue(User, FunDesc["ret"].get<std::string>());
      }
    }*/
  }
}

TaintConfigData TaintConfigData::loadDataFromFile(const llvm::Twine &Path,
                                                  const LLVMProjectIRDB &IRDB) {
  TaintConfigData Data = TaintConfigData();
  nlohmann::json Config = readJsonFile(Path);
  for (const auto &FunDesc : Config["functions"]) {
    auto Name = FunDesc["name"].get<std::string>();

    auto FnDefs = findAllFunctionDefs(IRDB, Name);

    if (FnDefs.empty()) {
      llvm::errs() << "WARNING: Cannot retrieve function " << Name << "\n";
      continue;
    }

    const auto *Fun = FnDefs[0];

    // handle a function's parameters
    if (FunDesc.contains("params")) {
      auto Params = FunDesc["params"];
      if (Params.contains("source")) {
        for (unsigned Idx : Params["source"]) {
          if (Idx >= Fun->arg_size()) {
            llvm::errs()
                << "ERROR: The source-function parameter index is out of "
                   "bounds: "
                << Idx << "\n";
            // Use 'continue' instead of 'break' to get error messages for the
            // remaining parameters as well
            continue;
          }
          Data.addSourceValue(Fun->getArg(Idx)->getName().str());
        }
      }
      if (Params.contains("sink")) {
        for (const auto &Idx : Params["sink"]) {
          if (Idx.is_number()) {
            if (Idx >= Fun->arg_size()) {
              llvm::errs()
                  << "ERROR: The source-function parameter index is out of "
                     "bounds: "
                  << Idx << "\n";
              continue;
            }
            Data.addSinkValue(Fun->getArg(Idx)->getName().str());
          } else if (Idx.is_string()) {
            const auto Sinks = Idx.get<std::string>();
            if (Sinks == "all") {
              for (const auto &Arg : Fun->args()) {
                Data.addSinkValue(Fun->getArg(Idx)->getName().str());
              }
            }
          }
        }
      }
      if (Params.contains("sanitizer")) {
        for (unsigned Idx : Params["sanitizer"]) {
          if (Idx >= Fun->arg_size()) {
            llvm::errs()
                << "ERROR: The source-function parameter index is out of "
                   "bounds: "
                << Idx << "\n";
            continue;
          }
          Data.addSanitizerValue(Fun->getArg(Idx)->getName().str());
        }
      }
    }
    // handle a function's return value
    /*
    if (FunDesc.contains("ret")) {
      for (const auto &User : Fun->users()) {
        Data.addTaintValue(User, FunDesc["ret"].get<std::string>());
      }
    }*/
  }

  return Data;
}

void TaintConfigData::addDataToFile(const llvm::Twine &Path) {
  nlohmann::json Config;

  for (const auto &Source : SourceValues) {
    Config.push_back({"SourceValues", {{Source}}});
  }

  for (const auto &Sink : SinkValues) {
    Config.push_back({"SinkValues", {{Sink}}});
  }

  for (const auto &Sanitizer : SanitizerValues) {
    Config.push_back({"SanitizerValues", {{Sanitizer}}});
  }

  std::error_code FileError;
  llvm::raw_fd_ostream File(Path.str(), FileError);

  if (FileError) {
    llvm::errs() << "Error while creating file: " << Path.str() << "\n";
  }

  File << Config;
}

void TaintConfigData::getValuesFromJSON(nlohmann::json JSON) {
  // TODO:
}

void TaintConfigData::loadDataFromFileForThis(const llvm::Twine &Path,
                                              const LLVMProjectIRDB &IRDB) {
  nlohmann::json Config = readJsonFile(Path);
  for (const auto &FunDesc : Config["functions"]) {
    auto Name = FunDesc["name"].get<std::string>();

    auto FnDefs = findAllFunctionDefs(IRDB, Name);

    if (FnDefs.empty()) {
      llvm::errs() << "WARNING: Cannot retrieve function " << Name << "\n";
      continue;
    }

    const auto *Fun = FnDefs[0];

    // handle a function's parameters
    if (FunDesc.contains("params")) {
      auto Params = FunDesc["params"];
      if (Params.contains("source")) {
        for (unsigned Idx : Params["source"]) {
          if (Idx >= Fun->arg_size()) {
            llvm::errs()
                << "ERROR: The source-function parameter index is out of "
                   "bounds: "
                << Idx << "\n";
            // Use 'continue' instead of 'break' to get error messages for the
            // remaining parameters as well
            continue;
          }
          SourceValues.insert(Fun->getArg(Idx)->getName().str());
        }
      }
      if (Params.contains("sink")) {
        for (const auto &Idx : Params["sink"]) {
          if (Idx.is_number()) {
            if (Idx >= Fun->arg_size()) {
              llvm::errs()
                  << "ERROR: The source-function parameter index is out of "
                     "bounds: "
                  << Idx << "\n";
              continue;
            }
            SinkValues.insert(Fun->getArg(Idx)->getName().str());
          } else if (Idx.is_string()) {
            const auto Sinks = Idx.get<std::string>();
            if (Sinks == "all") {
              for (const auto &Arg : Fun->args()) {
                SinkValues.insert(Fun->getArg(Idx)->getName().str());
              }
            }
          }
        }
      }
      if (Params.contains("sanitizer")) {
        for (unsigned Idx : Params["sanitizer"]) {
          if (Idx >= Fun->arg_size()) {
            llvm::errs()
                << "ERROR: The source-function parameter index is out of "
                   "bounds: "
                << Idx << "\n";
            continue;
          }
          SanitizerValues.insert(Fun->getArg(Idx)->getName().str());
        }
      }
    }
    // handle a function's return value
    /*
    if (FunDesc.contains("ret")) {
      for (const auto &User : Fun->users()) {
        Data.addTaintValue(User, FunDesc["ret"].get<std::string>());
      }
    }*/
  }
}

} // namespace psr
