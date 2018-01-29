#include "IFDSConstAnalysis.h"

IFDSConstAnalysis::IFDSConstAnalysis(LLVMBasedICFG &icfg, PointsToGraph &ptg,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), ptg(ptg), EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                         const llvm::Instruction *succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getNormalFlowFunction()";
  // check all store instructions
  if (const llvm::StoreInst *store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    const llvm::Value *pointerOp = store->getPointerOperand();
    set<const llvm::Value *> pointsToSet = ptg.getPointsToSet(pointerOp);
    BOOST_LOG_SEV(lg, DEBUG) << "POINTS-TO SET OF POINTEROP";
    // only interested in points-to information within the function scope, i.e.
    // -local instructions
    // -function args of parent function
    // -global variable/pointer
    set<const llvm::Value *> ToGenerate;
    const llvm::Function *currentFunction = curr->getFunction();
    for (auto alias : pointsToSet) {
      alias->dump();
      if (const llvm::Instruction *I =
              llvm::dyn_cast<llvm::Instruction>(alias)) {
        if (I->getFunction() == currentFunction) {
          ToGenerate.insert(alias);
        }
      } else if (llvm::isa<llvm::GlobalValue>(alias)) {
        ToGenerate.insert(alias);
      } else if (const llvm::Argument *A =
                     llvm::dyn_cast<llvm::Argument>(alias)) {
        if (A->getParent() == currentFunction) {
          ToGenerate.insert(alias);
        }
      } else {
        BOOST_LOG_SEV(lg, DEBUG) << "Could not cast the following alias: "
                                 << alias->getName().str();
      }
    }
    // check if the pointerOp is global variable; if so, generate a new data
    // flow fact since global variables are always initialized and after the
    // first store inst. considered to be mutable
    if (auto Global = llvm::dyn_cast<llvm::GlobalValue>(pointerOp)) {
      return make_shared<GenAll<const llvm::Value *>>(ToGenerate, zeroValue());
    } else {
      // check if it's a variables (or its aliases) second store; if so,
      // generate a new data flow fact for itself and all alias within
      // the function
      // NOTE: if alias is a GV, in some cases it might not be inside the
      // storedOnce set since the first store instruction of GV is implicit
      for (auto alias : pointsToSet) {
        if (llvm::isa<llvm::GlobalValue>(alias) ||
            storedOnce.find(alias) != storedOnce.end()) {
          return make_shared<GenAll<const llvm::Value *>>(ToGenerate,
                                                          zeroValue());
        }
      }
      // remember the first store into the memory location pointerOp is
      // pointing to
      storedOnce.insert(pointerOp);
    }
  } /* end store instruction */

  // Pass everything else as identity
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                       const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallFlowFunction()";
  // Map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    llvm::ImmutableCallSite CallSite(callStmt);
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
        if (!constanalysis->isZeroValue(source)) {
          set<const llvm::Value *> res;
          res.insert(zerovalue);
          for (unsigned idx = 0; idx < actuals.size(); ++idx) {
            if (source == actuals[idx]) {
              res.insert(formals[idx]); // corresponding formal
            }
          }
          return res;
        } else {
          return {source};
        }
      }
    };
    return make_shared<CAFF>(CallSite, destMthd, zeroValue(), this);
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
  // We must map the generated parameter facts back to the caller
  struct CAFF : FlowFunction<const llvm::Value *> {
    llvm::ImmutableCallSite callSite;
    const llvm::Function *calleeMthd;
    const llvm::Value *zerovalue;
    const IFDSConstAnalysis *constanalysis;
    PointsToGraph *pointsToGraph;
    vector<const llvm::Value *> actuals;
    vector<const llvm::Value *> formals;
    CAFF(llvm::ImmutableCallSite callsite, const llvm::Function *callemthd,
         const llvm::Value *zv, const IFDSConstAnalysis *ca, PointsToGraph *ptg)
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
      if (!constanalysis->isZeroValue(source)) {
        set<const llvm::Value *> res;
        res.insert(zerovalue);
        // collect everything that is returned by pointer/ reference
        // ignore return by value, since it will be stored into a
        // virtual register
        for (unsigned idx = 0; idx < formals.size(); ++idx) {
          if (source == formals[idx] &&
              formals[idx]->getType()->isPointerTy()) {
            // generate the actual parameter and all its alias within the caller
            for (auto alias :
                 pointsToGraph->getAliasWithinFunction(actuals[idx])) {
              res.insert(alias);
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
  static ZeroValue *zero = new ZeroValue;
  return zero;
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
  cout << "PRINTING INIT SET" << '\n';
  for (auto stmt : IFDSConstAnalysis::storedOnce) {
    stmt->dump();
  }
}
