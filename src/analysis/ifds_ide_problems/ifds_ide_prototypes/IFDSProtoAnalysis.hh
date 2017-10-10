/*
 * IFDSProtoAnalysis.hh
 *
 *  Created on: 15.09.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_PROBLEMS_IFDSPROTOANALYSIS_HH_
#define SRC_ANALYSIS_IFDS_IDE_PROBLEMS_IFDSPROTOANALYSIS_HH_

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../../../lib/LLVMShorthands.hh"
#include "../../icfg/LLVMBasedICFG.hh"
#include "../../ifds_ide/DefaultIFDSTabulationProblem.hh"
#include "../../ifds_ide/flow_func/Gen.hh"
#include "../../ifds_ide/flow_func/Kill.hh"
using namespace std;

class IFDSProtoAnalysis : public DefaultIFDSTabulationProblem<
                              const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, LLVMBasedICFG &> {
 private:
  vector<string> EntryPoints;

 public:
  IFDSProtoAnalysis(LLVMBasedICFG &I, vector<string> EntryPoints = {"main"});
  virtual ~IFDSProtoAnalysis() = default;
  shared_ptr<FlowFunction<const llvm::Value *>> getNormalFlowFunction(
      const llvm::Instruction *curr, const llvm::Instruction *succ) override;

  shared_ptr<FlowFunction<const llvm::Value *>> getCallFlowFunction(
      const llvm::Instruction *callStmt,
      const llvm::Function *destMthd) override;

  shared_ptr<FlowFunction<const llvm::Value *>> getRetFlowFunction(
      const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
      const llvm::Instruction *exitStmt,
      const llvm::Instruction *retSite) override;

  shared_ptr<FlowFunction<const llvm::Value *>> getCallToRetFlowFunction(
      const llvm::Instruction *callSite,
      const llvm::Instruction *retSite) override;

  shared_ptr<FlowFunction<const llvm::Value *>> getSummaryFlowFunction(
      const llvm::Instruction *callStmt, const llvm::Function *destMthd,
      vector<const llvm::Value *> inputs, vector<bool> context) override;

  map<const llvm::Instruction *, set<const llvm::Value *>> initialSeeds()
      override;

  const llvm::Value *createZeroValue() override;

  bool isZeroValue(const llvm::Value* d) const override;

  string D_to_string(const llvm::Value *d) override;

  string N_to_string(const llvm::Instruction *n) override;
  
  string M_to_string(const llvm::Function *m) override;
};

#endif /* SRC_ANALYSIS_IFDS_IDE_PROBLEMS_IFDSPROTOANALYSIS_HH_ */
