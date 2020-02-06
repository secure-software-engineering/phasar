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
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenAll.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

#include <phasar/Utils/LLVMIRToSrc.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/PAMMMacros.h>
#include <phasar/Utils/Utilities.h>

using namespace std;
using namespace psr;
namespace psr {

IFDSConstAnalysis::IFDSConstAnalysis(const ProjectIRDB *IRDB,
                                     const LLVMTypeHierarchy *TH,
                                     const LLVMBasedICFG *ICF,
                                     const LLVMPointsToInfo *PT,
                                     std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, EntryPoints),
      ptg(ICF->getWholeModulePTG()), AllMemLocs() {
  PAMM_GET_INSTANCE;
  REG_HISTOGRAM("Context-relevant Pointer", PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("[Calls] getContextRelevantPointsToSet", 0,
              PAMM_SEVERITY_LEVEL::Full);
  IFDSTabulationProblem::ZeroValue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getNormalFlowFunction(IFDSConstAnalysis::n_t curr,
                                         IFDSConstAnalysis::n_t succ) {
  auto &lg = lg::get();
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
    // WARNING: The VTT could also be stored, which would make this analysis
    // fail
    if (const llvm::ConstantExpr *CE =
            llvm::dyn_cast<llvm::ConstantExpr>(Store->getValueOperand())) {
      // llvm::ConstantExpr *CE = const_cast<llvm::ConstantExpr *>(ConstCE);
      auto CE_inst = const_cast<llvm::ConstantExpr *>(CE)->getAsInstruction();
      if (llvm::ConstantExpr *CF =
              llvm::dyn_cast<llvm::ConstantExpr>(CE_inst->getOperand(0))) {
        auto CF_inst = CF->getAsInstruction();
        if (auto VTable =
                llvm::dyn_cast<llvm::GlobalVariable>(CF_inst->getOperand(0))) {
          if (VTable->hasName() &&
              cxx_demangle(VTable->getName().str()).find("vtable") !=
                  string::npos) {
            LOG_IF_ENABLE(
                BOOST_LOG_SEV(lg, DEBUG)
                << "Store Instruction sets up or updates vtable - ignored!");
            return Identity<IFDSConstAnalysis::d_t>::getInstance();
          }
        }
        CF_inst->deleteValue();
      }
      CE_inst->deleteValue();
    } /* end vtable set-up instruction */
    IFDSConstAnalysis::d_t pointerOp = Store->getPointerOperand();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Pointer operand of store Instruction: "
                  << llvmIRToString(pointerOp));
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
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Compute context-relevant points-to "
                         "information for the pointer operand.");
        return make_shared<
            GenAll<IFDSConstAnalysis::d_t>>(/*pointsToSet*/
                                            getContextRelevantPointsToSet(
                                                pointsToSet,
                                                curr->getFunction()),
                                            getZeroValue());
      }
    }
    // If neither the pointer operand nor one of its alias is initialized,
    // we mark only the pointer operand (to keep the Initialized set as
    // small as possible) as initialized by adding it to the Initialized set.
    // We do not generate any new data-flow facts at this point.
    markAsInitialized(pointerOp);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Pointer operand marked as initialized!");
  } /* end store instruction */

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getCallFlowFunction(IFDSConstAnalysis::n_t callStmt,
                                       IFDSConstAnalysis::f_t destFun) {
  auto &lg = lg::get();
  // Handle one of the three llvm memory intrinsics (memcpy, memmove or memset)
  if (llvm::isa<llvm::MemIntrinsic>(callStmt)) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Call statement is a LLVM MemIntrinsic!");
    return KillAll<IFDSConstAnalysis::d_t>::getInstance();
  }
  // Check if its a Call Instruction or an Invoke Instruction. If so, we
  // need to map all actual parameters into formal parameters.
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    // return KillAll<IFDSConstAnalysis::d_t>::getInstance();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Call statement: " << llvmIRToString(callStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Destination method: " << destFun->getName().str());
    return make_shared<MapFactsToCallee>(
        llvm::ImmutableCallSite(callStmt), destFun,
        [](IFDSConstAnalysis::d_t actual) {
          return actual->getType()->isPointerTy();
        });
  } /* end call/invoke instruction */

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getRetFlowFunction(IFDSConstAnalysis::n_t callSite,
                                      IFDSConstAnalysis::f_t calleeFun,
                                      IFDSConstAnalysis::n_t exitStmt,
                                      IFDSConstAnalysis::n_t retSite) {
  // return KillAll<IFDSConstAnalysis::d_t>::getInstance();
  // Map formal parameter back to the actual parameter in the caller.
  return make_shared<MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeFun, exitStmt,
      [](IFDSConstAnalysis::d_t formal) {
        return formal->getType()->isPointerTy();
      },
      [](IFDSConstAnalysis::f_t cmthd) {
        return cmthd->getReturnType()->isPointerTy();
      });
  // All other data-flow facts of the callee function are killed at this point
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getCallToRetFlowFunction(
    IFDSConstAnalysis::n_t callSite, IFDSConstAnalysis::n_t retSite,
    set<IFDSConstAnalysis::f_t> callees) {
  auto &lg = lg::get();
  // Process the effects of a llvm memory intrinsic function.
  if (llvm::isa<llvm::MemIntrinsic>(callSite)) {
    IFDSConstAnalysis::d_t pointerOp = callSite->getOperand(0);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Pointer Operand: " << llvmIRToString(pointerOp));
    set<IFDSConstAnalysis::d_t> pointsToSet = ptg.getPointsToSet(pointerOp);
    for (auto alias : pointsToSet) {
      if (isInitialized(alias)) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Compute context-relevant points-to "
                         "information of the pointer operand.");
        return make_shared<
            GenAll<IFDSConstAnalysis::d_t>>(/*pointsToSet*/
                                            getContextRelevantPointsToSet(
                                                pointsToSet,
                                                callSite->getFunction()),
                                            getZeroValue());
      }
    }
    markAsInitialized(pointerOp);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Pointer operand marked as initialized!");
  }

  // Pass everything else as identity
  return Identity<IFDSConstAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::getSummaryFlowFunction(IFDSConstAnalysis::n_t callStmt,
                                          IFDSConstAnalysis::f_t destFun) {
  return nullptr;
}

