#include "IFDSTypeAnalysis.hh"

IFDSTypeAnalysis::IFDSTypeAnalysis(LLVMBasedICFG &icfg)
    : DefaultIFDSTabulationProblem(icfg) {
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
IFDSTypeAnalysis::getCallFlowFuntion(const llvm::Instruction *callStmt,
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
  const llvm::Function *mainfunction = icfg.getModule().getFunction("main");
  const llvm::Instruction *firstinst = &mainfunction->front().front();
  set<const llvm::Value *> iset{zeroValue()};
  map<const llvm::Instruction *, set<const llvm::Value *>> imap{
      {firstinst, iset}};
  return imap;
}

const llvm::Value *IFDSTypeAnalysis::createZeroValue() {
	static ZeroValue *zero = new ZeroValue;
	return zero;
}
