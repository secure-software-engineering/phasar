/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"
#include "phasar/PhasarLLVM/Utils/Annotation.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MapUtils.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"

#include <string>

namespace psr {

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

void LLVMTaintConfig::addAllFunctions(const LLVMProjectIRDB &IRDB,
                                      const TaintConfigData &Config) {
  for (const auto &FunDesc : Config.Functions) {
    const auto &Name = FunDesc.Name;

    auto FnDefs = findAllFunctionDefs(IRDB, Name);

    if (FnDefs.empty()) {
      llvm::errs() << "WARNING: Cannot retrieve function " << Name << "\n";
      continue;
    }

    const auto *Fun = FnDefs[0];

    // handle a function's source parameters
    for (const auto &Idx : FunDesc.SourceValues) {
      if (Idx >= Fun->arg_size()) {
        llvm::errs() << "ERROR: The source-function parameter index is out of "
                        "bounds: "
                     << Idx << "\n";
        // Use 'continue' instead of 'break' to get error messages for the
        // remaining parameters as well
        continue;
      }

      addTaintCategory(Fun->getArg(Idx), TaintCategory::Source);
    }
    for (const auto &Idx : FunDesc.SinkValues) {
      if (Idx >= Fun->arg_size()) {
        llvm::errs() << "ERROR: The sink-function parameter index is out of "
                        "bounds: "
                     << Idx << "\n";
        continue;
      }

      addTaintCategory(Fun->getArg(Idx), TaintCategory::Sink);
    }

    if (FunDesc.HasAllSinkParam) {
      for (const auto &Arg : Fun->args()) {
        addTaintCategory(&Arg, TaintCategory::Sink);
      }
    }

    for (const auto &Idx : FunDesc.SanitizerValues) {
      if (Idx >= Fun->arg_size()) {
        llvm::errs()
            << "ERROR: The sanitizer-function parameter index is out of "
               "bounds: "
            << Idx << "\n";
        continue;
      }
      addTaintCategory(Fun->getArg(Idx), TaintCategory::Sanitizer);
    }
    // handle a function's return value
    for (const auto &User : Fun->users()) {
      addTaintCategory(User, FunDesc.ReturnCat);
    }
  }
}

void LLVMTaintConfig::addAllVariables(const LLVMProjectIRDB &IRDB,
                                      const TaintConfigData &Config) {

  if (Config.Variables.empty()) {
    return;
  }

  // Map LLVM struct type → configured field name, built by correlating
  // debug-declare records with allocas (avoids IR type naming assumptions).
  llvm::DenseMap<const llvm::Type *, std::string_view> StructConfigMap;

  {
    // Pre-build scope → field name for O(1) lookup during the alloca scan.
    llvm::StringMap<std::string_view> StructScopeIndex;
    for (const auto &VarDesc : Config.Variables) {
      StructScopeIndex.try_emplace(VarDesc.Scope, VarDesc.Name);
    }

    for (const auto *Fun : IRDB.getAllFunctions()) {
      for (const auto &I : llvm::instructions(Fun)) {
        forEachDbgDeclare(I, [&](const llvm::DILocalVariable *LocalVar,
                                 const llvm::Value *Addr) {
          const auto *AllocaI = llvm::dyn_cast<llvm::AllocaInst>(Addr);
          if (!AllocaI) {
            return;
          }
          const auto *StructTy =
              llvm::dyn_cast<llvm::StructType>(AllocaI->getAllocatedType());
          if (!StructTy) {
            return;
          }
          const auto *CT = stripTypedefsToStruct(LocalVar->getType());
          if (!CT) {
            return;
          }
          if (const auto *Scope = getOrNull(StructScopeIndex, CT->getName())) {
            StructConfigMap.try_emplace(StructTy, *Scope);
          }
        });
      }
    }
  }

  const auto AddVariable = [this](const VariableData &VarDesc,
                                  const llvm::Instruction &I) {
    forEachDbgDeclare(I, [&](const llvm::DILocalVariable *Var,
                             const llvm::Value *Addr) {
      if (Var->getLine() == VarDesc.Line && Var->getName() == VarDesc.Name) {
        addTaintCategory(Addr, VarDesc.Cat);
      }
    });
  };

  for (const auto &VarDesc : Config.Variables) {
    for (const auto *Fun : IRDB.getAllFunctions()) {
      const bool MatchingFunScope = [&] {
        if (VarDesc.Scope == Fun->getName()) {
          return true;
        }
        const auto *Subp = Fun->getSubprogram();
        return Subp != nullptr && VarDesc.Name == Subp->getName();
      }();

      for (const auto &I : llvm::instructions(Fun)) {
        if (MatchingFunScope) {
          AddVariable(VarDesc, I);
        }

        if (!StructConfigMap.empty()) {
          if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(&I)) {
            if (const auto *StType = llvm::dyn_cast<llvm::StructType>(
                    Gep->getSourceElementType())) {
              // using substr to cover the edge case in which same variable
              // name is present as a local variable and also as a struct
              // member variable. (Ex. JsonConfig/fun_member_02.cpp)
              if (const auto *Scope = getOrNull(StructConfigMap, StType);
                  Scope && Gep->getName().starts_with(*Scope)) {
                addTaintCategory(Gep, VarDesc.Cat);
              }
            }
          }
        }
      }
    }
  }
}

