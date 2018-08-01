/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 *
 * LLVMBasedBackwardsICFG.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt


#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDICFG_H_

#include <phasar/PhasarLLVM/ControFlow/BiDiICFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <set>
#include <vector>

class LLVMBasedBackwardsICFG : public BiDiICFG<const llvm::Instruction*, const
llvm::Function*> {
private:


public:
        LLVMBasedBackwardsICFG();

        virtual ~LLVMBasedBackwardsICFG();

        //swapped
        std::vector<const llvm::Instruction*> getSuccsOf(const llvm::Instruction* n)
override;

        //swapped
        std::set<const llvm::Instruction*> getStartPointsOf(const llvm::Function* m)
override;

        //swapped
        std::set<const llvm::Instruction*> getReturnSitesOfCallAt(const
llvm::Instruction* n) override;

        //swapped
        bool isExitStmt(const llvm::Instruction* stmt) override;

        //swapped
        bool isStartPoint(const llvm::Instruction* stmt) override;

        //swapped
        std::set<const llvm::Instruction*> allNonCallStartNodes() override;

        //swapped
        std::vector<const llvm::Instruction*> getPredsOf(const llvm::Instruction* u)
override;

        //swapped
        std::set<const llvm::Instruction*> getEndPointsOf(const llvm::Function* m)
override;

        //swapped
        std::vector<const llvm::Instruction*> getPredsOfCallAt(const
llvm::Instruction* u) override;

        //swapped
        std::set<const llvm::Instruction*> allNonCallEndNodes() override;

        //same
        const llvm::Function* getMethodOf(const llvm::Instruction* n) override;

        //same
        std::set<const llvm::Function*> getCalleesOfCallAt(const llvm::Instruction*
n) override;

        //same
        std::set<const llvm::Instruction*> getCallersOf(const llvm::Function* m)
override;

        //same
        std::set<const llvm::Instruction*> getCallsFromWithin(const llvm::Function*
m) override;

        //same
        bool isCallStmt(const llvm::Instruction* stmt) override;

        //same
        //DirectedGraph<const llvm::Instruction*> getOrCreateUnitGraph(const
llvm::Function* m) override;

        //same
        std::vector<const llvm::Instruction*> getParameterRefs(const llvm::Function*
m) override;

        bool isFallThroughSuccessor(const llvm::Instruction* stmt, const
llvm::Instruction* succ) override;

        bool isBranchTarget(const llvm::Instruction* stmt, const
llvm::Instruction* succ) override;

        //swapped
        bool isReturnSite(const llvm::Instruction* n) override;

        // same
        bool isReachable(const llvm::Instruction* u) override;

};


#endif

*/
