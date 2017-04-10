#include "IFDSTaintAnalysis.hh"

IFDSTaintAnalysis::SourceFunction
IFDSTaintAnalysis::findSourceFunction(const llvm::Function *f) {
  for (auto sourcefunction : source_functions) {
    if (f->getName().str().find(sourcefunction.name) != string::npos)
      return sourcefunction;
  }
  return SourceFunction();
}

IFDSTaintAnalysis::SinkFunction
IFDSTaintAnalysis::findSinkFunction(const llvm::Function *f) {
  for (auto sinkfunction : sink_functions) {
    if (f->getName().str().find(sinkfunction.name) != string::npos)
      return sinkfunction;
  }
  return SinkFunction();
}

bool IFDSTaintAnalysis::isSourceFunction(const llvm::Function *f) {
  for (auto sourcefunction : source_functions) {
    if (f->getName().str().find(sourcefunction.name) != string::npos)
      return true;
  }
  return false;
}

bool IFDSTaintAnalysis::isSinkFunction(const llvm::Function *f) {
  for (auto sinkfunction : sink_functions) {
    if (f->getName().str().find(sinkfunction.name) != string::npos)
      return true;
  }
  return false;
}

IFDSTaintAnalysis::IFDSTaintAnalysis(LLVMBasedInterproceduralICFG &icfg,
                                     llvm::LLVMContext &c)
    : DefaultIFDSTabulationProblem(icfg), context(c) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                         const llvm::Instruction *succ) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getNormalFlowFunction()"
       << endl;
  // Taint the commandline arguments
  if (icfg.getNameOfMethod(curr) == "main" && icfg.isStartPoint(curr)) {
    struct TAFF : FlowFunction<const llvm::Value *> {
      const llvm::Function *function;
      const llvm::Value *zerovalue;
      TAFF(const llvm::Function *f, const llvm::Value *zv)
          : function(f), zerovalue(zv) {}
      set<const llvm::Value *>
      computeTargets(const llvm::Value *source) override {
        if (source == zerovalue) {
          set<const llvm::Value *> res;
          for (auto &commandline_arg : function->getArgumentList()) {
            res.insert(&commandline_arg);
          }
          res.insert(zerovalue);
          return res;
        } else {
          return set<const llvm::Value *>{source};
        }
      }
    };
    return make_shared<TAFF>(curr->getFunction(), zeroValue());
  }
  // If a tainted value is stored, the store location is also tainted
  if (llvm::isa<llvm::StoreInst>(curr)) {
    const llvm::StoreInst *store = llvm::dyn_cast<llvm::StoreInst>(curr);
    struct TAFF : FlowFunction<const llvm::Value *> {
      const llvm::StoreInst *store;
      TAFF(const llvm::StoreInst *s) : store(s){};
      set<const llvm::Value *>
      computeTargets(const llvm::Value *source) override {
        if (store->getValueOperand() == source) {
          return set<const llvm::Value *>{store->getPointerOperand(), source};
        } else if (store->getValueOperand() != source &&
                   store->getPointerOperand() == source) {
          return set<const llvm::Value *>{};
        } else {
          return set<const llvm::Value *>{source};
        }
      }
    };
    return make_shared<TAFF>(store);
  }
  // If a tainted value is loaded, the loaded value is of course tainted
  if (llvm::isa<llvm::LoadInst>(curr)) {
    const llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(curr);
    struct TAFF : FlowFunction<const llvm::Value *> {
      const llvm::LoadInst *load;
      TAFF(const llvm::LoadInst *l) : load(l) {}
      set<const llvm::Value *>
      computeTargets(const llvm::Value *source) override {
        if (source == load->getPointerOperand()) {
          return set<const llvm::Value *>{load, source};
        } else {
          return set<const llvm::Value *>{source};
        }
      }
    };
    return make_shared<TAFF>(load);
  }
  // Otherwise we do not care and leave everything as it is
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getCallFlowFuntion(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallFlowFunction()"
       << endl;
  if (llvm::isa<llvm::CallInst>(callStmt)) {
    const llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(callStmt);
    if (isSourceFunction(call->getCalledFunction())) {
      // Generate the values, that are tainted by this call to a source function
      struct TAFF : FlowFunction<const llvm::Value *> {
        const llvm::CallInst *call;
        const llvm::Value *zerovalue;
        const SourceFunction sourcefunction;
        TAFF(const llvm::CallInst *cs, const llvm::Value *zv,
             const SourceFunction sf)
            : call(cs), zerovalue(zv), sourcefunction(sf) {}
        set<const llvm::Value *> computeTargets(const llvm::Value *source) {
          if (source == zerovalue) {
            set<const llvm::Value *> res;
            for (auto idx : sourcefunction.genargs) {
              res.insert(call->getArgOperand(idx));
            }
            if (sourcefunction.genreturn) {
              for (auto user : call->users()) {
                res.insert(user);
              }
            }
            res.insert(zerovalue);
            return res;
          } else {
            return set<const llvm::Value *>{source};
          }
        }
      };
      return make_shared<TAFF>(call, zeroValue(),
                               findSourceFunction(call->getCalledFunction()));
    } else if (isSinkFunction(call->getCalledFunction())) {
      struct TAFF : FlowFunction<const llvm::Value *> {
        const llvm::CallInst *call;
        const SinkFunction sinkfunction;
        TAFF(const llvm::CallInst *cs, const SinkFunction sf)
            : call(cs), sinkfunction(sf) {}
        set<const llvm::Value *> computeTargets(const llvm::Value *source) {
          for (auto idx : sinkfunction.sinkargs) {
            if (call->getArgOperand(idx) == source) {
              cout << "Uuups, found a taint!" << endl;
              return set<const llvm::Value *>{source};
            }
          }
          return set<const llvm::Value *>{source};
        }
      };
      return make_shared<TAFF>(call,
                               findSinkFunction(call->getCalledFunction()));
    } else {
      // Map the parameters to the 'normal' function
      struct TAFF : FlowFunction<const llvm::Value *> {
        const llvm::CallInst *call;
        const llvm::Value *zerovalue;
        TAFF(const llvm::CallInst *cs, const llvm::Value *zv)
            : call(cs), zerovalue(zv) {}
        set<const llvm::Value *> computeTargets(const llvm::Value *source) {
          // TODO: this needs some work!
          vector<const llvm::Value *> actuals;
          for (unsigned idx = 0; idx < call->getNumArgOperands(); ++idx) {
            actuals.push_back(call->getArgOperand(idx));
          }
          vector<const llvm::Value *> formals;
          for (unsigned idx = 0;
               idx < call->getCalledFunction()->getNumOperands(); ++idx) {
            formals.push_back(call->getCalledFunction()->getOperandUse(idx));
          }

          set<const llvm::Value *> res;
          for (unsigned idx = 0; idx < actuals.size(); ++idx) {
            if (source == actuals[idx]) {
              res.insert(formals[idx]);
              res.insert(source);
              res.insert(zerovalue);
            }
          }
          return res;

          if (source == zerovalue) {
            unsigned argcounter = 0;
            set<const llvm::Value *> res;
            for (auto &actual : call->arg_operands()) {
              for (auto &formal : call->getCalledFunction()->args()) {
                //									if
                //(&actual == source) {
                //										res.insert(&formal);
                //									}
              }
            }
            res.insert(zerovalue);
            return res;
          }
          return set<const llvm::Value *>{source};
        }
      };
      return make_shared<TAFF>(call, zeroValue());
      return Identity<const llvm::Value *>::v();
    }
    //		} else if (llvm::isa<llvm::InvokeInst>(callStmt)) {
    //			// TODO handle invoke statement
    //			return Identity<const llvm::Value*>::v();
  }
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                      const llvm::Function *calleeMthd,
                                      const llvm::Instruction *exitStmt,
                                      const llvm::Instruction *retSite) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getRetFlowFunction()"
       << endl;
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Instruction *retSite) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallToRetFlowFunction()"
       << endl;
  return Identity<const llvm::Value *>::v();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSTaintAnalysis::initialSeeds() {
  // just start in main()
  const llvm::Function *mainfunction = icfg.getModule().getFunction("main");
  const llvm::Instruction *firstinst = &(*(mainfunction->begin()->begin()));
  set<const llvm::Value *> iset{zeroValue()};
  map<const llvm::Instruction *, set<const llvm::Value *>> imap{
      {firstinst, iset}};
  return imap;
}

const llvm::Value *IFDSTaintAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  static llvm::Value *zeroValue =
      llvm::ConstantInt::get(context, llvm::APInt(0, 0, true));
  return zeroValue;
}
