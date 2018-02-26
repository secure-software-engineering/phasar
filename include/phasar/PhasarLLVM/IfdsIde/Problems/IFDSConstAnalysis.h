/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSConstAnalysis.h
 *
 *  Created on: 07.06.2017
 *      Author: rleer
 */
#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_CONST_ANALYSIS_IFDSCONSTANALYSIS_H_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_CONST_ANALYSIS_IFDSCONSTANALYSIS_H_

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultSeeds.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenAll.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/SpecialSummaries.h>
#include <phasar/PhasarLLVM/IfdsIde/ZeroValue.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
using namespace std;

class IFDSConstAnalysis : public DefaultIFDSTabulationProblem<
                              const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, LLVMBasedICFG &> {
 private:
  PointsToGraph &ptg;
  //  IFDSSummaryPool<const llvm::Value *, const llvm::Instruction *> dynSum;
  vector<string> EntryPoints;
  set<const llvm::Value *> storedOnce;

 public:
  IFDSConstAnalysis(LLVMBasedICFG &icfg, vector<string> EntryPoints = {"main"});

  virtual ~IFDSConstAnalysis() = default;

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
      const llvm::Instruction *callStmt,
      const llvm::Function *destMthd) override;

  map<const llvm::Instruction *, set<const llvm::Value *>> initialSeeds()
      override;

  const llvm::Value *createZeroValue() override;

  bool isZeroValue(const llvm::Value *d) const override;

  string DtoString(const llvm::Value *d) override;

  string NtoString(const llvm::Instruction *n) override;

  string MtoString(const llvm::Function *m) override;

  void printInitilizedSet();
};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_CONST_ANALYSIS_IFDSCONSTANALYSIS_H_ \
        */
