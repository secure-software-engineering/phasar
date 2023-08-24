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
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"

#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include <utility>

namespace psr {

IFDSUninitializedVariables::IFDSUninitializedVariables(
    const LLVMProjectIRDB *IRDB, std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getNormalFlowFunction(
    IFDSUninitializedVariables::n_t Curr,
    IFDSUninitializedVariables::n_t /*Succ*/) {
  //----------------------------------------------------------------------------------
  // Why do we need this case?
  // Every alloca is reached eventually by this function
  //----------------------------------------------------------------------------------

  // initially mark every local as uninitialized (except entry point args)
  /* if (icfg.isStartPoint(curr) &&
            curr->getFunction()->getName() == "main") {
    const llvm::Function *func = icfg.getMethodOf(curr);
    // set all locals as uninitialized flow function
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
      const llvm::Function *func;
      IFDSUninitializedVariables::d_t zerovalue;
      UVFF(const llvm::Function *f, IFDSUninitializedVariables::d_t zv)
          : func(f), zerovalue(zv) {}
      set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t source) override {
        if (source == zerovalue) {
          set<IFDSUninitializedVariables::d_t> res;

          // first add all local values of primitive types
          for (auto &BB : *func) {
            for (auto &inst : BB) {
              // collect all alloca instructions of primitive types
              if (auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
                if (alloc->getAllocatedType()->isIntegerTy() ||
                    alloc->getAllocatedType()->isFloatingPointTy() ||
                    alloc->getAllocatedType()->isPointerTy() ||
                    alloc->getAllocatedType()->isArrayTy()) {
                  res.insert(alloc);
                }
              } else {
                // collect all instructions that use an undef literal
                for (auto &operand : inst.operands()) {
                  if (llvm::isa<llvm::UndefValue>(&operand)) {
                    res.insert(&inst);
                  }
                }
              }
            }
          }
          // remove function parameters of entry function
          for (auto &arg : func->args()) {
            res.erase(&arg);
          }
          res.insert(zerovalue);
          return res;
        }
        return {};
      }
    };
    return make_shared<UVFF>(func, zerovalue);
  }
  */

  // check the all store instructions and kill initialized variables
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
      // const llvm::Value *valueop;
      // const llvm::Value *pointerop;
      const llvm::StoreInst *Store;
      const llvm::Value *Zero;
      std::map<IFDSUninitializedVariables::n_t,
               std::set<IFDSUninitializedVariables::d_t>> &UndefValueUses;
      UVFF(const llvm::StoreInst *S,
           std::map<IFDSUninitializedVariables::n_t,
                    std::set<IFDSUninitializedVariables::d_t>> &UVU,
           const llvm::Value *Zero)
          : Store(S), Zero(Zero), UndefValueUses(UVU) {}
      std::set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t Source) override {

        //----------------------------------------------------------------------
        // I don't get the purpose of this for-loop;
        // When an undefined load is stored, it should eventually be source
        //----------------------------------------------------------------------

        /*
         // check if an uninitialized value is loaded and stored in a variable
         for (auto &use : store->getValueOperand()->uses()) {
           // check if use is load
           if (const llvm::LoadInst *load =
                   llvm::dyn_cast<llvm::LoadInst>(use)) {
             // if the following is uninit, then this store must be uninit
             if (source == load->getPointerOperand() || source == load) {
               UndefValueUses[load].insert(load->getPointerOperand());
               return {source, load, store->getValueOperand(),
                       store->getPointerOperand()};
             }
           }
           if (const llvm::Instruction *inst =
                   llvm::dyn_cast<llvm::Instruction>(use)) {
             for (auto &operand : inst->operands()) {
               if (const llvm::Value *val =
                       llvm::dyn_cast<llvm::Value>(&operand)) {
                 if (val == source || llvm::isa<llvm::UndefValue>(val)) {
                   return {source, val, store->getPointerOperand()};
                 }
               }
             }
           }
           if (use.get() == source) {
             return {source, store->getValueOperand(),
                     store->getPointerOperand()};
           }
         }
         // otherwise initialize (kill) the value
         if (store->getPointerOperand() == source) {
           return {};
         }
         */
        if (Source == Store->getValueOperand() ||
            (Source == Zero &&
             llvm::isa<llvm::UndefValue>(Store->getValueOperand()))) {
          return {Source, Store->getPointerOperand()};
        }
        if (Source ==
            Store->getPointerOperand()) { // storing an initialized value
                                          // kills the variable as it is
                                          // now initialized too
          return {};
        }
        // pass all other facts as identity
        return {Source};
      }
    };
    return std::make_shared<UVFF>(Store, UndefValueUses, getZeroValue());
  }
  if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {

    return lambdaFlow([Alloc, this](d_t Source) -> std::set<d_t> {
      if (isZeroValue(Source)) {
        if (Alloc->getAllocatedType()->isIntegerTy() ||
            Alloc->getAllocatedType()->isFloatingPointTy() ||
            Alloc->getAllocatedType()->isPointerTy() ||
            Alloc->getAllocatedType()->isArrayTy()) {
          //------------------------------------------------------------
          // Why not generate for structs, but for arrays? (would be
          // consistent to generate either both or none of them)
          //------------------------------------------------------------

          // generate the alloca
          return {Source, Alloc};
        }
      }
      // otherwise propagate all facts
      return {Source};
    });
  }
  // check if some instruction is using an undefined value (in)directly
  struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
    const llvm::Instruction *Inst;
    std::map<IFDSUninitializedVariables::n_t,
             std::set<IFDSUninitializedVariables::d_t>> &UndefValueUses;
    UVFF(const llvm::Instruction *Inst,
         std::map<IFDSUninitializedVariables::n_t,
                  std::set<IFDSUninitializedVariables::d_t>> &UVU)
        : Inst(Inst), UndefValueUses(UVU) {}
    std::set<IFDSUninitializedVariables::d_t>
    computeTargets(IFDSUninitializedVariables::d_t Source) override {
      for (const auto &Operand : Inst->operands()) {
        const llvm::UndefValue *Undef =
            llvm::dyn_cast<llvm::UndefValue>(Operand);
        if (Operand == Source || Operand == Undef) {
          //----------------------------------------------------------------
          // It is not necessary and (from my point of view) not intended to
          // report a leak on EVERY kind of instruction.
          // For some of them (e.g. gep, bitcast, ...) propagating the dataflow
          // facts may be enough
          //----------------------------------------------------------------
          if (!llvm::isa<llvm::GetElementPtrInst>(Inst) &&
              !llvm::isa<llvm::CastInst>(Inst) &&
              !llvm::isa<llvm::PHINode>(Inst)) {
            UndefValueUses[Inst].insert(Operand);
          }
          return {Source, Inst};
        }
      }
      return {Source};
    }
  };
  return std::make_shared<UVFF>(Curr, UndefValueUses);
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getCallFlowFunction(
    IFDSUninitializedVariables::n_t CallSite,
    IFDSUninitializedVariables::f_t DestFun) {
  if (llvm::isa<llvm::CallInst>(CallSite) ||
      llvm::isa<llvm::InvokeInst>(CallSite)) {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
      const llvm::Function *DestFun;
      const llvm::CallBase *CallSite;
      const llvm::Value *Zerovalue;
      std::vector<const llvm::Value *> Actuals;
      std::vector<const llvm::Value *> Formals;
      UVFF(const llvm::Function *DM, const llvm::CallBase *CallSite,
           const llvm::Value *ZV)
          : DestFun(DM), CallSite(CallSite), Zerovalue(ZV) {
        // set up the actual parameters
        for (unsigned Idx = 0; Idx < CallSite->arg_size(); ++Idx) {
          Actuals.push_back(CallSite->getArgOperand(Idx));
        }
        // set up the formal parameters
        /*for (unsigned idx = 0; idx < destFun->arg_size(); ++idx) {
          formals.push_back(getNthFunctionArgument(destFun, idx));
        }*/
        for (const auto &Arg : DestFun->args()) {
          Formals.push_back(&Arg);
        }
      }

      std::set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t Source) override {
        // perform parameter passing
        if (Source != Zerovalue) {
          std::set<const llvm::Value *> Res;
          // do the mapping from actual to formal parameters
          // caution: the loop iterates from 0 to formals.size(),
          // rather than actuals.size() as we may have more actual
          // than formal arguments in case of C-style varargs
          for (unsigned Idx = 0; Idx < Formals.size(); ++Idx) {
            if (Source == Actuals[Idx]) {
              Res.insert(Formals[Idx]);
            }
          }
          return Res;
        }

        //--------------------------------------------------------------
        // Why not letting the normal FF generate the allocas?
        //--------------------------------------------------------------

        // on zerovalue -> gen all locals parameter
        /* set<const llvm::Value *> res;
        for (auto &BB : *destFun) {
          for (auto &inst : BB) {
            if (auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
              // check if the allocated value is of a primitive type
              if (alloc->getAllocatedType()->isIntegerTy() ||
                  alloc->getAllocatedType()->isFloatingPointTy() ||
                  alloc->getAllocatedType()->isPointerTy() ||
                  alloc->getAllocatedType()->isArrayTy()) {
                res.insert(alloc);
              }
            } else {
              // check for instructions using undef value directly
              for (auto &operand : inst.operands()) {
                if (llvm::isa<llvm::UndefValue>(&operand)) {
                  res.insert(&inst);
                }
              }
            }
          }
        }
        return res;*/

        // propagate zero
        return {Source};
      }
    };
    return std::make_shared<UVFF>(DestFun, CS, getZeroValue());
  }
  return identityFlow();
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getRetFlowFunction(
    IFDSUninitializedVariables::n_t CallSite,
    IFDSUninitializedVariables::f_t /*CalleeFun*/,
    IFDSUninitializedVariables::n_t ExitStmt,
    IFDSUninitializedVariables::n_t /*RetSite*/) {
  if (llvm::isa<llvm::CallInst>(CallSite) ||
      llvm::isa<llvm::InvokeInst>(CallSite)) {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
      const llvm::CallBase *Call;
      const llvm::Instruction *Exit;
      UVFF(const llvm::CallBase *C, const llvm::Instruction *E)
          : Call(C), Exit(E) {}
      std::set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t Source) override {
        // check if we return an uninitialized value
        std::set<IFDSUninitializedVariables::d_t> Ret;
        if (Exit->getNumOperands() > 0 && Exit->getOperand(0) == Source) {
          Ret.insert(Call);
        }
        //----------------------------------------------------------------------
        // Handle pointer/reference parameters
        //----------------------------------------------------------------------
        if (Call->getCalledFunction()) {
          unsigned I = 0;
          for (const auto &Arg : Call->getCalledFunction()->args()) {
            // auto arg = getNthFunctionArgument(call.getCalledFunction(), i);
            if (&Arg == Source && Arg.getType()->isPointerTy()) {
              Ret.insert(Call->getArgOperand(I));
            }
            I++;
          }
        }

        // kill all other facts
        return Ret;
      }
    };
    return std::make_shared<UVFF>(CS, ExitStmt);
  }
  // kill everything else
  return killAllFlows();
}

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getCallToRetFlowFunction(
    IFDSUninitializedVariables::n_t CallSite,
    IFDSUninitializedVariables::n_t /*RetSite*/,
    llvm::ArrayRef<f_t> /*Callees*/) {
  //----------------------------------------------------------------------
  // Handle pointer/reference parameters
  //----------------------------------------------------------------------
  if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    return lambdaFlow([CS](d_t Source) -> std::set<d_t> {
      if (Source->getType()->isPointerTy()) {
        for (const auto &Arg : CS->args()) {
          if (Arg.get() == Source) {
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

IFDSUninitializedVariables::FlowFunctionPtrType
IFDSUninitializedVariables::getSummaryFlowFunction(
    IFDSUninitializedVariables::n_t /*CallSite*/,
    IFDSUninitializedVariables::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSUninitializedVariables::n_t, IFDSUninitializedVariables::d_t,
             IFDSUninitializedVariables::l_t>
IFDSUninitializedVariables::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSUninitializedVariables::initialSeeds()");
  return createDefaultSeeds();
}

IFDSUninitializedVariables::d_t
IFDSUninitializedVariables::createZeroValue() const {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSUninitializedVariables::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSUninitializedVariables::isZeroValue(
    IFDSUninitializedVariables::d_t Fact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

void IFDSUninitializedVariables::emitTextReport(
    const SolverResults<IFDSUninitializedVariables::n_t,
                        IFDSUninitializedVariables::d_t, l_t> & /*Result*/,
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
      size_t Count = 0;
      for (auto Res : UninitResults) {
        OS << "\n---------------------------------  " << ++Count
           << ". Use  ---------------------------------\n\n";
        Res.print(OS);
      }
    }
  }
}

std::vector<IFDSUninitializedVariables::UninitResult>
IFDSUninitializedVariables::aggregateResults() {
  std::vector<IFDSUninitializedVariables::UninitResult> Results;
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
    for (const auto *UndefV : User.second) {
      auto VarName = getVarNameFromIR(UndefV);
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

bool IFDSUninitializedVariables::UninitResult::empty() const {
  return Line == 0;
}

void IFDSUninitializedVariables::UninitResult::print(llvm::raw_ostream &OS) {
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

const std::map<IFDSUninitializedVariables::n_t,
               std::set<IFDSUninitializedVariables::d_t>> &
IFDSUninitializedVariables::getAllUndefUses() const {
  return UndefValueUses;
}

} // namespace psr
