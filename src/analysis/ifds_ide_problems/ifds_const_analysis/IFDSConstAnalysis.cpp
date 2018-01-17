#include "IFDSConstAnalysis.h"

IFDSConstAnalysis::IFDSConstAnalysis(LLVMBasedICFG &icfg,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
  ptg = icfg.getWholeModulePTG();
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
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING POINTS-TO SET OF POINTER OPERATOR";
    for(auto pointer : pointsToSet) {
      pointer->dump();
    }
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING ALIAS OF POINTER OPERATOR WITHIN FUNCTION";
    set<const llvm::Value *> aliasSet = ptg.getAliasWithinFunction(store->getPointerOperand());
    for(auto alias : aliasSet) {
      alias->dump();
    }
    // check if the pointerOp is global variable; if so, generate a new data flow fact
    // since global variables are always initialized and after the first store inst.
    // considered to be mutable
    if (auto Global = llvm::dyn_cast<llvm::GlobalValue>(pointerOp)) {
      BOOST_LOG_SEV(lg, DEBUG) << "FOUND GLOBAL";
      return make_shared<Gen<pair<const llvm::Value *,bool>>>(Global, zeroValue());
    } else {
      // check if it's a variables second store; if so, generate a new data flow fact
      if (pointsToSet.size() == 1 && storedOnce.find(pointerOp) != storedOnce.end()) {
        return make_shared<Gen<pair<const llvm::Value *,bool>>>(pointerOp, zeroValue());
      } else {
        for (auto value : pointsToSet) {
          if (storedOnce.find(value) != storedOnce.end()) {
            return make_shared<GenAll<pair<const llvm::Value *,bool>>>(pointsToSet, zeroValue());
          }
        }
      }
      // put the pointer operator into storedOnce set
      storedOnce.insert(pointerOp);
    }
  }
  if (const llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    set<const llvm::Value *> loadPointsToSet = ptg.getPointsToSet(load);
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING POINTS-TO SET OF LOAD INSTRUCTION";
    for (auto stmt : loadPointsToSet) {
      stmt->dump();
    }
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING POINTS-TO SET OF LOAD OPERATOR";
    set<const llvm::Value *> loadOpPointsToSet = ptg.getPointsToSet(load->getPointerOperand());
    for (auto stmt : loadOpPointsToSet) {
      stmt->dump();
    }
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING USER OF LOAD OPERATOR";
    for (auto user : load->getPointerOperand()->users()) {
      user->dump();
    }
  }
  return Identity<pair<const llvm::Value *,bool>>::v();
}

shared_ptr<FlowFunction<pair<const llvm::Value *,bool>>>
IFDSConstAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                       const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallFlowFunction()";
  // Map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    llvm::ImmutableCallSite CallSite(callStmt);
    // Do the mapping
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING FORMAL PARAMETERS";
    // set up the formal parameters
    for (unsigned idx = 0; idx < destMthd->getArgumentList().size(); ++idx) {
      getNthFunctionArgument(destMthd, idx)->dump();
    }
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING ACTUAL PARAMETERS";
    // set up the actual parameters
    for (unsigned idx = 0; idx < CallSite.getNumArgOperands(); ++idx) {
      CallSite.getArgOperand(idx)->dump();
    }
//    struct TAFF : FlowFunction<const llvm::Value *> {
//      llvm::ImmutableCallSite callSite;
//      const llvm::Function *destMthd;
//      const llvm::Value *zerovalue;
//      const IFDSTaintAnalysis *taintanalysis;
//      vector<const llvm::Value *> actuals;
//      vector<const llvm::Value *> formals;
//      TAFF(llvm::ImmutableCallSite cs, const llvm::Function *dm,
//           const llvm::Value *zv, const IFDSTaintAnalysis *ta)
//        : callSite(cs), destMthd(dm), zerovalue(zv), taintanalysis(ta) {
//        // set up the actual parameters
//        for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
//          actuals.push_back(callSite.getArgOperand(idx));
//        }
//        // set up the actual parameters
//        for (unsigned idx = 0; idx < destMthd->getArgumentList().size();
//             ++idx) {
//          formals.push_back(getNthFunctionArgument(destMthd, idx));
//        }
//      }
//      set<const llvm::Value *> computeTargets(const llvm::Value *source) {
//        if (!taintanalysis->isZeroValue(source)) {
//          set<const llvm::Value *> res;
//          for (unsigned idx = 0; idx < actuals.size(); ++idx) {
//            if (source == actuals[idx]) {
//              res.insert(formals[idx]); // corresponding formal
//              // res.insert(source); // corresponding actual
//              res.insert(zerovalue);
//            }
//          }
//          return res;
//        } else {
//          return {source};
//        }
//      }
//    };
//    return make_shared<TAFF>(CallSite, destMthd, zeroValue(), this);
  }
  // Pass everything else as identity
  return Identity<pair<const llvm::Value *,bool>>::v();
}

shared_ptr<FlowFunction<pair<const llvm::Value *,bool>>>
IFDSConstAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                      const llvm::Function *calleeMthd,
                                      const llvm::Instruction *exitStmt,
                                      const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getRetFlowFunction()";
  // Map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(callSite) ||
      llvm::isa<llvm::InvokeInst>(callSite)) {
    llvm::ImmutableCallSite CallSite(callSite);
    // Do the mapping
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING FORMAL PARAMETERS";
    // set up the formal parameters
    for (unsigned idx = 0; idx < calleeMthd->getArgumentList().size(); ++idx) {
      getNthFunctionArgument(calleeMthd, idx)->dump();
    }
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING ACTUAL PARAMETERS";
    // set up the actual parameters
    for (unsigned idx = 0; idx < CallSite.getNumArgOperands(); ++idx) {
      CallSite.getArgOperand(idx)->dump();
    }
  }
  // Pass everything else as identity
  return Identity<pair<const llvm::Value *,bool>>::v();
}

shared_ptr<FlowFunction<pair<const llvm::Value *,bool>>>
IFDSConstAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallToRetFlowFunction()";
//  return KillAll<pair<const llvm::Value *,bool>>::v();
  return Identity<pair<const llvm::Value *,bool>>::v();
}

shared_ptr<FlowFunction<pair<const llvm::Value *,bool>>>
IFDSConstAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                          const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getSummaryFlowFunction()";

  return nullptr;
}

map<const llvm::Instruction *, set<pair<const llvm::Value *,bool>>>
IFDSConstAnalysis::initialSeeds() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::initialSeeds()";
  // just start in main()
  map<const llvm::Instruction *, set<pair<const llvm::Value *,bool>>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<pair<const llvm::Value *,bool>>({zeroValue()})));
  }
  return SeedMap;
}

pair<const llvm::Value *,bool>IFDSConstAnalysis::createZeroValue() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::createZeroValue()";
  // create a special value to represent the zero value!
  static ZeroValue *zero = new ZeroValue;
  return zero;
}

bool IFDSConstAnalysis::isZeroValue(pair<const llvm::Value *,bool> d) const {
  return isLLVMZeroValue(d.first);
}

string IFDSConstAnalysis::DtoString(pair<const llvm::Value *,bool> d) {
  return llvmIRToString(d) ;
}

string IFDSConstAnalysis::NtoString(const llvm::Instruction *n) {
  return llvmIRToString(n);
}

string IFDSConstAnalysis::MtoString(const llvm::Function *m) {
  return m->getName().str();
}

void IFDSConstAnalysis::
printInitilizedSet() {
  cout << "PRINTING INIT SET" << '\n';
  for(auto stmt : IFDSConstAnalysis::storedOnce) {
    stmt->dump();
  }
}
