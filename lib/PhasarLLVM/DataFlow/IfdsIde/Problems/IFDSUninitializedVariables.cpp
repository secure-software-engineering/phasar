/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <utility>

namespace psr {

IFDSUninitializedVariables::IFDSUninitializedVariables(
    const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
    std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()),
      PT(PT) {
  assert(PT &&
         "IFDSUninitializedVariables requires the alias info not to be null!");
}

static bool isMatchingType(const llvm::Type *Ty) noexcept {
  return Ty->isIntegerTy() || Ty->isFloatingPointTy() || Ty->isPointerTy() ||
         Ty->isArrayTy();
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getNormalFlowFunction(n_t Curr, n_t /*Succ*/) {

  if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    // Allocas of primitive types are uninitialized by default
    if (isMatchingType(Alloc->getAllocatedType())) {
      return generateFromZero(Alloc);
    }
  }

  // check the all store instructions and kill initialized variables
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    auto AA = PT.getAliasSet(Store->getPointerOperand(), Store);

    if (llvm::isa<llvm::UndefValue>(Store->getValueOperand())) {
      return lambdaFlow<d_t>([AA](d_t Source) -> container_type {
        container_type Ret = {Source};
        if (LLVMZeroValue::isLLVMZeroValue(Source)) {
          Ret.insert(AA->begin(), AA->end());
        }
        return Ret;
      });
    }

    return lambdaFlow<d_t>([AA, Store](d_t Source) -> container_type {
      if (Source == Store->getValueOperand()) {
        container_type Ret;
        if (!isDefiniteLastUse(Store->getValueOperand())) {
          Ret.insert(Source);
        }
        Ret.insert(AA->begin(), AA->end());
        return Ret;
      }

      if (Source == Store->getPointerOperand()) {
        return {};
      }

      return {Source};
    });
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return lambdaFlow<d_t>([this, Load](d_t Source) -> container_type {
      auto IsDefinitivelyInitialized = [](d_t Source) {
        // TODO: Be more precise here
        return llvm::isa<llvm::GlobalValue>(Source);
      };

      if (Source == Load->getPointerOperand()) {
        if (!IsDefinitivelyInitialized(Source)) {
          UndefValueUses[Load].insert(Load->getPointerOperand());
        }
        container_type Ret;

        Ret.insert(Load);

        if (!isDefiniteLastUse(Load->getOperandUse(
                llvm::LoadInst::getPointerOperandIndex()))) {
          Ret.insert(Source);
        }

        return Ret;
      }

      return {Source};
    });
  }

  if (!Curr->getType()->isVoidTy()) {
    return lambdaFlow<d_t>([Curr](d_t Source) -> container_type {
      for (const auto &Op : Curr->operands()) {
        if (Source == Op) {
          container_type Ret = {Curr};

          if (!isDefiniteLastUse(Op)) {
            Ret.insert(Source);
          }

          return Ret;
        }
      }

      return {Source};
    });
  }

  return identityFlow<d_t>();
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getCallFlowFunction(n_t CallSite, f_t DestFun) {

  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);

  return mapFactsToCallee(Call, DestFun);
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                               n_t ExitStmt, n_t /*RetSite*/) {
  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);

  // TODO: Propagate back aliases

  return mapFactsToCaller(Call, ExitStmt, [](d_t Param, d_t Source) {
    return Param == Source && Param->getType()->isPointerTy();
  });
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getCallToRetFlowFunction(
    n_t CallSite, n_t /*RetSite*/, llvm::ArrayRef<f_t> Callees) {

  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);
  llvm::SmallBitVector LeakArgs(Call->arg_size());
  llvm::SmallBitVector SanitizerArgs(Call->arg_size());

  bool HasDeclOnly = false;

  for (const auto *Callee : Callees) {
    if (Callee->isDeclaration()) {
      HasDeclOnly = true;
      for (const auto &Arg : Callee->args()) {
        if (!isMatchingType(Arg.getType())) {
          continue;
        }
        if (!Arg.getType()->isPointerTy() ||
            Arg.hasPassPointeeByValueCopyAttr() || Arg.onlyReadsMemory()) {
          LeakArgs.set(Arg.getArgNo());
        } else if (Arg.getType()->isPointerTy() ||
                   isDefiniteLastUse(Call->getArgOperandUse(Arg.getArgNo()))) {
          // Here we may be unsound!
          // We just assume that the function will write to the pointer and
          // therefore initialize it
          SanitizerArgs.set(Arg.getArgNo());
        }
      }
    }
  }

  if (HasDeclOnly) {
    return lambdaFlow<d_t>([this, Call, LeakArgs{std::move(LeakArgs)},
                            SanitizerArgs{std::move(SanitizerArgs)}](
                               d_t Source) -> container_type {
      // Pass ZeroValue as is
      if (LLVMZeroValue::isLLVMZeroValue(Source)) {
        return {Source};
      }
      // Pass global variables as is
      // Need llvm::Constant here to cover also ConstantExpr and
      // ConstantAggregate
      if (llvm::isa<llvm::Constant>(Source)) {
        return {Source};
      }

      unsigned ArgIdx = 0;
      for (const auto &Arg : Call->args()) {
        if (Arg.get() == Source) {
          if (LeakArgs.test(ArgIdx)) {
            UndefValueUses[Call].insert(Arg);
          }

          if (SanitizerArgs.test(ArgIdx)) {
            return {};
          }

          return {Arg.get()};
        }
        ++ArgIdx;
      }

      return {Source};
    });
  }
  return mapFactsAlongsideCallSite(
      Call, [](d_t Arg) { return !Arg->getType()->isPointerTy(); });
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getSummaryFlowFunction(n_t /*CallSite*/,
                                                   f_t /*DestFun*/) {
  return nullptr;
}

