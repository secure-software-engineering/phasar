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
  PAMM_FACTORY;
  REG_SETH("Context-relevant-PT");
  REG_COUNTER("Calls to getContextRelevantPointsToSet");
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                         const llvm::Instruction *succ) {
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
                  std::string::npos) {
            BOOST_LOG_SEV(lg, DEBUG)
                << "Store Instruction sets up or updates vtable - ignored!";
            return Identity<const llvm::Value *>::v();
          }
        }
      }
    } /* end vtable set-up instruction */
    const llvm::Value *pointerOp = Store->getPointerOperand();
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand of store Instruction: "
                             << llvmIRToString(pointerOp);
    set<const llvm::Value *> pointsToSet = ptg.getPointsToSet(pointerOp);
    // Check if this store instruction is the second write access to the memory
    // location the pointer operand or it's alias are pointing to.
    // This is done by checking the Initialized set.
    // If so, generate the pointer operand as a new data-flow fact. Also
    // generate data-flow facts of all alias that meet the 'context-relevant'
    // requirements! (see getContextRelevantPointsToSet function)
    // NOTE: The points-to set of value x also contains the value x itself!
    //
    // CONSERVATIVE ANALYSIS: Simply generates the whole pointsToSet!
    for (auto alias : pointsToSet) {
      if (isInitialized(alias)) {
        BOOST_LOG_SEV(lg, DEBUG) << "Compute context-relevant points-to "
                                    "information for the pointer operand.";
        return make_shared<GenAll<const llvm::Value *>>(/*pointsToSet*/
            getContextRelevantPointsToSet(pointsToSet, curr->getFunction()),
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
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                       const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallFlowFunction()";
  // If the called function is one of the following three llvm intrinsics
  //   - memcpy
  //   - memmove
  //   - memset
  // we count them as a write access to the target memory location. To do
  // so, we need to kill all data-flow facts at this point. The respective
  // data-flow facts are then generated in the corresponding call-to-return
  // flow function.
  //if (llvm::isa<llvm::MemIntrinsic>(callStmt)) {
  //  BOOST_LOG_SEV(lg, DEBUG) << "Call statement is a LLVM MemIntrinsic!";
  //  return KillAll<const llvm::Value *>::v();
  //}
  // Check if its a Call Instruction or an Invoke Instruction. If so, we
  // need to map all actual parameters into formal parameters.
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    return KillAll<const llvm::Value *>::v();
    //BOOST_LOG_SEV(lg, DEBUG) << "Call statement: " << llvmIRToString(callStmt);
    //BOOST_LOG_SEV(lg, DEBUG) << "Destination method: "
    //                         << destMthd->getName().str();
    //struct CAFF : FlowFunction<const llvm::Value *> {
    //  llvm::ImmutableCallSite callSite;
    //  const llvm::Function *destMthd;
    //  const llvm::Value *zerovalue;
    //  const IFDSConstAnalysis *constanalysis;
    //  vector<const llvm::Value *> actuals;
    //  vector<const llvm::Value *> formals;
    //  CAFF(llvm::ImmutableCallSite cs, const llvm::Function *dm,
    //       const llvm::Value *zv, const IFDSConstAnalysis *ca)
    //      : callSite(cs), destMthd(dm), zerovalue(zv), constanalysis(ca) {
    //    // Set up the actual parameters
    //    for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
    //      actuals.push_back(callSite.getArgOperand(idx));
    //    }
    //    // Set up the formal parameters
    //    for (unsigned idx = 0; idx < destMthd->getArgumentList().size();
    //         ++idx) {
    //      formals.push_back(getNthFunctionArgument(destMthd, idx));
    //    }
    //  }
    //  set<const llvm::Value *> computeTargets(const llvm::Value *source) {
    //    auto &lg = lg::get();
    //    if (!constanalysis->isZeroValue(source)) {
    //      set<const llvm::Value *> res;
    //      // Map actual parameter of pointer type into corresponding
    //      // formal parameter.
    //      for (unsigned idx = 0; idx < actuals.size(); ++idx) {
    //        if (source == actuals[idx] && actuals[idx]->getType()->isPointerTy()) {
    //          BOOST_LOG_SEV(lg, DEBUG) << "Actual Param.: "
    //                                   << llvmIRToString(actuals[idx]);
    //          BOOST_LOG_SEV(lg, DEBUG) << "Formal Param.: "
    //                                   << llvmIRToString(formals[idx]);
    //          res.insert(formals[idx]); // corresponding formal
    //        }
    //      }
    //      return res;
    //    } else {
    //      return {source};
    //    }
    //  }
    //};
    //return make_shared<CAFF>(llvm::ImmutableCallSite(callStmt), destMthd,
    //                         zeroValue(), this);
  } /* end call/invoke instruction */

  // Pass everything else as identity
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                      const llvm::Function *calleeMthd,
                                      const llvm::Instruction *exitStmt,
                                      const llvm::Instruction *retSite) {
  return KillAll<const llvm::Value *>::v();
  //auto &lg = lg::get();
  //BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getRetFlowFunction()";
  //BOOST_LOG_SEV(lg, DEBUG) << "Call site: " << llvmIRToString(callSite);
  //BOOST_LOG_SEV(lg, DEBUG) << "Caller context: "
  //                         << callSite->getFunction()->getName().str();
  //BOOST_LOG_SEV(lg, DEBUG) << "Retrun site: " << llvmIRToString(retSite);
  //BOOST_LOG_SEV(lg, DEBUG) << "Callee method: " << calleeMthd->getName().str();
  //BOOST_LOG_SEV(lg, DEBUG) << "Callee exit statement: "
  //                         << llvmIRToString(exitStmt);
  //// We must map formal parameter back to the actual parameter in the caller.
  //struct CAFF : FlowFunction<const llvm::Value *> {
  //  llvm::ImmutableCallSite callSite;
  //  const llvm::Function *calleeMthd;
  //  const llvm::ReturnInst *exitStmt;
  //  const llvm::Value *zerovalue;
  //  IFDSConstAnalysis *constanalysis;
  //  PointsToGraph *pointsToGraph;
  //  vector<const llvm::Value *> actuals;
  //  vector<const llvm::Value *> formals;
  //  CAFF(llvm::ImmutableCallSite callsite, const llvm::Function *callemthd,
  //       const llvm::Instruction *exitstmt, const llvm::Value *zv,
  //       IFDSConstAnalysis *ca, PointsToGraph *ptg)
  //      : callSite(callsite), calleeMthd(callemthd),
  //        exitStmt(llvm::dyn_cast<llvm::ReturnInst>(exitstmt)), zerovalue(zv),
  //        constanalysis(ca), pointsToGraph(ptg) {
  //    // Set up the actual parameters
  //    for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
  //      actuals.push_back(callSite.getArgOperand(idx));
  //    }
  //    // Set up the formal parameters
  //    for (unsigned idx = 0; idx < calleeMthd->getArgumentList().size();
  //         ++idx) {
  //      formals.push_back(getNthFunctionArgument(calleeMthd, idx));
  //    }
  //  }
  //  set<const llvm::Value *>
  //  computeTargets(const llvm::Value *source) override {
  //    auto &lg = lg::get();
  //    if (!constanalysis->isZeroValue(source)) {
  //      set<const llvm::Value *> res;
  //      const llvm::Function *callSiteFunction = callSite->getFunction();
  //      // (i): Map parameter of pointer type back into the caller context.
  //      //
  //      // CONSERVATIVE ANALYSIS:: Generate also all alias of the actual parameter.
  //      for (unsigned idx = 0; idx < formals.size(); ++idx) {
  //        if (source == formals[idx] &&
  //            formals[idx]->getType()->isPointerTy()) {
  //          BOOST_LOG_SEV(lg, DEBUG) << "Actual Param.: "
  //                                   << llvmIRToString(actuals[idx]);
  //          BOOST_LOG_SEV(lg, DEBUG) << "Formal Param.: "
  //                                   << llvmIRToString(formals[idx]);
  //          res.insert(actuals[idx]);
  //          //BOOST_LOG_SEV(lg, DEBUG) << "Generate alias for the actual parameter.";
  //          //set<const llvm::Value *> pointsToSet =
  //          //    pointsToGraph->getPointsToSet(actuals[idx]);
  //          //for (auto fact : pointsToSet/*constanalysis->getContextRelevantPointsToSet(
  //          //         pointsToSet, callSiteFunction)*/) {
  //          //  res.insert(fact);
  //          //}
  //        }
  //      }
  //      // (ii): Return value mutable.
  //      // If the return value is a data-flow fact and of pointer type, we
  //      // need to generate the return value in the caller context. We
  //      // do not pass all alias of the return value, since this would carry no
  //      // vital information, we would only model alias information through
  //      // data-flow facts. Again, only points-to information and the
  //      // Initialized set determine, if new data-flow facts will be generated!
  //      //
  //      // CONSERVATIVE ANALYSIS: Also generate all alias of the return
  //      // value back in the caller context.
  //      if (source == exitStmt->getReturnValue() &&
  //          calleeMthd->getReturnType()->isPointerTy()) {
  //        BOOST_LOG_SEV(lg, DEBUG) << "Callee return value: "
  //                                 << llvmIRToString(source);
  //        res.insert(source);
  //        //BOOST_LOG_SEV(lg, DEBUG) << "Generate alias for the return value.";
  //        //set<const llvm::Value *> pointsToSet =
  //        //    pointsToGraph->getPointsToSet(callSite.getInstruction());
  //        //for (auto fact : getPointsToSet/*constanalysis->getContextRelevantPointsToSet(
  //        //         pointsToSet, callSiteFunction)*/) {
  //        //  res.insert(fact);
  //        //}
  //      }
  //      return res;
  //    }
  //    // (iii): Return value is only initialized.
  //    // Check if the return value is a pointer type and initialized.
  //    // We then need to mark the call site as initialized.
  //    // UPDATE: This was just a workaround to a bug in the points-to
  //    // analysis, where returned memory was missing points-to relations
  //    // between different function contexts.
  //    // if (calleeMthd->getReturnType()->isPointerTy()) {
  //    //  BOOST_LOG_SEV(lg, DEBUG) << "Callee Return Value: "
  //    //                           <<
  //    //                           llvmIRToString(exitStmt->getReturnValue());
  //    //   for (auto alias :
  //    //   pointsToGraph->getPointsToSet(exitStmt->getReturnValue())) {
  //    //     if (constanalysis->isInitialized(alias)) {
  //    //       BOOST_LOG_SEV(lg, DEBUG) << "Call site is marked as
  //    //       initialized!";
  //    //       constanalysis->markAsInitialized(callSite.getInstruction());
  //    //       break;
  //    //     }
  //    //   }
  //    //}
  //    // (iv): Just draw the zero edge.
  //    return {source};
  //  }
  //};
  //return make_shared<CAFF>(llvm::ImmutableCallSite(callSite), calleeMthd,
  //                         exitStmt, zeroValue(), this, &ptg);
  // All other data-flow facts of the callee function are killed at this point
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallToRetFlowFunction()";
  BOOST_LOG_SEV(lg, DEBUG) << "Call site: " << llvmIRToString(callSite);
  BOOST_LOG_SEV(lg, DEBUG) << "Return site: " << llvmIRToString(retSite);
  // Process the effects of a llvm memory intrinsic function. They are handled
  // as a store instruction, i.e. we generate new data-flow facts if the first
  // operand (pointer to destination) is already initialized. Otherwise, we mark
  // the first operand as initialized.
  if (llvm::isa<llvm::MemIntrinsic>(callSite)) {
    const llvm::Value *pointerOp = callSite->getOperand(0);
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer Operand: "
                             << llvmIRToString(pointerOp);
    set<const llvm::Value *> pointsToSet = ptg.getPointsToSet(pointerOp);
    for (auto alias : pointsToSet) {
      if (isInitialized(alias)) {
        BOOST_LOG_SEV(lg, DEBUG) << "Compute context-relevant points-to "
                                    "information of the pointer operand.";
        return make_shared<GenAll<const llvm::Value *>>(/*pointsToSet*/
            getContextRelevantPointsToSet(pointsToSet, callSite->getFunction()),
            zeroValue());
      }
    }
    markAsInitialized(pointerOp);
    BOOST_LOG_SEV(lg, DEBUG) << "Pointer operand marked as initialized!";
  }

  // Pass everything else as identity
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
  BOOST_LOG_SEV(lg, DEBUG)
      << "Printing all initialized memory location (or one of its alias)";
  for (auto stmt : IFDSConstAnalysis::Initialized) {
    BOOST_LOG_SEV(lg, DEBUG) << llvmIRToString(stmt);
  }
}

set<const llvm::Value *> IFDSConstAnalysis::getContextRelevantPointsToSet(
    set<const llvm::Value *> &PointsToSet,
    const llvm::Function *CurrentContext) {
  PAMM_FACTORY;
  INC_COUNTER("Calls to getContextRelevantPointsToSet");
  START_TIMER("Compute ContextRelevantPointsToSet");
  auto &lg = lg::get();
  set<const llvm::Value *> ToGenerate;
  // We only want/need to generate alias if they meet one of the
  // following conditions
  //  (i)   alias is an instruction from within the current function
  //        context
  //  (ii)  alias is an allocation instruction for stack memory (alloca)
  //        or heap memory (new, new[], malloc, calloc, realloc) from
  //        any function context
  //  (iii) alias is a global variable
  //  (iv)  alias is a formal argument of the current function
  //  (v)   alias is a return value of pointer type
  // Condition (i) is necessary to cover the case, when an initialized
  // memory location is mutated in a function different from where its
  // original allocation site.
  // Condition (iii) is necessary to be able to map mutated parameter
  // back to the caller context if needed. Same goes for (iv).
  //
  // Everything else will be ignored since we are not interested in
  // intermediate pointer or values of other functions, i.e. values
  // in virtual registers.
  // Only points-to information and the Initialized set determine, if
  // new data-flow facts will be generated.
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
  ADD_TO_SETH("Context-relevant-PT", ToGenerate.size());
  return ToGenerate;
}

bool IFDSConstAnalysis::isInitialized(const llvm::Value *d) const {
  return llvm::isa<llvm::GlobalValue>(d) || Initialized.count(d);
}

void IFDSConstAnalysis::markAsInitialized(const llvm::Value *d) {
  Initialized.insert(d);
}

size_t IFDSConstAnalysis::getInitializedSize() {
  return Initialized.size();
}
