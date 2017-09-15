/*
 * IFDSProtoAnalysis.cpp
 *
 *  Created on: 15.09.2017
 *      Author: philipp
 */

#include "IFDSProtoAnalysis.hh"

IFDSProtoAnalysis::IFDSProtoAnalysis(LLVMBasedICFG &I,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem<const llvm::Instruction *,
                                   const llvm::Value *, const llvm::Function *,
                                   LLVMBasedICFG &>(I),
      EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSProtoAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                         const llvm::Instruction *succ) {
  cout << "IFDSProtoAnalysis::getNormalFlowFunction()\n";
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    return make_shared<Gen<const llvm::Value *>>(
        Store->getPointerOperand(), DefaultIFDSTabulationProblem::zerovalue);
  }
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSProtoAnalysis::getCallFlowFuntion(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd) {
  cout << "IFDSProtoAnalysis::getCallFlowFuntion()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSProtoAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                      const llvm::Function *calleeMthd,
                                      const llvm::Instruction *exitStmt,
                                      const llvm::Instruction *retSite) {
  cout << "IFDSProtoAnalysis::getRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSProtoAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Instruction *retSite) {
  cout << "IFDSProtoAnalysis::getCallToRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSProtoAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                          const llvm::Function *destMthd,
                                          vector<const llvm::Value *> inputs,
                                          vector<bool> context) {
  cout << "IFDSProtoAnalysis::getSummaryFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSProtoAnalysis::initialSeeds() {
  cout << "IFDSProtoAnalysis::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IFDSProtoAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  static ZeroValue *zero = new ZeroValue;
  return zero;
}

bool IFDSProtoAnalysis::isZeroValue(const llvm::Value* d) {
  return isLLVMZeroValue(d);
}

string IFDSProtoAnalysis::D_to_string(const llvm::Value *d) {
  return llvmIRToString(d);
}
