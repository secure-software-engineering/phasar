/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/IntrinsicInst.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenAll.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSConstAnalysis.h>

#include <phasar/Utils/PAMM.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;
namespace psr {

IFDSConstAnalysis::IFDSConstAnalysis(IFDSConstAnalysis::i_t icfg,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), ptg(icfg.getWholeModulePTG()),
      EntryPoints(EntryPoints) {
  PAMM_FACTORY;
  REG_HISTOGRAM("Context-relevant-PT");
  REG_COUNTER("Calls to getContextRelevantPointsToSet");
  IFDSConstAnalysis::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getNormalFlowFunction(IFDSConstAnalysis::n_t curr,
                                         IFDSConstAnalysis::n_t succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getNormalFlowFunction()";
  // Check all store instructions.
  if (const llvm::StoreInst *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    // If the store instruction sets up or updates the vtable, i.e. value
    // operand is vtable pointer, ignore it!
    // Setting up the vtable is counted towards the initialization of an
    // object - the object stays immutable.
    // To identifiy such a store instruction, we need to check the stored
    // value, which is of i32 (...)** type, e.g.
    //   i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]*
    //                           @_ZTV5Child, i32 0, i32 2) to i32 (...)**)
    //
    if (const llvm::ConstantExpr *CE =
            llvm::dyn_cast<llvm::ConstantExpr>(Store->getValueOperand())) {
      // llvm::ConstantExpr *CE = const_cast<llvm::ConstantExpr *>(ConstCE);
      if (llvm::ConstantExpr *CF = llvm::dyn_cast<llvm::ConstantExpr>(
              const_cast<llvm::ConstantExpr *>(CE)
                  ->getAsInstruction()
                  ->getOperand(0))) {
        if (auto VTable = llvm::dyn_cast<llvm::GlobalVariable>(
                CF->getAsInstruction()->getOperand(0))) {
          if (VTable->hasName() &&
              cxx_demangle(VTable->getName().str()).find("vtable") !=
                  string::npos) {
            BOOST_LOG_SEV(lg, DEBUG)
                << "Store Instruction sets up or updates vtable - ignored!";
            return Identity<IFDSConstAnalysis::d_t>::getInstance();
          }
        }
      }
    } /* end vtable set-up instruction */
    IFDSConstAnalysis::d_t pointerOp = Store->getPointerOperand();
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand of store Instruction: "
                             << llvmIRToString(pointerOp);
    set<IFDSConstAnalysis::d_t> pointsToSet = ptg.getPointsToSet(pointerOp);
    // Check if this store instruction is the second write access to the memory
    // location the pointer operand or it's alias are pointing to.
    // This is done by checking the Initialized set.
    // If so, generate the pointer operand as a new data-flow fact. Also
    // generate data-flow facts of all alias that meet the 'context-relevant'
    // requirements! (see getContextRelevantPointsToSet function)
    // NOTE: The points-to set of value x also contains the value x itself!
    for (auto alias : pointsToSet) {
      if (isInitialized(alias)) {
        BOOST_LOG_SEV(lg, DEBUG) << "Compute context-relevant points-to "
                                    "information for the pointer operand.";
        return make_shared<
            GenAll<IFDSConstAnalysis::d_t>>(/*pointsToSet*/
                                            getContextRelevantPointsToSet(
                                                pointsToSet,
                                                curr->getFunction()),
                                            zeroValue());
      }
    }
    // If neither the pointer operand nor one of its alias is initialized,
    // we mark only the pointer operand (to keep the Initialized set as
    // small as possible) as initialized by adding it to the Initialized set.
    // We do not generate any new data-flow facts at this point.
    markAsInitialized(pointerOp);
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand marked as initialized!";
  } /* end store instruction */

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getCallFlowFunction(IFDSConstAnalysis::n_t callStmt,
                                       IFDSConstAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallFlowFunction()";
  // Handle one of the three llvm memory intrinsics (memcpy, memmove or memset)
  if (llvm::isa<llvm::MemIntrinsic>(callStmt)) {
    BOOST_LOG_SEV(lg, DEBUG) << "Call statement is a LLVM MemIntrinsic!";
    return KillAll<IFDSConstAnalysis::d_t>::getInstance();
  }
  // Check if its a Call Instruction or an Invoke Instruction. If so, we
  // need to map all actual parameters into formal parameters.
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    // return KillAll<IFDSConstAnalysis::d_t>::getInstance();
    BOOST_LOG_SEV(lg, DEBUG) << "Call statement: " << llvmIRToString(callStmt);
    BOOST_LOG_SEV(lg, DEBUG)
        << "Destination method: " << destMthd->getName().str();
    return make_shared<MapFactsToCallee>(
        llvm::ImmutableCallSite(callStmt), destMthd,
        [](IFDSConstAnalysis::d_t actual) {
          return actual->getType()->isPointerTy();
        });
  } /* end call/invoke instruction */

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getRetFlowFunction(IFDSConstAnalysis::n_t callSite,
                                      IFDSConstAnalysis::m_t calleeMthd,
                                      IFDSConstAnalysis::n_t exitStmt,
                                      IFDSConstAnalysis::n_t retSite) {
  // return KillAll<IFDSConstAnalysis::d_t>::getInstance();
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getRetFlowFunction()";
  BOOST_LOG_SEV(lg, DEBUG) << "Call site: " << llvmIRToString(callSite);
  BOOST_LOG_SEV(lg, DEBUG) << "Caller context: "
                           << callSite->getFunction()->getName().str();
  BOOST_LOG_SEV(lg, DEBUG) << "Retrun site: " << llvmIRToString(retSite);
  BOOST_LOG_SEV(lg, DEBUG) << "Callee method: " << calleeMthd->getName().str();
  BOOST_LOG_SEV(lg, DEBUG) << "Callee exit statement: "
                           << llvmIRToString(exitStmt);
  // Map formal parameter back to the actual parameter in the caller.
  return make_shared<MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeMthd, exitStmt,
      [](IFDSConstAnalysis::d_t formal) {
        return formal->getType()->isPointerTy();
      },
      [](IFDSConstAnalysis::m_t cmthd) {
        return cmthd->getReturnType()->isPointerTy();
      });
  // All other data-flow facts of the callee function are killed at this point
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getCallToRetFlowFunction(
    IFDSConstAnalysis::n_t callSite, IFDSConstAnalysis::n_t retSite,
    set<IFDSConstAnalysis::m_t> callees) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallToRetFlowFunction()";
  BOOST_LOG_SEV(lg, DEBUG) << "Call site: " << llvmIRToString(callSite);
  BOOST_LOG_SEV(lg, DEBUG) << "Return site: " << llvmIRToString(retSite);
  // Process the effects of a llvm memory intrinsic function.
  if (llvm::isa<llvm::MemIntrinsic>(callSite)) {
    IFDSConstAnalysis::d_t pointerOp = callSite->getOperand(0);
    BOOST_LOG_SEV(lg, DEBUG)
        << "Pointer Operand: " << llvmIRToString(pointerOp);
    set<IFDSConstAnalysis::d_t> pointsToSet = ptg.getPointsToSet(pointerOp);
    for (auto alias : pointsToSet) {
      if (isInitialized(alias)) {
        BOOST_LOG_SEV(lg, DEBUG) << "Compute context-relevant points-to "
                                    "information of the pointer operand.";
        return make_shared<
            GenAll<IFDSConstAnalysis::d_t>>(/*pointsToSet*/
                                            getContextRelevantPointsToSet(
                                                pointsToSet,
                                                callSite->getFunction()),
                                            zeroValue());
      }
    }
    markAsInitialized(pointerOp);
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand marked as initialized!";
  }

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getSummaryFlowFunction(IFDSConstAnalysis::n_t callStmt,
                                          IFDSConstAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getSummaryFlowFunction()";

  return nullptr;
}

map<IFDSConstAnalysis::n_t, set<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::initialSeeds() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::initialSeeds()";
  // just start in main()
  map<IFDSConstAnalysis::n_t, set<IFDSConstAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                             set<IFDSConstAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSConstAnalysis::d_t IFDSConstAnalysis::createZeroValue() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::createZeroValue()";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSConstAnalysis::isZeroValue(IFDSConstAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

string IFDSConstAnalysis::DtoString(IFDSConstAnalysis::d_t d) const {
  return llvmIRToString(d);
}

string IFDSConstAnalysis::NtoString(IFDSConstAnalysis::n_t n) const {
  return llvmIRToString(n);
}

string IFDSConstAnalysis::MtoString(IFDSConstAnalysis::m_t m) const {
  return m->getName().str();
}

void IFDSConstAnalysis::printInitMemoryLocations() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "Printing all initialized memory location (or one of its alias)";
  for (auto stmt : IFDSConstAnalysis::Initialized) {
    BOOST_LOG_SEV(lg, DEBUG) << llvmIRToString(stmt);
  }
}

set<IFDSConstAnalysis::d_t> IFDSConstAnalysis::getContextRelevantPointsToSet(
    set<IFDSConstAnalysis::d_t> &PointsToSet,
    IFDSConstAnalysis::m_t CurrentContext) {
  PAMM_FACTORY;
  INC_COUNTER("Calls to getContextRelevantPointsToSet");
  START_TIMER("Compute ContextRelevantPointsToSet");
  auto &lg = lg::get();
  set<IFDSConstAnalysis::d_t> ToGenerate;
  for (auto alias : PointsToSet) {
    BOOST_LOG_SEV(lg, DEBUG) << "Alias: " << llvmIRToString(alias);
    // Case (i + ii)
    if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(alias)) {
      if (isAllocaInstOrHeapAllocaFunction(alias)) {
        ToGenerate.insert(alias);
        BOOST_LOG_SEV(lg, DEBUG)
            << "alloca inst will be generated as a new fact!";
      } else if (I->getFunction() == CurrentContext) {
        ToGenerate.insert(alias);
        BOOST_LOG_SEV(lg, DEBUG) << "instruction within current function will "
                                    "be generated as a new fact!";
      }
    } // Case (ii)
    else if (llvm::isa<llvm::GlobalValue>(alias)) {
      ToGenerate.insert(alias);
      BOOST_LOG_SEV(lg, DEBUG)
          << "global variable will be generated as a new fact!";
    } // Case (iii)
    else if (const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(alias)) {
      if (A->getParent() == CurrentContext) {
        ToGenerate.insert(alias);
        BOOST_LOG_SEV(lg, DEBUG)
            << "formal argument will be generated as a new fact!";
      }
    } // ignore everything else
  }
  PAUSE_TIMER("Compute ContextRelevantPointsToSet");
  ADD_TO_HIST("Context-relevant-PT", ToGenerate.size());
  return ToGenerate;
}

bool IFDSConstAnalysis::isInitialized(IFDSConstAnalysis::d_t d) const {
  return llvm::isa<llvm::GlobalValue>(d) || Initialized.count(d);
}

void IFDSConstAnalysis::markAsInitialized(IFDSConstAnalysis::d_t d) {
  Initialized.insert(d);
}

size_t IFDSConstAnalysis::initMemoryLocationCount() {
  return Initialized.size();
}

} // namespace psr
