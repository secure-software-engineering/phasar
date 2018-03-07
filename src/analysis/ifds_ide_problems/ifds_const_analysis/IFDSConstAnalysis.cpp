/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "IFDSConstAnalysis.h"

IFDSConstAnalysis::IFDSConstAnalysis(LLVMBasedICFG &icfg,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), ptg(icfg.getWholeModulePTG()),
      EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                         const llvm::Instruction *succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getNormalFlowFunction()";
  // check all store instructions
  if (const llvm::StoreInst *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    // if store instruction sets up or updates the vtable, ignore it!
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
                  std::string::npos) {
            BOOST_LOG_SEV(lg, DEBUG)
                << "Store Instruction sets up or updates vtable - ignored!";
            return Identity<const llvm::Value *>::v();
          }
        }
      }
    }
    const llvm::Value *pointerOp = Store->getPointerOperand();
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand of store Instruction: "
                             << llvmIRToString(pointerOp);
    set<const llvm::Value *> pointsToSet = ptg.getPointsToSet(pointerOp);
    // Check if this store instruction is the second write access to the memory
    // location that pointer operand or it's alias are pointing to. If so,
    // generate a new data flow fact for the pointer operand. Also generate
    // data flow facts for all alias of the pointer operand from within
    // the function.
    // NOTE: If alias is a global variable, it might not be inside the
    // storedOnce set since GV is always initialized, i.e. the first store
    // will mark the memory location GV is pointing to as mutable!
    for (auto alias : pointsToSet) {
      if (llvm::isa<llvm::GlobalValue>(alias) || storedOnce.count(alias)) {
        BOOST_LOG_SEV(lg, DEBUG) << "Compute context-relevant points-to "
                                    "information of the pointer operand.";
        return make_shared<GenAll<const llvm::Value *>>(
            getContextRelevantPointsToSet(pointsToSet, curr->getFunction()),
            zeroValue());
      }
    }
    // If it is the first write access to the memory location that pointer
    // operand is pointing, mark it as initialized by adding the pointer
    // operand to the storedOnce set. We do not generate any new data flow
    // facts at this point.
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand marked as initialized!";
    storedOnce.insert(pointerOp);
  } /* end store instruction */

  // Pass everything else as identity
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                       const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallFlowFunction()";
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    BOOST_LOG_SEV(lg, DEBUG) << "Call Statement: " << llvmIRToString(callStmt);
    BOOST_LOG_SEV(lg, DEBUG) << "Destination Method: "
                             << destMthd->getName().str();

    // If the called function is one of the following three llvm intrinsics
    //   - memcpy
    //   - memmove
    //   - memset
    // handle it as a store instruction!
    // Otherwise, map the actual parameter into the formal parameters.
    if (llvm::isa<llvm::MemIntrinsic>(callStmt)) {
      BOOST_LOG_SEV(lg, DEBUG) << "Call Statement is a LLVM MemIntrinsic!";
      const llvm::Value *pointerOp = callStmt->getOperand(0);
      BOOST_LOG_SEV(lg, DEBUG) << "Pointer Operand: "
                               << llvmIRToString(pointerOp);
      set<const llvm::Value *> pointsToSet = ptg.getPointsToSet(pointerOp);
      for (auto alias : pointsToSet) {
        if (llvm::isa<llvm::GlobalValue>(alias) || storedOnce.count(alias)) {
          BOOST_LOG_SEV(lg, DEBUG) << "Compute context-relevant points-to "
                                      "information of the pointer operand.";
          return make_shared<GenAll<const llvm::Value *>>(
              getContextRelevantPointsToSet(pointsToSet,
                                            callStmt->getFunction()),
              zeroValue());
        }
      }
      BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand marked as initialized!";
      storedOnce.insert(pointerOp);
    } else {
      struct CAFF : FlowFunction<const llvm::Value *> {
        llvm::ImmutableCallSite callSite;
        const llvm::Function *destMthd;
        const llvm::Value *zerovalue;
        const IFDSConstAnalysis *constanalysis;
        vector<const llvm::Value *> actuals;
        vector<const llvm::Value *> formals;
        CAFF(llvm::ImmutableCallSite cs, const llvm::Function *dm,
             const llvm::Value *zv, const IFDSConstAnalysis *ca)
            : callSite(cs), destMthd(dm), zerovalue(zv), constanalysis(ca) {
          // set up the actual parameters
          for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
            actuals.push_back(callSite.getArgOperand(idx));
          }
          // set up the formal parameters
          for (unsigned idx = 0; idx < destMthd->getArgumentList().size();
               ++idx) {
            formals.push_back(getNthFunctionArgument(destMthd, idx));
          }
        }
        set<const llvm::Value *> computeTargets(const llvm::Value *source) {
          auto &lg = lg::get();
          if (!constanalysis->isZeroValue(source)) {
            set<const llvm::Value *> res;
            for (unsigned idx = 0; idx < actuals.size(); ++idx) {
              if (source == actuals[idx]) {
                BOOST_LOG_SEV(lg, DEBUG) << "Actual Param.: "
                                         << llvmIRToString(actuals[idx]);
                BOOST_LOG_SEV(lg, DEBUG) << "Formal Param.: "
                                         << llvmIRToString(formals[idx]);
                res.insert(formals[idx]); // corresponding formal
              }
            }
            return res;
          } else {
            return {source};
          }
        }
      };
      return make_shared<CAFF>(llvm::ImmutableCallSite(callStmt), destMthd,
                               zeroValue(), this);
    }
  } /* end call/invoke instruction */

  // Pass everything else as identity
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                      const llvm::Function *calleeMthd,
                                      const llvm::Instruction *exitStmt,
                                      const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getRetFlowFunction()";
  BOOST_LOG_SEV(lg, DEBUG) << "Callsite: " << llvmIRToString(callSite);
  BOOST_LOG_SEV(lg, DEBUG) << "Callee Method: " << calleeMthd->getName().str();
  BOOST_LOG_SEV(lg, DEBUG) << "Exit Statement: " << llvmIRToString(exitStmt);
  BOOST_LOG_SEV(lg, DEBUG) << "Retrun Site: " << llvmIRToString(retSite);
  // We must map formal parameter back to the actual parameter in the caller.
  // We also generate all relevant alias within the caller context!
  struct CAFF : FlowFunction<const llvm::Value *> {
    llvm::ImmutableCallSite callSite;
    const llvm::Function *calleeMthd;
    const llvm::Value *zerovalue;
    IFDSConstAnalysis *constanalysis;
    PointsToGraph *pointsToGraph;
    vector<const llvm::Value *> actuals;
    vector<const llvm::Value *> formals;
    CAFF(llvm::ImmutableCallSite callsite, const llvm::Function *callemthd,
         const llvm::Value *zv, IFDSConstAnalysis *ca, PointsToGraph *ptg)
        : callSite(callsite), calleeMthd(callemthd), zerovalue(zv),
          constanalysis(ca), pointsToGraph(ptg) {
      // set up the actual parameters
      for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
        actuals.push_back(callSite.getArgOperand(idx));
      }
      // set up the formal parameters
      for (unsigned idx = 0; idx < calleeMthd->getArgumentList().size();
           ++idx) {
        formals.push_back(getNthFunctionArgument(calleeMthd, idx));
      }
    }
    set<const llvm::Value *>
    computeTargets(const llvm::Value *source) override {
      auto &lg = lg::get();
      if (!constanalysis->isZeroValue(source)) {
        set<const llvm::Value *> res;
        // collect everything that is returned by pointer/ reference
        // ignore return by value, since it will be stored into a
        // virtual register
        for (unsigned idx = 0; idx < formals.size(); ++idx) {
          if (source == formals[idx] &&
              formals[idx]->getType()->isPointerTy()) {
            // generate the actual parameter and all its alias within the caller
            const llvm::Function *callSiteFunction = callSite->getFunction();
            BOOST_LOG_SEV(lg, DEBUG) << "Actual Param.: "
                                     << llvmIRToString(actuals[idx]);
            BOOST_LOG_SEV(lg, DEBUG) << "Formal Param.: "
                                     << llvmIRToString(formals[idx]);
            BOOST_LOG_SEV(lg, DEBUG) << "Caller Context: "
                                     << callSiteFunction->getName().str();
            BOOST_LOG_SEV(lg, DEBUG)
                << "Compute caller context-relevant points-to information.";
            set<const llvm::Value *> pointsToSet =
                pointsToGraph->getPointsToSet(actuals[idx]);
            for (auto fact : constanalysis->getContextRelevantPointsToSet(
                     pointsToSet, callSiteFunction)) {
              res.insert(fact);
            }
          }
        }
        return res;
      }
      // else just draw the zero edge
      return {source};
    }
  };
  return make_shared<CAFF>(llvm::ImmutableCallSite(callSite), calleeMthd,
                           zeroValue(), this, &ptg);
  // All other data flow facts of the callee function are killed at this point
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallToRetFlowFunction()";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                          const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getSummaryFlowFunction()";

  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSConstAnalysis::initialSeeds() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::initialSeeds()";
  // just start in main()
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IFDSConstAnalysis::createZeroValue() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::createZeroValue()";
  // create a special value to represent the zero value!
  return ZeroValue::getInstance();
}

