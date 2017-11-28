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
    set<const llvm::Value*> pointsToSet = ptg.getPointsToSet(pointerOp);
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING POINTS-TO SET OF POINTER OPERATOR";
    for(auto stmt : pointsToSet) {
      stmt->dump();
    }
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING POINTS-TO SET OF VALUE OPERATOR";
    set<const llvm::Value *> pointsToSetValue = ptg.getPointsToSet(store->getValueOperand());
    for(auto stmt : pointsToSetValue) {
      stmt->dump();
    }
    // check if the pointerOp is global variable; if so, generate a new data flow fact
    // since global variables are always initialized and after the first store inst.
    // considered to be mutable
    if (auto Global = llvm::dyn_cast<llvm::GlobalValue>(pointerOp)) {
      BOOST_LOG_SEV(lg, DEBUG) << "FOUND GLOBAL";
      return make_shared<Gen<const llvm::Value *>>(Global, DefaultIFDSTabulationProblem::zerovalue);
    } else {
      // check if it's a variables second store; if so, generate a new data flow fact
      if (pointsToSet.size() == 1 && storedOnce.find(pointerOp) != storedOnce.end()) {
        return make_shared<Gen<const llvm::Value *>>(pointerOp, DefaultIFDSTabulationProblem::zerovalue);
      } else {
        for (auto value : pointsToSet) {
          if (storedOnce.find(value) != storedOnce.end()) {
            return make_shared<GenAll<const llvm::Value *>>(pointsToSet, DefaultIFDSTabulationProblem::zerovalue);
          }
        }
      }
      // put the pointer operator into storedOnce set
      storedOnce.insert(pointerOp);
    }
  }
  if (const llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    BOOST_LOG_SEV(lg, DEBUG) << "PRINTING POINTS-TO SET OF LOAD INSTRUCTION";
    set<const llvm::Value *> loadPointsToSet = ptg.getPointsToSet(load->getPointerOperand());
    for(auto stmt : loadPointsToSet) {
      stmt->dump();
    }
  }
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                       const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getCallFlowFunction()";

  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                      const llvm::Function *calleeMthd,
                                      const llvm::Instruction *exitStmt,
                                      const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSConstAnalysis::getRetFlowFunction()";

  return Identity<const llvm::Value *>::v();
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

void IFDSConstAnalysis::
printInitilizedSet() {
  cout << "PRINTING INIT SET" << '\n';
  for(auto stmt : IFDSConstAnalysis::storedOnce) {
    stmt->dump();
  }
}
