#include "IFDSTypeAnalysis.hh"

IFDSTypeAnalysis::IFDSTypeAnalysis(LLVMBasedICFG &icfg,
                                   vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTypeAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                        const llvm::Instruction *succ) {
  cout << "type analysis getNormalFlowFunction()" << endl;
  struct TAFF : FlowFunction<const llvm::Value *> {
    set<const llvm::Value *>
    computeTargets(const llvm::Value *source) override {
      return set<const llvm::Value *>{};
    }
  };
  return make_shared<TAFF>();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTypeAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd) {
  cout << "type analysis getCallFlowFunction()" << endl;
  struct TAFF : FlowFunction<const llvm::Value *> {
    set<const llvm::Value *>
    computeTargets(const llvm::Value *source) override {
      return set<const llvm::Value *>{};
    }
  };
  return make_shared<TAFF>();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTypeAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                     const llvm::Function *calleeMthd,
                                     const llvm::Instruction *exitStmt,
                                     const llvm::Instruction *retSite) {
  cout << "type analysis getRetFlowFunction()" << endl;
  struct TAFF : FlowFunction<const llvm::Value *> {
    set<const llvm::Value *>
    computeTargets(const llvm::Value *source) override {
      return set<const llvm::Value *>{};
    }
  };
  return make_shared<TAFF>();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTypeAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                           const llvm::Instruction *retSite) {
  cout << "type analysis getCallToRetFlowFunction()" << endl;
  struct TAFF : FlowFunction<const llvm::Value *> {
    set<const llvm::Value *>
    computeTargets(const llvm::Value *source) override {
      return set<const llvm::Value *>{};
    }
  };
  return make_shared<TAFF>();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSTypeAnalysis::initialSeeds() {
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IFDSTypeAnalysis::createZeroValue() {
  static ZeroValue *zero = new ZeroValue;
  return zero;
}

bool IFDSTypeAnalysis::isZeroValue(const llvm::Value *d) const {
  return isLLVMZeroValue(d);
}

string IFDSTypeAnalysis::D_to_string(const llvm::Value *d) {
  return llvmIRToString(d);
}

string IFDSTypeAnalysis::N_to_string(const llvm::Instruction *n) {
  return llvmIRToString(n);
}

string IFDSTypeAnalysis::M_to_string(const llvm::Function *m) {
  return m->getName().str();
}