bool IFDSConstAnalysis::isZeroValue(const llvm::Value *d) const {
  return isLLVMZeroValue(d);
}

string IFDSConstAnalysis::DtoString(const llvm::Value *d) {
  return llvmIRToString(d);
}

string IFDSConstAnalysis::NtoString(const llvm::Instruction *n) {
  return llvmIRToString(n);
}

string IFDSConstAnalysis::MtoString(const llvm::Function *m) {
  return m->getName().str();
}

void IFDSConstAnalysis::printInitilizedSet() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Printing the 'storedOnce' set";
  for (auto stmt : IFDSConstAnalysis::storedOnce) {
    BOOST_LOG_SEV(lg, DEBUG) << llvmIRToString(stmt);
  }
}

set<const llvm::Value *> IFDSConstAnalysis::getContextRelevantPointsToSet(
    set<const llvm::Value *> &PointsToSet, const llvm::Function *context) {
  auto &lg = lg::get();
  set<const llvm::Value *> ToGenerate;
  for (auto alias : PointsToSet) {
    BOOST_LOG_SEV(lg, DEBUG) << "Alias: " << llvmIRToString(alias);
    if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(alias)) {
      if (I->getFunction() == context) {
        ToGenerate.insert(alias);
        BOOST_LOG_SEV(lg, DEBUG) << "will be generated as a new fact!";
      }
    } else if (llvm::isa<llvm::GlobalValue>(alias)) {
      ToGenerate.insert(alias);
      BOOST_LOG_SEV(lg, DEBUG) << "will be generated as a new fact!";
    } else if (const llvm::Argument *A =
                   llvm::dyn_cast<llvm::Argument>(alias)) {
      if (A->getParent() == context) {
        ToGenerate.insert(alias);
        BOOST_LOG_SEV(lg, DEBUG) << "will be generated as a new fact!";
      }
    } else {
      BOOST_LOG_SEV(lg, DEBUG) << "could not cast this alias!";
    }
  }
  return ToGenerate;
}
