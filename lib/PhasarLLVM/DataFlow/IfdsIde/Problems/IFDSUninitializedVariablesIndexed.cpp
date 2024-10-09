/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariablesIndexed.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/IndexWrapper.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include <cstddef>
#include <set>
#include <string>

#include <llvm-14/llvm/Support/Casting.h>

namespace psr {

IFDSUninitializedVariablesIndexed::IFDSUninitializedVariablesIndexed(
    const LLVMProjectIRDB *IRDB, std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IFDSUninitializedVariablesIndexed::FlowFunctionPtrType
IFDSUninitializedVariablesIndexed::getNormalFlowFunction(n_t Curr,
                                                         n_t /*Succ*/) {
  auto Zero = getZeroValue();
  /**
   * an ExtractValueInst will become Source if some Source Fact contains the
   * extracted Value
   */
  if (const auto *ExtractValue = llvm::dyn_cast<llvm::ExtractValueInst>(Curr)) {
    return lambdaFlow([ExtractValue](const d_t &Source) -> std::set<d_t> {
      if (Source.contains(*ExtractValue)) {
        return {Source, IndexWrapper<llvm::Value>(ExtractValue)};
      }
      return {Source};
    });
  }
  if (const auto *GetElementPtr =
          llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return lambdaFlow([GetElementPtr](const d_t &Source) -> std::set<d_t> {
      if (Source.contains(*GetElementPtr)) {
        return {Source, IndexWrapper<llvm::Value>(GetElementPtr)};
      }
      return {Source};
    });
  }
  /**
   * We have to distinghuishe the cases that something gets inserted in some
   * undefined/Source Value, or som undegined/Source Value gets inserted into
   * something If both are undefined the whole object gets undefined by the
   * IndexWrapper(InsertValue) fact It is important to notice, that this dosn't
   * prevent additional indexed Facts to be generated because the Sources "don't
   * know of each other" If something defined gets inserted into something
   * undefined/source we generate a Source fact for all indices but the one
   * inserted If something undefined/source gets inserted into something defined
   * we only generate the Source fact for this index
   */
  if (const auto *InsertValue = llvm::dyn_cast<llvm::InsertValueInst>(Curr)) {
    return lambdaFlow([InsertValue, Zero](const d_t &Source) -> std::set<d_t> {
      const llvm::Value *AggregatOperand = InsertValue->getAggregateOperand();
      const auto *InsertOperand = InsertValue->getInsertedValueOperand();
      std::set<d_t> ReturnSet = {Source};
      auto Indices = InsertValue->getIndices();

      if ((Source.getValue() == Zero.getValue() &&
           llvm::isa<llvm::UndefValue>(AggregatOperand)) ||
          Source.getValue() == AggregatOperand) {
        if ((Source.getValue() == Zero.getValue() &&
             llvm::isa<llvm::UndefValue>(InsertOperand)) ||
            Source.getValue() == InsertOperand) {
          // IsertOperand and AggregateOperand are both undefined so the whole
          // InsertValue becomes source
          ReturnSet.insert(IndexWrapper<llvm::Value>(InsertValue));
        } else {
          // only AggregateOperand is undefined so we have to exclude the
          // inserted value
          auto Excluded =
              IndexWrapper<llvm::Value>(InsertValue).excluded(Indices);
          ReturnSet.insert(Excluded.begin(), Excluded.end());
        }
      } else if ((Source.getValue() == Zero.getValue() &&
                  llvm::isa<llvm::UndefValue>(InsertOperand)) ||
                 Source.getValue() == InsertOperand) {
        // only the InsertOperand is undefiend so only this becomes source
        ReturnSet.insert(IndexWrapper<llvm::Value>(InsertValue, Indices.vec()));
      };

      return ReturnSet;
    });
  }
  // check the store instructions and kill initialized variables
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return lambdaFlow([Store, Zero](const d_t &Source) -> std::set<d_t> {
      if (Source.getValue() == Store->getValueOperand() ||
          (Source.getValue() == Zero.getValue() &&
           llvm::isa<llvm::UndefValue>(Store->getValueOperand()))) {
        d_t PointerOperand = IndexWrapper(Store->getPointerOperand());
        return {Source, PointerOperand};
      }
      if (Source.getValue() == Store->getPointerOperand() &&
          !llvm::isa<llvm::UndefValue>(
              Store->getValueOperand())) { // storing an initialized value
        // kills the variable as it is
        // now initialized too
        return {};
      }
      // pass all other facts as identity
      return {Source};
    });
  }
  if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    return lambdaFlow([Alloc, this](const d_t &Source) -> std::set<d_t> {
      if (isZeroValue(Source)) {
        if (Alloc->getAllocatedType()->isIntegerTy() ||
            Alloc->getAllocatedType()->isFloatingPointTy() ||
            Alloc->getAllocatedType()->isPointerTy() ||
            Alloc->getAllocatedType()->isArrayTy() ||
            Alloc->getAllocatedType()->isStructTy()) {
          // generate the alloca
          return {Source, IndexWrapper<llvm::Value>(Alloc)};
        }
      }
      // otherwise propagate all facts
      return {Source};
    });
  }
  const llvm::Instruction *Inst = Curr;
  return lambdaFlow([Inst, this](const d_t &Source) -> std::set<d_t> {
    for (const auto &Operand : Inst->operands()) {
      if (Operand == Source.getValue() ||
          llvm::isa<llvm::UndefValue>(Operand)) {
        if (!llvm::isa<llvm::GetElementPtrInst>(Inst) &&
            !llvm::isa<llvm::CastInst>(Inst) &&
            !llvm::isa<llvm::PHINode>(Inst)) {
          UndefValueUses[Inst].insert(IndexWrapper<llvm::Value>(Operand));
        }
        return {Source, IndexWrapper<llvm::Value>(Inst)};
      }
    }
    return {Source};
  });
}

