/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSTypeAnalysis.h
 *
 *  Created on: 06.12.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TYPE_ANALYSIS_IFDSTYPEANALYSIS_H_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TYPE_ANALYSIS_IFDSTYPEANALYSIS_H_

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <map>
#include <memory>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultSeeds.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <set>
#include <string>
using namespace std;
class IFDSTypeAnalysis : public DefaultIFDSTabulationProblem<
                             const llvm::Instruction *, const llvm::Value *,
                             const llvm::Function *, LLVMBasedICFG &> {
private:
  vector<string> EntryPoints;

public:
  IFDSTypeAnalysis(LLVMBasedICFG &icfg, vector<string> EntryPoints = {"main"});

  virtual ~IFDSTypeAnalysis() = default;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getCallFlowFunction(const llvm::Instruction *callStmt,
                      const llvm::Function *destMthd) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeMthd,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite) override;

  map<const llvm::Instruction *, set<const llvm::Value *>>
  initialSeeds() override;

  const llvm::Value *createZeroValue() override;

  bool isZeroValue(const llvm::Value *d) const override;

  string DtoString(const llvm::Value *d) const override;

  string NtoString(const llvm::Instruction *n) const override;

  string MtoString(const llvm::Function *m) const override;
};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TYPE_ANALYSIS_IFDSTYPEANALYSIS_HH_   \
        */
