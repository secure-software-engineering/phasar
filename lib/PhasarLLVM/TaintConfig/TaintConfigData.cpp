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

#include <system_error>

namespace psr {

TaintConfigData::TaintConfigData(const LLVMProjectIRDB &IRDB,
                                 const nlohmann::json &Config) {
  // handle functions
  if (Config.contains("functions")) {
    addAllFunctions(IRDB, Config);
  }

  // handle variables
  if (Config.contains("variables")) {
    // scope can be a function name or a struct.
    std::unordered_map<const llvm::Type *, const nlohmann::json>
        StructConfigMap;

    // read all struct types from config
    for (const auto &VarDesc : Config["variables"]) {
      llvm::DebugInfoFinder DIF;
      const auto *M = IRDB.getModule();

      DIF.processModule(*M);
      for (const auto &Ty : DIF.types()) {
        if (Ty->getTag() == llvm::dwarf::DW_TAG_structure_type &&
            Ty->getName().equals(VarDesc["scope"].get<std::string>())) {
          for (const auto &LlvmStructTy : M->getIdentifiedStructTypes()) {
            StructConfigMap.insert(
                std::pair<const llvm::Type *, const nlohmann::json>(
                    LlvmStructTy, VarDesc));
          }
        }
      }
      DIF.reset();
    }

    // add corresponding Allocas or getElementPtr instructions to the taint
    // category
    for (const auto &VarDesc : Config["variables"]) {
      for (const auto &Fun : IRDB.getAllFunctions()) {
        for (const auto &I : llvm::instructions(Fun)) {
          if (const auto *DbgDeclare =
                  llvm::dyn_cast<llvm::DbgDeclareInst>(&I)) {
            const llvm::DILocalVariable *LocalVar = DbgDeclare->getVariable();
            // matching line number with for Allocas
            if (LocalVar->getName().equals(
                    VarDesc["name"].get<std::string>()) &&
                LocalVar->getLine() == VarDesc["line"].get<unsigned int>()) {
              addTaintCategory(DbgDeclare->getAddress(),
                               VarDesc["cat"].get<std::string>());
            }
          } else if (!StructConfigMap.empty()) {
            // Ignorning line numbers for getElementPtr instructions
            if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(&I)) {
              const auto *StType = llvm::dyn_cast<llvm::StructType>(
                  Gep->getPointerOperandType()->getPointerElementType());
              if (StType && StructConfigMap.count(StType)) {
                const auto VarDesc = StructConfigMap.at(StType);
                auto VarName = VarDesc["name"].get<std::string>();
                // using substr to cover the edge case in which same variable
                // name is present as a local variable and also as a struct
                // member variable. (Ex. JsonConfig/fun_member_02.cpp)
                if (Gep->getName().substr(0, VarName.size()).equals(VarName)) {
                  addTaintCategory(Gep, VarDesc["cat"].get<std::string>());
                }
              }
            }
          }
        }
      }
    }
  }
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

void TaintConfigData::addAllFunctions(const LLVMProjectIRDB &IRDB,
                                      const nlohmann::json &Config) {
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
          addTaintCategory(Fun->getArg(Idx), "source");
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
            addTaintCategory(Fun->getArg(Idx), "Sink");
          } else if (Idx.is_string()) {
            const auto Sinks = Idx.get<std::string>();
            if (Sinks == "all") {
              for (const auto &Arg : Fun->args()) {
                addTaintCategory(&Arg, "sink");
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
          addTaintCategory(Fun->getArg(Idx), "sanitizer");
        }
      }
    }
    // handle a function's return value
    if (FunDesc.contains("ret")) {
      for (const auto &User : Fun->users()) {
        addTaintCategory(User, FunDesc["ret"].get<std::string>());
      }
    }
  }
}

//
// --- Own API function implementations
//

void TaintConfigData::addSourceValue(const llvm::Value *V) {
  SourceValues.insert(V);
}

void TaintConfigData::addSinkValue(const llvm::Value *V) {
  SinkValues.insert(V);
}

void TaintConfigData::addSanitizerValue(const llvm::Value *V) {
  SanitizerValues.insert(V);
}

void TaintConfigData::addTaintCategory(const llvm::Value *Val,
                                       llvm::StringRef AnnotationStr) {
  auto TC = toTaintCategory(AnnotationStr);
  if (TC == TaintCategory::None) {
    PHASAR_LOG_LEVEL(ERROR, "Unknown taint category: " << AnnotationStr);
  } else {
    addTaintCategory(Val, TC);
  }
}

void TaintConfigData::addTaintCategory(const llvm::Value *Val,
                                       TaintCategory Annotation) {
  switch (Annotation) {
  case TaintCategory::Source:
    addSourceValue(Val);
    break;
  case TaintCategory::Sink:
    addSinkValue(Val);
    break;
  case TaintCategory::Sanitizer:
    addSanitizerValue(Val);
    break;
  default:
    // ignore
    break;
  }
}

} // namespace psr