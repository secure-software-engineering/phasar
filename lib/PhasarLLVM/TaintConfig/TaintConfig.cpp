/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

#include "nlohmann/json-schema.hpp"
#include "nlohmann/json.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/Utils/Annotation.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

namespace {
const nlohmann::json TaintConfigSchema =
#include "../config/TaintConfigSchema.json"
    ;
} // anonymous namespace

namespace psr {

TaintCategory toTaintCategory(llvm::StringRef Str) {
  return llvm::StringSwitch<TaintCategory>(Str)
      .CaseLower("source", TaintCategory::Source)
      .CaseLower("sink", TaintCategory::Sink)
      .CaseLower("sanitizer", TaintCategory::Sanitizer)
      .Default(TaintCategory::None);
}

void TaintConfig::addTaintCategory(const llvm::Value *Val,
                                   llvm::StringRef AnnotationStr) {
  auto TC = toTaintCategory(AnnotationStr);
  if (TC == TaintCategory::None) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), ERROR)
                  << "ERROR: Unknown taint category: " << AnnotationStr.str());
  } else {
    addTaintCategory(Val, TC);
  }
}

void TaintConfig::addTaintCategory(const llvm::Value *Val,
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

static llvm::SmallVector<const llvm::Function *>
findAllFunctionDefs(const ProjectIRDB &IRDB, llvm::StringRef Name) {
  llvm::SmallVector<const llvm::Function *> FnDefs;
  llvm::DebugInfoFinder DIF;
  for (const auto *M : IRDB.getAllModules()) {
    DIF.processModule(*M);
    for (const auto &SubProgram : DIF.subprograms()) {
      if (SubProgram->isDistinct() && !SubProgram->getLinkageName().empty() &&
          (SubProgram->getName() == Name ||
           SubProgram->getLinkageName() == Name)) {
        FnDefs.push_back(IRDB.getFunction(SubProgram->getLinkageName()));
      }
    }
    DIF.reset();
  }

  if (FnDefs.empty()) {
    const auto *F = IRDB.getFunction(Name);
    if (F) {
      FnDefs.push_back(F);
    }
  } else if (FnDefs.size() > 1) {
    std::cerr << "The function name '" << Name.str()
              << "' is ambiguous. Possible candidates are:\n";
    for (const auto *F : FnDefs) {
      std::cerr << "> " << F->getName().str() << "\n";
    }
    std::cerr << "Please further specify the function's name, such that it "
                 "becomes unambiguous\n";
  }

  return FnDefs;
}

void TaintConfig::addAllFunctions(const ProjectIRDB &IRDB,
                                  const nlohmann::json &Config) {

  for (const auto &FunDesc : Config["functions"]) {
    auto Name = FunDesc["name"].get<std::string>();

    auto FnDefs = findAllFunctionDefs(IRDB, Name);

    if (FnDefs.empty()) {
      std::cerr << "WARNING: Cannot retrieve function " << Name << "\n";
      continue;
    }

    const auto *Fun = FnDefs[0];

    // handle a function's parameters
    if (FunDesc.contains("params")) {
      auto Params = FunDesc["params"];
      if (Params.contains("source")) {
        for (unsigned Idx : Params["source"]) {
          if (Idx >= Fun->arg_size()) {
            std::cerr << "ERROR: The source-function parameter index is out of "
                         "bounds: "
                      << Idx << "\n";
            // Use 'continue' instead of 'break' to get error messages for the
            // remaining parameters as well
            continue;
          }
          addTaintCategory(Fun->getArg(Idx), TaintCategory::Source);
        }
      }
      if (Params.contains("sink")) {
        for (const auto &Idx : Params["sink"]) {
          if (Idx.is_number()) {
            if (Idx >= Fun->arg_size()) {
              std::cerr
                  << "ERROR: The source-function parameter index is out of "
                     "bounds: "
                  << Idx << "\n";
              continue;
            }
            addTaintCategory(Fun->getArg(Idx), TaintCategory::Sink);
          } else if (Idx.is_string()) {
            const auto Sinks = Idx.get<std::string>();
            if (Sinks == "all") {
              for (const auto &Arg : Fun->args()) {
                addTaintCategory(&Arg, TaintCategory::Sink);
              }
            }
          }
        }
      }
      if (Params.contains("sanitizer")) {
        for (unsigned Idx : Params["sanitizer"]) {
          if (Idx >= Fun->arg_size()) {
            std::cerr << "ERROR: The source-function parameter index is out of "
                         "bounds: "
                      << Idx << "\n";
            continue;
          }
          addTaintCategory(Fun->getArg(Idx), TaintCategory::Sanitizer);
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

TaintConfig::TaintConfig(const psr::ProjectIRDB &Code, // NOLINT
                         const nlohmann::json &Config) {

  // handle functions
  if (Config.contains("functions")) {
    addAllFunctions(Code, Config);
  }

  // handle variables
  if (Config.contains("variables")) {
    // scope can be a function name or a struct.
    std::unordered_map<const llvm::Type *, const nlohmann::json>
        StructConfigMap;

    // read all struct types from config
    for (const auto &VarDesc : Config["variables"]) {
      llvm::DebugInfoFinder DIF;
      for (const auto &M : Code.getAllModules()) {
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
    }

    // add corresponding Allocas or getElementPtr instructions to the taint
    // category
    for (const auto &VarDesc : Config["variables"]) {
      for (const auto &Fun : Code.getAllFunctions()) {
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

TaintConfig::TaintConfig(const psr::ProjectIRDB &AnnotatedCode) { // NOLINT
  // handle "local" annotation declarations
  const auto *Annotation = AnnotatedCode.getFunction("llvm.var.annotation");
  if (Annotation) {
    for (const auto *VarAnnotationUser : Annotation->users()) {
      if (const auto *AnnotationCall =
              llvm::dyn_cast<llvm::CallBase>(VarAnnotationUser)) {
        VarAnnotation A(AnnotationCall);
        // get annotation string an remove prefix "psr."
        auto AnnotationStr = A.getAnnotationString().substr(4);
        const auto *Val =
            VarAnnotation::getOriginalValueOrOriginalArg(A.getValue());
        assert(Val && "Could not retrieve original value!");
        addTaintCategory(Val, AnnotationStr);
      }
    }
  }
  // handle "global" annotation declarations
  const auto *GlobalAnnotations =
      AnnotatedCode.getGlobalVariableDefinition("llvm.global.annotations");
  if (GlobalAnnotations) {
    for (const auto &Op : GlobalAnnotations->operands()) {
      if (auto *Array = llvm::dyn_cast<llvm::ConstantArray>(Op)) {
        for (auto &ArrayElem : Array->operands()) {
          if (auto *Struct = llvm::dyn_cast<llvm::ConstantStruct>(ArrayElem)) {
            GlobalAnnotation A(Struct);
            // get annotation string an remove prefix "psr."
            auto AnnotationStr = A.getAnnotationString().substr(4);
            const auto *Fun = A.getFunction();
            for (const auto &User : Fun->users()) {
              if (llvm::isa<llvm::CallBase>(User)) {
                addTaintCategory(User, AnnotationStr);
              }
            }
          }
        }
      }
    }
  }

  std::vector<const llvm::Function *> PtrAnnotations{};
  PtrAnnotations.reserve(AnnotatedCode.getAllFunctions().size());
  for (const auto *F : AnnotatedCode.getAllFunctions()) {
    if (F->getName().startswith("llvm.ptr.annotation")) {
      PtrAnnotations.push_back(F);
    }
  }

  for (const auto *Annotation : PtrAnnotations) {
    for (const auto *VarAnnotationUser : Annotation->users()) {
      if (const auto *AnnotationCall =
              llvm::dyn_cast<llvm::CallBase>(VarAnnotationUser)) {
        VarAnnotation A(AnnotationCall);
        // get annotation string an remove prefix "psr."
        auto AnnotationStr = A.getAnnotationString().substr(4);
        const auto *Val =
            VarAnnotation::getOriginalValueOrOriginalArg(A.getValue());
        assert(Val && "Could not retrieve original value!");
        addTaintCategory(Val, AnnotationStr);
      }
    }
  }
}

TaintConfig::TaintConfig(TaintDescriptionCallBackTy SourceCB,
                         TaintDescriptionCallBackTy SinkCB,
                         TaintDescriptionCallBackTy SanitizerCB)
    : SourceCallBack(std::move(SourceCB)), SinkCallBack(std::move(SinkCB)),
      SanitizerCallBack(std::move(SanitizerCB)) {}

void TaintConfig::registerSourceCallBack(TaintDescriptionCallBackTy SourceCB) {
  SourceCallBack = std::move(SourceCB);
}

void TaintConfig::registerSinkCallBack(TaintDescriptionCallBackTy SinkCB) {
  SinkCallBack = std::move(SinkCB);
}

void TaintConfig::registerSanitizerCallBack(
    TaintDescriptionCallBackTy SanitizerCB) {
  SanitizerCallBack = std::move(SanitizerCB);
}

const TaintConfig::TaintDescriptionCallBackTy &
TaintConfig::getRegisteredSourceCallBack() const {
  return SourceCallBack;
}

const TaintConfig::TaintDescriptionCallBackTy &
TaintConfig::getRegisteredSinkCallBack() const {
  return SinkCallBack;
}

bool TaintConfig::isSource(const llvm::Value *V) const {
  return SourceValues.count(V);
}

bool TaintConfig::isSink(const llvm::Value *V) const {
  return SinkValues.count(V);
}

bool TaintConfig::isSanitizer(const llvm::Value *V) const {
  return SanitizerValues.count(V);
}

void TaintConfig::forAllGeneratedValuesAt(
    const llvm::Instruction *Inst, const llvm::Function *Callee,
    llvm::function_ref<void(const llvm::Value *)> Handler) const {
  assert(Inst != nullptr);
  assert(Handler);

  if (SourceCallBack) {
    for (const auto *CBFact : SourceCallBack(Inst)) {
      Handler(CBFact);
    }
  }

  if (Callee) {
    for (const auto &Arg : Callee->args()) {
      if (SourceValues.count(&Arg)) {
        Handler(Inst->getOperand(Arg.getArgNo()));
      }
    }
  }

  for (const auto *Op : Inst->operand_values()) {
    if (SourceValues.count(Op)) {
      Handler(Op);
    }
  }

  if (SourceValues.count(Inst)) {
    Handler(Inst);
  }
}

void TaintConfig::forAllLeakCandidatesAt(
    const llvm::Instruction *Inst, const llvm::Function *Callee,
    llvm::function_ref<void(const llvm::Value *)> Handler) const {
  assert(Inst != nullptr);
  assert(Handler);

  if (SinkCallBack) {
    for (const auto *CBLeak : SinkCallBack(Inst)) {
      Handler(CBLeak);
    }
  }

  if (Callee) {
    for (const auto &Arg : Callee->args()) {
      if (SinkValues.count(&Arg)) {
        Handler(Inst->getOperand(Arg.getArgNo()));
      }
    }
  }

  // Do not iterate over the actual paramaters of Inst as we did in
  // forAllGeneratedValuesAt, because sink-values are not propagated in the
  // current taint analyses. Handling sink-values should be done in the
  // SinkCallBack
}

void TaintConfig::forAllSanitizedValuesAt(
    const llvm::Instruction *Inst, const llvm::Function *Callee,
    llvm::function_ref<void(const llvm::Value *)> Handler) const {
  assert(Inst != nullptr);
  assert(Handler);

  if (SanitizerCallBack) {
    for (const auto *CBSani : SanitizerCallBack(Inst)) {
      Handler(CBSani);
    }
  }

  if (Callee) {
    for (const auto &Arg : Callee->args()) {
      if (SanitizerValues.count(&Arg)) {
        Handler(Inst->getOperand(Arg.getArgNo()));
      }
    }
  }
}

bool TaintConfig::generatesValuesAt(const llvm::Instruction *Inst,
                                    const llvm::Function *Callee) const {
  assert(Inst != nullptr);

  if (SourceCallBack && !SourceCallBack(Inst).empty()) {
    return true;
  }

  if (Callee && std::any_of(Callee->arg_begin(), Callee->arg_end(),
                            [this](const auto &Arg) {
                              return SourceValues.count(&Arg);
                            })) {
    return true;
  }

  return SourceValues.count(Inst) ||
         std::any_of(Inst->value_op_begin(), Inst->value_op_end(),
                     [this](const auto *Op) { return SourceValues.count(Op); });
}

bool TaintConfig::mayLeakValuesAt(const llvm::Instruction *Inst,
                                  const llvm::Function *Callee) const {
  assert(Inst != nullptr);

  if (SinkCallBack && !SinkCallBack(Inst).empty()) {
    return true;
  }

  return Callee && std::any_of(Callee->arg_begin(), Callee->arg_end(),
                               [this](const auto &Arg) {
                                 return SinkValues.count(&Arg);
                               });
}
bool TaintConfig::sanitizesValuesAt(const llvm::Instruction *Inst,
                                    const llvm::Function *Callee) const {
  assert(Inst != nullptr);

  if (SanitizerCallBack && !SanitizerCallBack(Inst).empty()) {
    return true;
  }

  return Callee && std::any_of(Callee->arg_begin(), Callee->arg_end(),
                               [this](const auto &Arg) {
                                 return SanitizerValues.count(&Arg);
                               });
}

TaintCategory TaintConfig::getCategory(const llvm::Value *V) const {
  if (isSource(V)) {
    return TaintCategory::Source;
  }
  if (isSink(V)) {
    return TaintCategory::Sink;
  }
  if (isSanitizer(V)) {
    return TaintCategory::Sanitizer;
  }
  return TaintCategory::None;
}

void TaintConfig::addSourceValue(const llvm::Value *V) {
  SourceValues.insert(V);
}

void TaintConfig::addSinkValue(const llvm::Value *V) { SinkValues.insert(V); }

void TaintConfig::addSanitizerValue(const llvm::Value *V) {
  SanitizerValues.insert(V);
}

std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
TaintConfig::makeInitialSeeds() const {
  std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
      InitialSeeds;
  for (const auto *SourceValue : SourceValues) {
    if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(SourceValue)) {
      InitialSeeds[Inst].insert(Inst);
    } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(SourceValue)) {
      const auto *FunFirstInst = &Arg->getParent()->front().front();
      InitialSeeds[FunFirstInst].insert(Arg);
    }
  }
  return InitialSeeds;
}

std::ostream &operator<<(std::ostream &OS, const TaintConfig &TC) {
  OS << "TaintConfiguration: ";
  if (TC.SourceValues.empty() && TC.SinkValues.empty() &&
      TC.SanitizerValues.empty() && !TC.getRegisteredSourceCallBack() &&
      !TC.getRegisteredSinkCallBack()) {
    return OS << "empty";
  }
  OS << "\n\tSourceCallBack registered: " << std::boolalpha
     << (bool)TC.SourceCallBack << std::noboolalpha << '\n';
  OS << "\tSinkCallBack registered: " << std::boolalpha << (bool)TC.SinkCallBack
     << std::noboolalpha << '\n';
  OS << "\tSources (" << TC.SourceValues.size() << "):\n";
  for (const auto *SourceValue : TC.SourceValues) {
    OS << "\t\t" << psr::llvmIRToString(SourceValue) << '\n';
  }
  OS << "\tSinks (" << TC.SinkValues.size() << "):\n";
  for (const auto *SinkValue : TC.SinkValues) {
    OS << "\t\t" << psr::llvmIRToString(SinkValue) << '\n';
  }
  OS << "\tSanitizers (" << TC.SanitizerValues.size() << "):\n";
  for (const auto *SanitizerValue : TC.SanitizerValues) {
    OS << "\t\t" << psr::llvmIRToString(SanitizerValue) << '\n';
  }
  return OS;
}

nlohmann::json parseTaintConfig(const std::filesystem::path &Path) {
  std::string RawText = readTextFile(Path);
  nlohmann::json TaintConfig(nlohmann::json::parse(RawText));
  nlohmann::json_schema::json_validator Validator;
  try {
    Validator.set_root_schema(TaintConfigSchema); // insert root-schema
  } catch (const std::exception &E) {
    std::cerr << "Validation of schema failed, here is why: " << E.what()
              << "\n";
    return 1;
  }
  // a custom error handler
  class CustomJsonErrorHandler
      : public nlohmann::json_schema::basic_error_handler {
    void error(const nlohmann::json_pointer<nlohmann::basic_json<>> &Pointer,
               const nlohmann::json &Instance,
               const std::string &Message) override {
      nlohmann::json_schema::basic_error_handler::error(Pointer, Instance,
                                                        Message);
      std::cerr << "ERROR: '" << Pointer << "' - '" << Instance
                << "': " << Message << "\n";
    }
  };
  CustomJsonErrorHandler Err;
  Validator.validate(TaintConfig, Err);
  if (Err) {
    std::cerr << "Validation failed\n";
  }
  return TaintConfig;
}

} // namespace psr