LLVMTaintConfig::LLVMTaintConfig(const psr::LLVMProjectIRDB &Code,
                                 const TaintConfigData &Config) {
  addAllFunctions(Code, Config);
  addAllVariables(Code, Config);
}

LLVMTaintConfig::LLVMTaintConfig(const psr::LLVMProjectIRDB &AnnotatedCode) {

  llvm::SmallVector<const llvm::Function *, 1> VarAnnotations{};
  llvm::SmallVector<const llvm::Function *, 1> PtrAnnotations{};
  for (const auto *F : AnnotatedCode.getAllFunctions()) {
    if (F->getIntrinsicID() == llvm::Intrinsic::var_annotation) {
      VarAnnotations.push_back(F);
    }
    if (F->getIntrinsicID() == llvm::Intrinsic::ptr_annotation) {
      PtrAnnotations.push_back(F);
    }
  }

  // handle "local" annotation declarations
  for (const auto *Annotation : VarAnnotations) {
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

LLVMTaintConfig::LLVMTaintConfig(
    TaintDescriptionCallBackTy SourceCB, TaintDescriptionCallBackTy SinkCB,
    TaintDescriptionCallBackTy SanitizerCB) noexcept {
  registerSourceCallBack(std::move(SourceCB));
  registerSinkCallBack(std::move(SinkCB));
  registerSanitizerCallBack(std::move(SanitizerCB));
}

//
// --- Own API function implementations
//

void LLVMTaintConfig::addSourceValue(const llvm::Value *V) {
  SourceValues.insert(V);
}

void LLVMTaintConfig::addSinkValue(const llvm::Value *V) {
  SinkValues.insert(V);
}

void LLVMTaintConfig::addSanitizerValue(const llvm::Value *V) {
  SanitizerValues.insert(V);
}

void LLVMTaintConfig::addTaintCategory(const llvm::Value *Val,
                                       llvm::StringRef AnnotationStr) {
  auto TC = toTaintCategory(AnnotationStr);
  if (TC == TaintCategory::None) {
    PHASAR_LOG_LEVEL(ERROR, "Unknown taint category: " << AnnotationStr);
  } else {
    addTaintCategory(Val, TC);
  }
}

void LLVMTaintConfig::addTaintCategory(const llvm::Value *Val,
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

//
// --- TaintConfigBase API function implementations
//

bool LLVMTaintConfig::isSourceImpl(const llvm::Value *V) const {
  return SourceValues.contains(V);
}
bool LLVMTaintConfig::isSinkImpl(const llvm::Value *V) const {
  return SinkValues.contains(V);
}
bool LLVMTaintConfig::isSanitizerImpl(const llvm::Value *V) const {
  return SanitizerValues.contains(V);
}

void LLVMTaintConfig::forAllGeneratedValuesAtImpl(
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
      if (SourceValues.contains(&Arg)) {
        Handler(Inst->getOperand(Arg.getArgNo()));
      }
    }
  } else {
    /// If we have a call to a source function, we would generate via formal
    /// parameter instead via actual argument.
    /// If any function is called with a variable that was defined as source, we
    /// don't want to re-generate the value.
    for (const auto *Op : Inst->operand_values()) {
      if (SourceValues.contains(Op)) {
        Handler(Op);
      }
    }
  }

  if (SourceValues.contains(Inst)) {
    Handler(Inst);
  }
}

void LLVMTaintConfig::forAllLeakCandidatesAtImpl(
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
      if (SinkValues.contains(&Arg)) {
        Handler(Inst->getOperand(Arg.getArgNo()));
      }
    }
  }

  // Do not iterate over the actual parameters of Inst as we did in
  // forAllGeneratedValuesAt, because sink-values are not propagated in the
  // current taint analyses. Handling sink-values should be done in the
  // SinkCallBack
}