IFDSUninitializedVariablesIndexed::FlowFunctionPtrType
IFDSUninitializedVariablesIndexed::getCallFlowFunction(n_t CallSite,
                                                       f_t DestFun) {
  if (llvm::isa<llvm::CallInst>(CallSite) ||
      llvm::isa<llvm::InvokeInst>(CallSite)) {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    const auto *Zerovalue = getZeroValue().getValue();
    std::vector<const llvm::Value *> Actuals;
    std::vector<const llvm::Value *> Formals;
    // set up the actual parameters
    for (unsigned Idx = 0; Idx < CS->arg_size(); ++Idx) {
      Actuals.push_back(CS->getArgOperand(Idx));
    }
    // set up the formal parameters
    /*for (unsigned idx = 0; idx < destFun->arg_size(); ++idx) {
      formals.push_back(getNthFunctionArgument(destFun, idx));
    }*/
    for (const auto &Arg : DestFun->args()) {
      Formals.push_back(&Arg);
    }
    return lambdaFlow(
        [Zerovalue, Actuals, Formals](const d_t &Source) -> std::set<d_t> {
          // perform parameter passing
          if (Source.getValue() != Zerovalue) {
            std::set<d_t> Res;
            // do the mapping from actual to formal parameters
            // caution: the loop iterates from 0 to formals.size(),
            // rather than actuals.size() as we may have more actual
            // than formal arguments in case of C-style varargs
            for (unsigned Idx = 0; Idx < Formals.size(); ++Idx) {
              if (Source.getValue() == Actuals[Idx]) {
                Res.insert(IndexWrapper(Formals[Idx]));
              }
            }
            return Res;
          }
          return {Source};
        });
  }
  return identityFlow();
}

IFDSUninitializedVariablesIndexed::FlowFunctionPtrType
IFDSUninitializedVariablesIndexed::getRetFlowFunction(n_t CallSite,
                                                      f_t /*CalleeFun*/,
                                                      n_t ExitStmt,
                                                      n_t /*RetSite*/) {
  if (llvm::isa<llvm::CallInst>(CallSite) ||
      llvm::isa<llvm::InvokeInst>(CallSite)) {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    return lambdaFlow([CS, ExitStmt](const d_t &Source) -> std::set<d_t> {
      std::set<d_t> Ret;
      if (ExitStmt->getNumOperands() > 0 &&
          ExitStmt->getOperand(0) == Source.getValue()) {
        Ret.insert(IndexWrapper<llvm::Value>(CS));
      }
      //----------------------------------------------------------------------
      // Handle pointer/reference parameters
      //----------------------------------------------------------------------
      if (CS->getCalledFunction()) {
        unsigned I = 0;
        for (const auto &Arg : CS->getCalledFunction()->args()) {
          // auto arg = getNthFunctionArgument(call.getCalledFunction(), i);
          if (&Arg == Source.getValue() && Arg.getType()->isPointerTy()) {
            Ret.insert(IndexWrapper(CS->getArgOperand(I)));
          }
          I++;
        }
      }
      // kill all other facts
      return Ret;
    });
  }
  // kill everything else
  return killAllFlows();
}