map<IFDSConstAnalysis::n_t, set<IFDSConstAnalysis::d_t>>
IFDSConstAnalysis::initialSeeds() {
  // just start in main()
  map<IFDSConstAnalysis::n_t, set<IFDSConstAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IFDSConstAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSConstAnalysis::d_t IFDSConstAnalysis::createZeroValue() const {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSConstAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSConstAnalysis::isZeroValue(IFDSConstAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

void IFDSConstAnalysis::printNode(ostream &os, IFDSConstAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSConstAnalysis::printDataFlowFact(ostream &os,
                                          IFDSConstAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSConstAnalysis::printFunction(ostream &os,
                                      IFDSConstAnalysis::f_t m) const {
  os << m->getName().str();
}

void IFDSConstAnalysis::printInitMemoryLocations() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(
      BOOST_LOG_SEV(lg, DEBUG)
      << "Printing all initialized memory location (or one of its alias)");
  for (auto stmt : IFDSConstAnalysis::Initialized) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << llvmIRToString(stmt));
  }
}

set<IFDSConstAnalysis::d_t> IFDSConstAnalysis::getContextRelevantPointsToSet(
    set<IFDSConstAnalysis::d_t> &PointsToSet,
    IFDSConstAnalysis::f_t CurrentContext) {
  PAMM_GET_INSTANCE;
  INC_COUNTER("[Calls] getContextRelevantPointsToSet", 1,
              PAMM_SEVERITY_LEVEL::Full);
  START_TIMER("Context-Relevant-PointsTo-Set Computation",
              PAMM_SEVERITY_LEVEL::Full);
  auto &lg = lg::get();
  set<IFDSConstAnalysis::d_t> ToGenerate;
  for (auto alias : PointsToSet) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Alias: " << llvmIRToString(alias));
    // Case (i + ii)
    if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(alias)) {
      if (isAllocaInstOrHeapAllocaFunction(alias)) {
        ToGenerate.insert(alias);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "alloca inst will be generated as a new fact!");
      } else if (I->getFunction() == CurrentContext) {
        ToGenerate.insert(alias);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "instruction within current function will "
                         "be generated as a new fact!");
      }
    } // Case (ii)
    else if (llvm::isa<llvm::GlobalValue>(alias)) {
      ToGenerate.insert(alias);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "global variable will be generated as a new fact!");
    } // Case (iii)
    else if (const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(alias)) {
      if (A->getParent() == CurrentContext) {
        ToGenerate.insert(alias);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "formal argument will be generated as a new fact!");
      }
    } // ignore everything else
  }
  PAUSE_TIMER("Context-Relevant-PointsTo-Set Computation",
              PAMM_SEVERITY_LEVEL::Full);
  ADD_TO_HISTOGRAM("Context-relevant Pointer", ToGenerate.size(), 1,
                   PAMM_SEVERITY_LEVEL::Full);
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

void IFDSConstAnalysis::emitTextReport(
    const SolverResults<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
                        BinaryDomain> &SR,
    ostream &os) {
  // 1) Remove all mutable memory locations
  for (auto f : ICF->getAllFunctions()) {
    for (auto exit : ICF->getExitPointsOf(f)) {
      std::set<const llvm::Value *> facts = SR.ifdsResultsAt(exit);
      // Empty facts means the exit statement is part of a not
      // analyzed function, thus remove all memory locations of that function
      if (facts.empty()) {
        for (auto mem_itr = AllMemLocs.begin(); mem_itr != AllMemLocs.end();) {
          if (auto Inst = llvm::dyn_cast<llvm::Instruction>(*mem_itr)) {
            if (Inst->getParent()->getParent() == f) {
              mem_itr = AllMemLocs.erase(mem_itr);
            } else {
              ++mem_itr;
            }
          } else {
            ++mem_itr;
          }
        }
      } else {
        for (auto fact : facts) {
          if (isAllocaInstOrHeapAllocaFunction(fact) ||
              llvm::isa<llvm::GlobalValue>(fact)) {
            // remove memory locations that are mutable, i.e. are valid facts
            AllMemLocs.erase(fact);
          }
        }
      }
    }
  }
  // 2) Print all immutbale/const memory locations
  os << "=========== IFDS Const Analysis Results ===========\n";
  if (AllMemLocs.empty()) {
    os << "No immutable memory locations found!\n";
  } else {
    os << "Immutable/const stack and/or heap memory locations:\n";
    for (auto memloc : AllMemLocs) {
      os << "\nIR  : " << llvmIRToString(memloc) << '\n';
    }
  }
  os << "\n===================================================\n";
}

} // namespace psr