void LLVMTaintConfig::forAllSanitizedValuesAtImpl(
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
      if (SanitizerValues.contains(&Arg)) {
        Handler(Inst->getOperand(Arg.getArgNo()));
      }
    }
  }
}

bool LLVMTaintConfig::generatesValuesAtImpl(
    const llvm::Instruction *Inst, const llvm::Function *Callee) const {
  assert(Inst != nullptr);

  if (SourceCallBack && !SourceCallBack(Inst).empty()) {
    return true;
  }

  if (Callee && llvm::any_of(Callee->args(), [this](const auto &Arg) {
        return SourceValues.count(&Arg);
      })) {
    return true;
  }

  return SourceValues.contains(Inst) ||
         llvm::any_of(Inst->operand_values(), [this](const auto *Op) {
           return SourceValues.count(Op);
         });
}

bool LLVMTaintConfig::mayLeakValuesAtImpl(const llvm::Instruction *Inst,
                                          const llvm::Function *Callee) const {
  assert(Inst != nullptr);

  if (SinkCallBack && !SinkCallBack(Inst).empty()) {
    return true;
  }

  return Callee && llvm::any_of(Callee->args(), [this](const auto &Arg) {
           return SinkValues.count(&Arg);
         });
}

bool LLVMTaintConfig::sanitizesValuesAtImpl(
    const llvm::Instruction *Inst, const llvm::Function *Callee) const {
  assert(Inst != nullptr);

  if (SanitizerCallBack && !SanitizerCallBack(Inst).empty()) {
    return true;
  }

  return Callee && llvm::any_of(Callee->args(), [this](const auto &Arg) {
           return SanitizerValues.count(&Arg);
         });
}

TaintCategory LLVMTaintConfig::getCategoryImpl(const llvm::Value *V) const {
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

std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
LLVMTaintConfig::makeInitialSeedsImpl(SeedConfig Conf) const {
  std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
      InitialSeeds;
  for (const auto *SourceValue : SourceValues) {
    if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(SourceValue)) {
      if (hasFlag(Conf, SeedConfig::Instructions)) {
        InitialSeeds[Inst].insert(Inst);
      }
    } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(SourceValue);
               Arg && !Arg->getParent()->isDeclaration()) {
      if (hasFlag(Conf, SeedConfig::Arguments)) {
        LLVMBasedCFG C;
        for (const auto *SP : C.getStartPointsOf(Arg->getParent())) {
          InitialSeeds[SP].insert(Arg);
        }
      }
    }
  }
  return InitialSeeds;
}

void LLVMTaintConfig::printImpl(llvm::raw_ostream &OS) const {
  OS << "TaintConfiguration: ";
  if (SourceValues.empty() && SinkValues.empty() && SanitizerValues.empty() &&
      !getRegisteredSourceCallBack() && !getRegisteredSinkCallBack()) {
    OS << "empty\n";
    return;
  }
  OS << "\n\tSourceCallBack registered: " << (bool)SourceCallBack << '\n';
  OS << "\tSinkCallBack registered: " << (bool)SinkCallBack << '\n';
  OS << "\tSources (" << SourceValues.size() << "):\n";
  for (const auto *SourceValue : SourceValues) {
    OS << "\t\t" << psr::llvmIRToString(SourceValue) << '\n';
  }
  OS << "\tSinks (" << SinkValues.size() << "):\n";
  for (const auto *SinkValue : SinkValues) {
    OS << "\t\t" << psr::llvmIRToString(SinkValue) << '\n';
  }
  OS << "\tSanitizers (" << SanitizerValues.size() << "):\n";
  for (const auto *SanitizerValue : SanitizerValues) {
    OS << "\t\t" << psr::llvmIRToString(SanitizerValue) << '\n';
  }
}

template class TaintConfigBase<LLVMTaintConfig>;
} // namespace psr