IFDSUninitializedVariablesIndexed::FlowFunctionPtrType
IFDSUninitializedVariablesIndexed::getCallToRetFlowFunction(
    n_t CallSite, n_t /*RetSite*/, llvm::ArrayRef<f_t> /*Callees*/) {
  //----------------------------------------------------------------------
  // Handle pointer/reference parameters
  //----------------------------------------------------------------------
  if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    return lambdaFlow([CS](const d_t &Source) -> std::set<d_t> {
      if (Source.getValue()->getType()->isPointerTy()) {
        for (const auto &Arg : CS->args()) {
          if (Arg.get() == Source.getValue()) {
            // do not propagate pointer arguments, since the function may
            // initialize them (would be much more precise with
            // field-sensitivity)
            return {};
          }
        }
      }
      return {Source};
    });
  }
  return identityFlow();
}

IFDSUninitializedVariablesIndexed::FlowFunctionPtrType
IFDSUninitializedVariablesIndexed::getSummaryFlowFunction(n_t /*CallSite*/,
                                                          f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSUninitializedVariablesIndexed::n_t,
             IFDSUninitializedVariablesIndexed::d_t,
             IFDSUninitializedVariablesIndexed::l_t>
IFDSUninitializedVariablesIndexed::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSUninitializedVariablesStructs::initialSeeds()");
  return createDefaultSeeds();
}

IFDSUninitializedVariablesIndexed::d_t
IFDSUninitializedVariablesIndexed::createZeroValue() const {
  PHASAR_LOG_LEVEL(DEBUG,
                   "IFDSUninitializedVariablesStructs::createZeroValue()");
  // create a special value to represent the zero value!
  return {LLVMZeroValue::getInstance()};
}

bool IFDSUninitializedVariablesIndexed::isZeroValue(
    IFDSUninitializedVariablesIndexed::d_t Fact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(Fact.getValue());
}

void IFDSUninitializedVariablesIndexed::emitTextReport(
    const SolverResults<IFDSUninitializedVariablesIndexed::n_t,
                        IFDSUninitializedVariablesIndexed::d_t, l_t>
        & /*Result*/,
    llvm::raw_ostream &OS) {
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
        for (const auto &UndefV : User.second) {
          OS << "   Uninit Value: " << DToString(UndefV.getValue());
          OS << "   Index: "
             << std::string(UndefV.getIndices().begin(),
                            UndefV.getIndices().end());
          OS << '\n';
        }
      }
      OS << '\n';
    } else {
      auto UninitResults = aggregateResults();
      OS << "\nTotal uses of uninitialized variables: " << UninitResults.size()
         << '\n';
      size_t Count = 0;
      for (auto Res : UninitResults) {
        OS << "\n---------------------------------  " << ++Count
           << ". Use  ---------------------------------\n\n";
        Res.print(OS);
      }
    }
  }
}

std::vector<IFDSUninitializedVariablesIndexed::UninitResult>
IFDSUninitializedVariablesIndexed::aggregateResults() {
  std::vector<IFDSUninitializedVariablesIndexed::UninitResult> Results;
  unsigned int LineNr = 0;

  unsigned int CurrLineNr = 0;
  UninitResult UR;
  for (const auto &User : UndefValueUses) {
    // new line nr idicates a new uninit use on source code level
    LineNr = getLineFromIR(User.first);
    if (CurrLineNr != LineNr) {
      CurrLineNr = LineNr;
      UninitResult NewUR;
      NewUR.Line = LineNr;
      NewUR.FuncName = getFunctionNameFromIR(User.first);
      NewUR.FilePath = getFilePathFromIR(User.first);
      NewUR.SrcCode = getSrcCodeFromIR(User.first);
      if (!UR.empty()) {
        Results.push_back(UR);
      }
      UR = NewUR;
    }
    // add current IR trace
    UR.IRTrace[User.first] = User.second;
    // add (possibly) new variable names
    for (const auto &UndefV : User.second) {
      auto VarName = getVarNameFromIR(UndefV.getValue());
      if (!VarName.empty()) {
        UR.VarNames.push_back(VarName);
      }
    }
  }
  if (!UR.empty()) {
    Results.push_back(UR);
  }
  return Results;
}

bool IFDSUninitializedVariablesIndexed::UninitResult::empty() const {
  return Line == 0;
}

void IFDSUninitializedVariablesIndexed::UninitResult::print(
    llvm::raw_ostream &OS) {
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
      for (const auto &IRVal : Trace.second) {
        OS << "   Uninit Value: " << llvmIRToString(IRVal.getValue()) << '\n';
      }
      // os << '\n';
    }
  }
}

const std::map<IFDSUninitializedVariablesIndexed::n_t,
               std::set<IFDSUninitializedVariablesIndexed::d_t>> &
IFDSUninitializedVariablesIndexed::getAllUndefUses() const {
  return UndefValueUses;
}

} // namespace psr