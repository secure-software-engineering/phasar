/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DevAPIs/Taint/TaintConfig.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;
using namespace psr::taintapi;

namespace psr {
namespace taintapi {

// Demangled (plain C) phasar_taint_configuration_api function names
const char *PhasarDeclareCompleteFunAsSourceDemangled =
    "phasar_declare_complete_fun_as_source";
const char *PhasarDeclareFunAsSourceDemangled = "phasar_declare_fun_as_source";
const char *PhasarDeclareCompleteFunAsSanitizerDemangled =
    "phasar_declare_complete_fun_as_sanitizer";
const char *PhasarDeclareFunAsSanitizerDemangled =
    "phasar_declare_fun_as_sanitizer";
const char *PhasarDeclareCompleteFunAsSinkDemangled =
    "phasar_declare_complete_fun_as_sink";
const char *PhasarDeclareFunAsSinkDemangled = "phasar_declare_fun_as_sink";
const char *PhasarDeclareVarAsSourceDemangled = "phasar_declare_var_as_source";
const char *PhasarDeclareVarAsSinkDemangled = "phasar_declare_var_as_sink";
const char *PhasarDeclareVarAsSanitizedDemangled =
    "phasar_declare_var_as_sanitized";
// Mangled (C++) phasar_taint_configuration_api function names
const char *PhasarDeclareCompleteFunAsSourceMangled =
    "_Z37phasar_declare_complete_fun_as_sourcePv";
const char *PhasarDeclareFunAsSourceMangled =
    "_Z28phasar_declare_fun_as_sourcePvbz";
const char *PhasarDeclareCompleteFunAsSanitizerMangled =
    "_Z40phasar_declare_complete_fun_as_sanitizerPv";
const char *PhasarDeclareFunAsSanitizerMangled =
    "_Z31phasar_declare_fun_as_sanitizerPvz";
const char *PhasarDeclareCompleteFunAsSinkMangled =
    "_Z35phasar_declare_complete_fun_as_sinkPv";
const char *PhasarDeclareFunAsSinkMangled = "_Z26phasar_declare_fun_as_sinkPvz";
const char *PhasarDeclareVarAsSourceMangled =
    "_Z28phasar_declare_var_as_sourcePv";
const char *PhasarDeclareVarAsSinkMangled = "_Z26phasar_declare_var_as_sinkPv";
const char *PhasarDeclareVarAsSanitizedMangled =
    "_Z31phasar_declare_var_as_sanitizedPv";

const std::unordered_set<std::string> PhasarTaintConfigurationAPIFunctionNames{
    // C unmangeld version
    PhasarDeclareCompleteFunAsSourceDemangled,
    PhasarDeclareFunAsSourceDemangled,
    PhasarDeclareCompleteFunAsSanitizerDemangled,
    PhasarDeclareFunAsSanitizerDemangled,
    PhasarDeclareCompleteFunAsSinkDemangled, PhasarDeclareFunAsSinkDemangled,
    PhasarDeclareVarAsSourceDemangled, PhasarDeclareVarAsSinkDemangled,
    PhasarDeclareVarAsSanitizedDemangled,
    // C++ mangeld version
    PhasarDeclareCompleteFunAsSourceMangled, PhasarDeclareFunAsSourceMangled,
    PhasarDeclareCompleteFunAsSanitizerMangled,
    PhasarDeclareFunAsSanitizerMangled, PhasarDeclareCompleteFunAsSinkMangled,
    PhasarDeclareFunAsSinkMangled, PhasarDeclareVarAsSourceMangled,
    PhasarDeclareVarAsSinkMangled, PhasarDeclareVarAsSanitizedMangled};

// generate stringified version of enum class TaintType
std::string to_string(TaintType Type) {
  switch (Type) {
  case TaintType::Source:
    return "Source";
    break;
  case TaintType::Sink:
    return "Sink";
    break;
  case TaintType::Sanitizer:
    return "Sanitizer";
    break;
  }
}

TaintType TaintAPIFunctionInfo::getTaintType() const { return Type; };

bool TaintAPIFunctionInfo::isSource() const {
  return (Type == TaintType::Source);
}

bool TaintAPIFunctionInfo::isSink() const { return (Type == TaintType::Sink); }

bool TaintAPIFunctionInfo::isSanitizer() const {
  return (Type == TaintType::Sanitizer);
}

bool TaintAPIFunctionInfo::isCompleteFun() const { return CompleteFun; }

bool TaintAPIFunctionInfo::hasInterestingReturnValue() const { return RetVal; }

const llvm::Function *TaintAPIFunctionInfo::getFunction() const { return Fun; };

std::vector<const llvm::Value *> TaintAPIFunctionInfo::getParams() const {
  return Params;
}

// print the TaintAPIFunctionInfo
std::ostream &operator<<(std::ostream &OS, const TaintAPIFunctionInfo &FI) {
  const llvm::Function *Fun = FI.getFunction();
  OS << to_string(FI.getTaintType()) + " function name: ";
  OS << Fun->getName().str();
  OS << '\n';
  OS << "TaintType: " << to_string(FI.getTaintType()) + "\n";
  // If complete function is Source/Sink/Sanitizer
  if (FI.isCompleteFun()) {
    OS << "Complete Function is " << to_string(FI.getTaintType()) + ": 1";
    OS << '\n';
  } else {
    OS << "Function is " << to_string(FI.getTaintType()) + ": 1";
    OS << '\n';
    OS << "Return value is tainted: " << FI.hasInterestingReturnValue();
    OS << '\n';
    OS << "Tainted Arguments index: ";
    std::vector<const llvm::Value *> Params = FI.getParams();
    for (const auto *Param : Params) {
      OS << llvmIRToString(Param) + " ";
    }

    OS << '\n';
  }
  return OS;
}

TaintAPIFunctionInfo::TaintAPIFunctionInfo(TaintType TP,
                                           const llvm::Function *FN, bool CF,
                                           bool RV,
                                           std::vector<const llvm::Value *> PM)
    : Type(TP), Fun(FN), CompleteFun(CF), RetVal(RV), Params(PM) {}

// Check if the function call belongs to Phasar Taint API
bool isCallToPhasarTaintConfigurationAPI(const llvm::Instruction *I) {
  if (!(llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I))) {
    return false;
  }
  // We found a call instruction
  llvm::ImmutableCallSite CS(I);
  // phasar_taint_configuration_api calls are always direct calls
  const auto *F = CS.getCalledFunction();
  if (!F) {
    return false;
  }
  // Check if function of phasar_taint_configuration_api is being called
  return F->hasName() &&
         PhasarTaintConfigurationAPIFunctionNames.find(F->getName()) !=
             PhasarTaintConfigurationAPIFunctionNames.end();
}

// Check if a variable is declared as Source
bool declaresVarAsSource(const llvm::Instruction *I) {
  if (isCallToPhasarTaintConfigurationAPI(I)) {
    llvm::ImmutableCallSite CS(I);
    const auto *F = CS.getCalledFunction();
    // The check for iterator is not
    // PhasarTaintConfigurationAPIFunctionNamed.end() is done in
    // isCallToPhasarTaintConfigurationAPI(I)
    const auto FuncName =
        PhasarTaintConfigurationAPIFunctionNames.find(F->getName());
    return ((*FuncName == PhasarDeclareVarAsSourceDemangled) ||
            (*FuncName == PhasarDeclareVarAsSourceMangled));
  } else {
    return false;
  }
}
// Check if a variable is declared as Sink
bool declaresVarAsSink(const llvm::Instruction *I) {
  if (isCallToPhasarTaintConfigurationAPI(I)) {
    llvm::ImmutableCallSite CS(I);
    const auto *F = CS.getCalledFunction();
    const auto FuncName =
        PhasarTaintConfigurationAPIFunctionNames.find(F->getName());
    return ((*FuncName == PhasarDeclareVarAsSinkDemangled) ||
            (*FuncName == PhasarDeclareVarAsSinkMangled));
  } else {
    return false;
  }
}
// Check if a variable is declared as Sanitized
bool declaresVarAsSanitized(const llvm::Instruction *I) {
  if (isCallToPhasarTaintConfigurationAPI(I)) {
    llvm::ImmutableCallSite CS(I);
    const auto *F = CS.getCalledFunction();
    const auto FuncName =
        PhasarTaintConfigurationAPIFunctionNames.find(F->getName());
    return ((*FuncName == PhasarDeclareVarAsSanitizedDemangled) ||
            (*FuncName == PhasarDeclareVarAsSanitizedMangled));
  } else {
    return false;
  }
}
// Check if a Function is declared as Source
bool declaresFunAsSource(const llvm::Instruction *I) {
  if (isCallToPhasarTaintConfigurationAPI(I)) {
    llvm::ImmutableCallSite CS(I);
    const auto *F = CS.getCalledFunction();
    const auto FuncName =
        PhasarTaintConfigurationAPIFunctionNames.find(F->getName());
    return ((*FuncName == PhasarDeclareFunAsSourceDemangled) ||
            (*FuncName == PhasarDeclareFunAsSourceMangled) ||
            (*FuncName == PhasarDeclareCompleteFunAsSourceDemangled) ||
            (*FuncName == PhasarDeclareCompleteFunAsSourceMangled));
  } else {
    return false;
  }
}
// Check if a Function is declared as Sink
bool declaresFunAsSink(const llvm::Instruction *I) {
  if (isCallToPhasarTaintConfigurationAPI(I)) {
    llvm::ImmutableCallSite CS(I);
    const auto *F = CS.getCalledFunction();
    const auto FuncName =
        PhasarTaintConfigurationAPIFunctionNames.find(F->getName());
    return ((*FuncName == PhasarDeclareFunAsSinkDemangled) ||
            (*FuncName == PhasarDeclareFunAsSinkMangled) ||
            (*FuncName == PhasarDeclareCompleteFunAsSinkDemangled) ||
            (*FuncName == PhasarDeclareCompleteFunAsSinkMangled));
  } else {
    return false;
  }
}
// Check if a Function is declared as Sanitizer
bool declaresFunAsSanitizer(const llvm::Instruction *I) {
  if (isCallToPhasarTaintConfigurationAPI(I)) {
    llvm::ImmutableCallSite CS(I);
    const auto *F = CS.getCalledFunction();
    const auto FuncName =
        PhasarTaintConfigurationAPIFunctionNames.find(F->getName());
    return ((*FuncName == PhasarDeclareFunAsSanitizerDemangled) ||
            (*FuncName == PhasarDeclareFunAsSanitizerMangled) ||
            (*FuncName == PhasarDeclareCompleteFunAsSanitizerDemangled) ||
            (*FuncName == PhasarDeclareCompleteFunAsSanitizerMangled));
  } else {
    return false;
  }
}

// Return variable name declared as source
const llvm::Value *getVarDeclaredAsSource(const llvm::Instruction *I) {
  if (declaresVarAsSource(I)) {
    llvm::ImmutableCallSite CS(I);
    if (CS.getNumArgOperands() > 0) {
      const llvm::Value *V = CS.getArgOperand(0);
      return V;
    } else {
      return nullptr;
    }
  } else {
    return nullptr;
  }
}
// Return variable name declared as sink
const llvm::Value *getVarDeclaredAsSink(const llvm::Instruction *I) {
  if (declaresVarAsSink(I)) {
    llvm::ImmutableCallSite CS(I);
    if (CS.getNumArgOperands() > 0) {
      const llvm::Value *V = CS.getArgOperand(0);
      return V;
    } else {
      return nullptr;
    }
  } else {
    return nullptr;
  }
}
// Return variable name declared as sanitized
const llvm::Value *getVarDeclaredAsSanitized(const llvm::Instruction *I) {
  if (declaresVarAsSanitized(I)) {
    llvm::ImmutableCallSite CS(I);
    if (CS.getNumArgOperands() > 0) {
      const llvm::Value *V = CS.getArgOperand(0);
      return V;
    } else {
      return nullptr;
    }
  } else {
    return nullptr;
  }
}

// Return data for Functions declared as Source/Sink/Sanitizer
TaintAPIFunctionInfo getAPIFunctionInfo(const llvm::Instruction *I) {

  // TODO this still looks like a setter
  // introduce a contructor to TaintAPIFunctionInfo that accepts a const
  // llvm::Instruction* implement this function to just return return
  // TaintAPIFunctionInfo{I};

  TaintType Type;
  const llvm::Function *Fun;
  bool CompleteFun;
  bool RetVal;
  std::vector<const llvm::Value *> Params;
  llvm::ImmutableCallSite CS(I);
  if (declaresFunAsSource(I)) {
    Type = TaintType::Source;
  } else if (declaresFunAsSink(I)) {
    Type = TaintType::Sink;
  } else if (declaresFunAsSanitizer(I)) {
    Type = TaintType::Sanitizer;
  }
  Fun = static_cast<const llvm::Function *>(CS.getArgOperand(0));
  // if the function has return value marked as Source or arguments marked as
  // Source/Sink/Sanitized
  if (CS.getNumArgOperands() > 1) {
    // Mark the function as not completely Source/Sink/Sanitizer
    // TODO this looks wrong, please fix the following lines
    std::string RetValString = llvmIRToString(CS.getArgOperand(1));
    if (RetValString.find("true") != std::string::npos) {
      RetVal = true;
    } else {
      RetVal = false;
    }
    int ArgOperandIndex = 2;
    // Set Argument index for Sink or Sanitizer functions as 1 since there is no
    // return value for Sink and Sanitizer functions
    if (Type == TaintType::Sink || Type == TaintType::Sanitizer) {
      ArgOperandIndex = 1;
    }
    for (unsigned I = ArgOperandIndex; I < CS.getNumArgOperands(); ++I) {
      const llvm::Value *Index = CS.getArgOperand(I);
      Params.insert(Params.begin(), Index);
    }
    CompleteFun = false;
  } else {
    CompleteFun = true;
    RetVal = true;
  }
  TaintAPIFunctionInfo funInfo(Type, Fun, CompleteFun, RetVal, Params);
  return funInfo;
}

std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
makeInitialSeedFromSourceDeclarations(const ProjectIRDB &IRDB,
                                      const LLVMBasedICFG &ICF) {
  std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
      InitialSeeds;
  // TODO implement
  return InitialSeeds;
}

} // namespace taintapi
} // namespace psr
