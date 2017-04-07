#include "IDETaintAnalysis.hh"

bool IDETaintAnalysis::set_contains_str(set<string> s, string str) {
  return s.find(str) != s.end();
}

IDETaintAnalysis::IDETaintAnalysis(LLVMBasedInterproceduralICFG &icfg,
                                   llvm::LLVMContext &c)
    : DefaultIDETabulationProblem(icfg), context(c) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                        const llvm::Instruction *succ) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getCallFlowFuntion(const llvm::Instruction *callStmt,
                                     const llvm::Function *destMthd) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                     const llvm::Function *calleeMthd,
                                     const llvm::Instruction *exitStmt,
                                     const llvm::Instruction *retSite) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                           const llvm::Instruction *retSite) {
  return Identity<const llvm::Value *>::v();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IDETaintAnalysis::initialSeeds() {
  // just start in main()
  const llvm::Function *mainfunction = icfg.getModule().getFunction("main");
  const llvm::Instruction *firstinst = &(*(mainfunction->begin()->begin()));
  set<const llvm::Value *> iset{zeroValue()};
  map<const llvm::Instruction *, set<const llvm::Value *>> imap{
      {firstinst, iset}};
  return imap;
}

const llvm::Value *IDETaintAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  static llvm::Value *zeroValue =
      llvm::ConstantInt::get(context, llvm::APInt(0, 0, true));
  return zeroValue;
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getNormalEdgeFunction(const llvm::Instruction *curr,
                                        const llvm::Value *currNode,
                                        const llvm::Instruction *succ,
                                        const llvm::Value *succNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getCallEdgeFunction(const llvm::Instruction *callStmt,
                                      const llvm::Value *srcNode,
                                      const llvm::Function *destiantionMethod,
                                      const llvm::Value *destNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getReturnEdgeFunction(const llvm::Instruction *callSite,
                                        const llvm::Function *calleeMethod,
                                        const llvm::Instruction *exitStmt,
                                        const llvm::Value *exitNode,
                                        const llvm::Instruction *reSite,
                                        const llvm::Value *retNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getCallToReturnEdgeFunction(const llvm::Instruction *callSite,
                                              const llvm::Value *callNode,
                                              const llvm::Instruction *retSite,
                                              const llvm::Value *retSiteNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

const llvm::Value *IDETaintAnalysis::topElement() { return nullptr; }

const llvm::Value *IDETaintAnalysis::bottomElement() { return nullptr; }

const llvm::Value *IDETaintAnalysis::join(const llvm::Value *lhs,
                                          const llvm::Value *rhs) {
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::allTopFunction() {
  return make_shared<IDETainAnalysisAllTop>();
}

const llvm::Value *IDETaintAnalysis::IDETainAnalysisAllTop::computeTarget(
    const llvm::Value *source) {
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::IDETainAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> secondFunction) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::IDETainAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> otherFunction) {
  return EdgeIdentity<const llvm::Value *>::v();
}

bool IDETaintAnalysis::IDETainAnalysisAllTop::equalTo(
    shared_ptr<EdgeFunction<const llvm::Value *>> other) {
  return false;
}