auto IFDSUninitializedVariables::initialSeeds() -> InitialSeeds<n_t, d_t, l_t> {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSUninitializedVariables::initialSeeds()");
  return createDefaultSeeds();
}

auto IFDSUninitializedVariables::createZeroValue() const -> d_t {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSUninitializedVariables::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSUninitializedVariables::isZeroValue(d_t Fact) const {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

void IFDSUninitializedVariables::emitTextReport(
    const SolverResults<n_t, d_t, l_t> & /*Result*/, llvm::raw_ostream &OS) {
  OS << "====================== IFDS-Uninitialized-Analysis Report "
        "======================\n";
  if (UndefValueUses.empty()) {
    OS << "No uses of uninitialized variables found by the analysis!\n";
  } else {
    if (!IRDB->debugInfoAvailable()) {
      // Emit only IR code, function name and module info
      OS << "\nWARNING: No Debug Info available - emiting results without "
            "source code mapping!\n";
      OS << "\nTotal uses of uninitialized IR Value's: "
         << UndefValueUses.size() << '\n';
      size_t Count = 0;
      for (const auto &User : UndefValueUses) {
        OS << "\n---------------------------------  " << ++Count
           << ". Use  ---------------------------------\n\n";
        OS << "At IR statement: " << NToString(User.first);
        OS << "\n    in function: " << getFunctionNameFromIR(User.first);
        OS << "\n    in module  : " << getModuleIDFromIR(User.first) << "\n\n";
        for (const auto *UndefV : User.second) {
          OS << "   Uninit Value: " << DToString(UndefV);
          OS << '\n';
        }
      }
      OS << '\n';
    } else {
      auto UninitResults = aggregateResults();
      OS << "\nTotal uses of uninitialized variables: " << UninitResults.size()
         << '\n';

      std::sort(UninitResults.begin(), UninitResults.end(),
                [](const UninitResult &R1, const UninitResult &R2) {
                  return std::tie(R1.FilePath, R1.Line) <
                         std::tie(R2.FilePath, R2.Line);
                });

      size_t Count = 0;
      for (auto Res : UninitResults) {
        OS << "\n---------------------------------  " << ++Count
           << ". Use  ---------------------------------\n\n";
        Res.print(OS);
      }
    }
  }
}

auto IFDSUninitializedVariables::aggregateResults()
    -> std::vector<UninitResult> {
  std::vector<UninitResult> Results;
  Results.reserve(UndefValueUses.size());

  unsigned int LineNr = 0;
  unsigned int CurrLineNr = 0;

  UninitResult UR;

  auto Push = [&UR, &Results] {
    if (!UR.empty()) {
      std::sort(UR.IRTrace.begin(), UR.IRTrace.end(),
                [](const auto &TR1, const auto &TR2) {
                  return LLVMValueIDLess{}(TR1.first, TR2.first);
                });
      Results.push_back(std::move(UR));
    }
  };

  for (const auto &User : UndefValueUses) {
    // new line nr idicates a new uninit use on source code level
    LineNr = getLineFromIR(User.first);
    if (CurrLineNr != LineNr) {
      CurrLineNr = LineNr;

      Push();

      UR.Line = LineNr;
      UR.FuncName = llvm::demangle(getFunctionNameFromIR(User.first));
      UR.FilePath = getFilePathFromIR(User.first);
      UR.SrcCode = getSrcCodeFromIR(User.first);
    }
    // add current IR trace
    UR.IRTrace.push_back(User);
    // add (possibly) new variable names
    for (const auto *UndefV : User.second) {
      auto VarName = getVarNameFromIR(UndefV);
      if (!VarName.empty()) {
        UR.VarNames.push_back(std::move(VarName));
      }
    }
  }

  Push();
  return Results;
}

bool IFDSUninitializedVariables::UninitResult::empty() const {
  return Line == 0;
}

void IFDSUninitializedVariables::UninitResult::print(
    llvm::raw_ostream &OS) const {
  OS << "Variable(s): ";
  if (!VarNames.empty()) {
    for (size_t I = 0; I < VarNames.size(); ++I) {
      OS << VarNames[I];
      if (I < VarNames.size() - 1) {
        OS << ", ";
      }
    }
    OS << '\n';
  }
  OS << "Line       : " << Line << '\n';
  OS << "Source code: " << SrcCode << '\n';
  OS << "Function   : " << FuncName << '\n';
  OS << "File       : " << FilePath << '\n';
  OS << "\nCorresponding IR Statements and uninit. Values\n";
  if (!IRTrace.empty()) {
    for (const auto &Trace : IRTrace) {
      OS << "At IR Statement: " << llvmIRToString(Trace.first) << '\n';
      for (const auto *IRVal : Trace.second) {
        OS << "   Uninit Value: " << llvmIRToString(IRVal) << '\n';
      }
      // os << '\n';
    }
  }
}

} // namespace psr
