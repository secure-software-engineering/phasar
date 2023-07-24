/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSConstAnalysis.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMCXXShorthands.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"

#include <memory>
#include <utility>

namespace psr {

IFDSConstAnalysis::IFDSConstAnalysis(const LLVMProjectIRDB *IRDB,
                                     LLVMAliasInfoRef PT,
                                     std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()),
      PT(PT) {
  assert(PT);
  PAMM_GET_INSTANCE;
  REG_HISTOGRAM("Context-relevant Pointer", PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("[Calls] getContextRelevantAliasSet", 0,
              PAMM_SEVERITY_LEVEL::Full);
}

IFDSConstAnalysis::FlowFunctionPtrType
IFDSConstAnalysis::getNormalFlowFunction(IFDSConstAnalysis::n_t Curr,
                                         IFDSConstAnalysis::n_t /*Succ*/) {
  // Check all store instructions.
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    // If the store instruction sets up or updates the vtable, i.e. value
    // operand is vtable pointer, ignore it!
    if (isTouchVTableInst(Store)) {
      return Identity<IFDSConstAnalysis::d_t>::getInstance();
    }

    IFDSConstAnalysis::d_t PointerOp = Store->getPointerOperand();
    PHASAR_LOG_LEVEL(DEBUG, "Pointer operand of store Instruction: "
                                << llvmIRToString(PointerOp));
    auto PTS = PT.getAliasSet(PointerOp);
    std::set<IFDSConstAnalysis::d_t> AliasSet(PTS->begin(), PTS->end());
    // Check if this store instruction is the second write access to the
    // memory location the pointer operand or it's alias are pointing to.
    // This is done by checking the Initialized set.
    // If so, generate the pointer operand as a new data-flow fact. Also
    // generate data-flow facts of all alias that meet the
    // 'context-relevant' requirements! (see getContextRelevantPointsToSet
    // function) NOTE: The points-to set of value x also contains the value
    // x itself!
    for (const auto *Alias : AliasSet) {
      if (isInitialized(Alias)) {
        PHASAR_LOG_LEVEL(DEBUG, "Compute context-relevant points-to "
                                "information for the pointer operand.");
        return generateManyFlows(
            getContextRelevantAliasSet(AliasSet, Curr->getFunction()),
            getZeroValue());
      }
    }
    // If neither the pointer operand nor one of its alias is initialized,
    // we mark only the pointer operand (to keep the Initialized set as
    // small as possible) as initialized by adding it to the Initialized
    // set. We do not generate any new data-flow facts at this point.
    markAsInitialized(PointerOp);
    PHASAR_LOG_LEVEL(DEBUG, "Pointer operand marked as initialized!");
  } /* end store instruction */

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

IFDSConstAnalysis::FlowFunctionPtrType
IFDSConstAnalysis::getCallFlowFunction(IFDSConstAnalysis::n_t CallSite,
                                       IFDSConstAnalysis::f_t DestFun) {
  // Handle one of the three llvm memory intrinsics (memcpy, memmove or
  // memset)
  if (llvm::isa<llvm::MemIntrinsic>(CallSite)) {
    PHASAR_LOG_LEVEL(DEBUG, "Call statement is a LLVM MemIntrinsic!");
    return killAllFlows<d_t>();
  }
  // Check if its a Call Instruction or an Invoke Instruction. If so, we
  // need to map all actual parameters into formal parameters.
  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    // return KillAll<IFDSConstAnalysis::d_t>::getInstance();
    PHASAR_LOG_LEVEL(DEBUG, "Call statement: " << llvmIRToString(CallSite));
    PHASAR_LOG_LEVEL(DEBUG, "Destination method: " << DestFun->getName());
    return mapFactsToCallee(Call, DestFun, [](d_t Actual, d_t Source) {
      return Actual == Source && Actual->getType()->isPointerTy();
    });
  } /* end call/invoke instruction */

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

IFDSConstAnalysis::FlowFunctionPtrType IFDSConstAnalysis::getRetFlowFunction(
    IFDSConstAnalysis::n_t CallSite, IFDSConstAnalysis::f_t /*CalleeFun*/,
    IFDSConstAnalysis::n_t ExitStmt, IFDSConstAnalysis::n_t /*RetSite*/) {
  // return KillAll<IFDSConstAnalysis::d_t>::getInstance();
  // Map formal parameter back to the actual parameter in the caller.

  return mapFactsToCaller(llvm::cast<llvm::CallBase>(CallSite), ExitStmt,
                          [](d_t Param, d_t Source) {
                            return Param == Source &&
                                   Param->getType()->isPointerTy();
                          });
  // All other data-flow facts of the callee function are killed at this point
}

IFDSConstAnalysis::FlowFunctionPtrType
IFDSConstAnalysis::getCallToRetFlowFunction(IFDSConstAnalysis::n_t CallSite,
                                            IFDSConstAnalysis::n_t /*RetSite*/,
                                            llvm::ArrayRef<f_t> /*Callees*/) {
  // Process the effects of a llvm memory intrinsic function.
  if (llvm::isa<llvm::MemIntrinsic>(CallSite)) {
    IFDSConstAnalysis::d_t PointerOp = CallSite->getOperand(0);
    PHASAR_LOG_LEVEL(DEBUG, "Pointer Operand: " << llvmIRToString(PointerOp));
    auto PTS = PT.getAliasSet(PointerOp);
    std::set<IFDSConstAnalysis::d_t> AliasSet(PTS->begin(), PTS->end());
    for (const auto *Alias : AliasSet) {
      if (isInitialized(Alias)) {
        PHASAR_LOG_LEVEL(DEBUG, "Compute context-relevant points-to "
                                "information of the pointer operand.");
        return generateManyFlows(
            getContextRelevantAliasSet(AliasSet, CallSite->getFunction()),
            getZeroValue());
      }
    }
    markAsInitialized(PointerOp);
    PHASAR_LOG_LEVEL(DEBUG, "Pointer operand marked as initialized!");
  }

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

IFDSConstAnalysis::FlowFunctionPtrType
IFDSConstAnalysis::getSummaryFlowFunction(IFDSConstAnalysis::n_t /*CallSite*/,
                                          IFDSConstAnalysis::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::l_t>
IFDSConstAnalysis::initialSeeds() {
  // just start in main()
  return createDefaultSeeds();
}

IFDSConstAnalysis::d_t IFDSConstAnalysis::createZeroValue() const {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSConstAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSConstAnalysis::isZeroValue(IFDSConstAnalysis::d_t Fact) const {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

void IFDSConstAnalysis::printNode(llvm::raw_ostream &OS,
                                  IFDSConstAnalysis::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IFDSConstAnalysis::printDataFlowFact(llvm::raw_ostream &OS,
                                          IFDSConstAnalysis::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IFDSConstAnalysis::printFunction(llvm::raw_ostream &OS,
                                      IFDSConstAnalysis::f_t Func) const {
  OS << Func->getName();
}

void IFDSConstAnalysis::printInitMemoryLocations() {
  PHASAR_LOG_LEVEL(
      DEBUG, "Printing all initialized memory location (or one of its alias)");
#ifdef DYNAMIC_LOG
  for (const auto *Stmt : IFDSConstAnalysis::Initialized) {
    PHASAR_LOG_LEVEL(DEBUG, llvmIRToString(Stmt));
  }
#endif
}

std::set<IFDSConstAnalysis::d_t> IFDSConstAnalysis::getContextRelevantAliasSet(
    std::set<IFDSConstAnalysis::d_t> &AliasSet,
    IFDSConstAnalysis::f_t CurrentContext) {
  PAMM_GET_INSTANCE;
  INC_COUNTER("[Calls] getContextRelevantAliasSet", 1,
              PAMM_SEVERITY_LEVEL::Full);
  START_TIMER("Context-Relevant-Alias-Set Computation",
              PAMM_SEVERITY_LEVEL::Full);
  std::set<IFDSConstAnalysis::d_t> ToGenerate;
  for (const auto *Alias : AliasSet) {
    PHASAR_LOG_LEVEL(DEBUG, "Alias: " << llvmIRToString(Alias));
    // Case (i + ii)
    if (const auto *I = llvm::dyn_cast<llvm::Instruction>(Alias)) {
      if (isAllocaInstOrHeapAllocaFunction(Alias)) {
        ToGenerate.insert(Alias);
        PHASAR_LOG_LEVEL(DEBUG, "alloca inst will be generated as a new fact!");
      } else if (I->getFunction() == CurrentContext) {
        ToGenerate.insert(Alias);
        PHASAR_LOG_LEVEL(DEBUG, "instruction within current function will "
                                "be generated as a new fact!");
      }
    } // Case (ii)
    else if (llvm::isa<llvm::GlobalValue>(Alias)) {
      ToGenerate.insert(Alias);
      PHASAR_LOG_LEVEL(DEBUG,
                       "global variable will be generated as a new fact!");
    } // Case (iii)
    else if (const auto *A = llvm::dyn_cast<llvm::Argument>(Alias)) {
      if (A->getParent() == CurrentContext) {
        ToGenerate.insert(Alias);
        PHASAR_LOG_LEVEL(DEBUG,
                         "formal argument will be generated as a new fact!");
      }
    } // ignore everything else
  }
  PAUSE_TIMER("Context-Relevant-Alias-Set Computation",
              PAMM_SEVERITY_LEVEL::Full);
  ADD_TO_HISTOGRAM("Context-relevant Pointer", ToGenerate.size(), 1,
                   PAMM_SEVERITY_LEVEL::Full);
  return ToGenerate;
}

bool IFDSConstAnalysis::isInitialized(IFDSConstAnalysis::d_t Fact) const {
  return llvm::isa<llvm::GlobalValue>(Fact) || Initialized.count(Fact);
}

void IFDSConstAnalysis::markAsInitialized(IFDSConstAnalysis::d_t Fact) {
  Initialized.insert(Fact);
}

size_t IFDSConstAnalysis::initMemoryLocationCount() {
  return Initialized.size();
}

void IFDSConstAnalysis::emitTextReport(
    const SolverResults<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
                        BinaryDomain> &SR,
    llvm::raw_ostream &OS) {

  LLVMBasedCFG CFG;
  // 1) Remove all mutable memory locations
  for (const auto *F : IRDB->getAllFunctions()) {
    for (const auto *Exit : CFG.getExitPointsOf(F)) {
      std::set<const llvm::Value *> Facts = SR.ifdsResultsAt(Exit);
      // Empty facts means the exit statement is part of a not
      // analyzed function, thus remove all memory locations of that
      // function
      if (Facts.empty()) {
        for (auto MemItr = AllMemLocs.begin(); MemItr != AllMemLocs.end();) {
          if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(*MemItr)) {
            if (Inst->getParent()->getParent() == F) {
              MemItr = AllMemLocs.erase(MemItr);
            } else {
              ++MemItr;
            }
          } else {
            ++MemItr;
          }
        }
      } else {
        for (const auto *Fact : Facts) {
          if (isAllocaInstOrHeapAllocaFunction(Fact) ||
              llvm::isa<llvm::GlobalValue>(Fact)) {
            // remove memory locations that are mutable, i.e. are valid
            // facts
            AllMemLocs.erase(Fact);
          }
        }
      }
    }
  }
  // 2) Print all immutbale/const memory locations
  OS << "=========== IFDS Const Analysis Results ===========\n";
  if (AllMemLocs.empty()) {
    OS << "No immutable memory locations found!\n";
  } else {
    OS << "Immutable/const stack and/or heap memory locations:\n";
    for (const auto *Memloc : AllMemLocs) {
      OS << "\nIR  : " << llvmIRToString(Memloc) << '\n';
    }
  }
  OS << "\n===================================================\n";
}

} // namespace psr
