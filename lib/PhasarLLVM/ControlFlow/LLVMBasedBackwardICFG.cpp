/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <memory>

#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "boost/graph/copy.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMM.h"
#include "phasar/Utils/Utilities.h"

using namespace psr;
using namespace std;
namespace psr {

LLVMBasedBackwardsICFG::LLVMBasedBackwardsICFG(LLVMBasedICFG &ICFG)
    : ForwardICFG(ICFG) {
  auto CgCopy = ForwardICFG.CallGraph;
  boost::copy_graph(boost::make_reverse_graph(CgCopy), ForwardICFG.CallGraph);
  createBackwardRets();
}

void LLVMBasedBackwardsICFG::createBackwardRets() {
  for (const auto *Function : ForwardICFG.getAllFunctions()) {
    BackwardRetToFunction[BackwardRets[Function].getInstance()] = Function;
  }
}

bool LLVMBasedBackwardsICFG::isIndirectFunctionCall(
    const llvm::Instruction *Stmt) const {
  return ForwardICFG.isIndirectFunctionCall(Stmt);
}

bool LLVMBasedBackwardsICFG::isVirtualFunctionCall(
    const llvm::Instruction *Stmt) const {
  return ForwardICFG.isVirtualFunctionCall(Stmt);
}

std::set<const llvm::Function *>
LLVMBasedBackwardsICFG::getAllFunctions() const {
  return ForwardICFG.getAllFunctions();
}

const llvm::Function *
LLVMBasedBackwardsICFG::getFunction(const std::string &Fun) const {
  return ForwardICFG.getFunction(Fun);
}

std::set<const llvm::Function *>
LLVMBasedBackwardsICFG::getCalleesOfCallAt(const llvm::Instruction *N) const {
  return ForwardICFG.getCalleesOfCallAt(N);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallersOf(const llvm::Function *M) const {
  return ForwardICFG.getCallersOf(M);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallsFromWithin(const llvm::Function *M) const {
  return ForwardICFG.getCallsFromWithin(M);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getReturnSitesOfCallAt(
    const llvm::Instruction *N) const {
  std::set<const llvm::Instruction *> ReturnSites;
  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(N)) {
    for (const auto *Succ : this->getSuccsOf(Call)) {
      ReturnSites.insert(Succ);
    }
  }
  return ReturnSites;
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::allNonCallStartNodes() const {
  return ForwardICFG.allNonCallStartNodes();
}

const llvm::Function *
LLVMBasedBackwardsICFG::getFunctionOf(const llvm::Instruction *Stmt) const {
  auto BackwardRetIt = BackwardRetToFunction.find(Stmt);
  if (BackwardRetIt != BackwardRetToFunction.end()) {
    return BackwardRetIt->second;
  }
  return Stmt->getFunction();
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getPredsOf(const llvm::Instruction *Stmt) const {
  auto BackwardRetIt = BackwardRetToFunction.find(Stmt);
  if (BackwardRetIt == BackwardRetToFunction.end()) {
    return LLVMBasedBackwardCFG::getPredsOf(Stmt);
  }
  auto ExitPoints =
      LLVMBasedBackwardCFG::getExitPointsOf(BackwardRetIt->second);
  return {ExitPoints.begin(), ExitPoints.end()};
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getSuccsOf(const llvm::Instruction *Stmt) const {
  if (isExitInst(Stmt)) {
    return {};
  }
  std::vector<const llvm::Instruction *> Succs =
      LLVMBasedBackwardCFG::getSuccsOf(Stmt);
  if (Succs.empty()) {
    assert(Stmt->getParent()->getParent() &&
           "Could not find parent of stmt's parent ");
    Succs.push_back(
        BackwardRets.at(Stmt->getParent()->getParent()).getInstance());
  }
  return Succs;
}

bool LLVMBasedBackwardsICFG::isExitInst(const llvm::Instruction *Stmt) const {
  return (Stmt->getParent() == nullptr &&
          BackwardRetToFunction.count(Stmt) > 0);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getExitPointsOf(const llvm::Function *Fun) const {
  return {BackwardRets.at(Fun).getInstance()};
}

void LLVMBasedBackwardsICFG::mergeWith(const LLVMBasedBackwardsICFG &Other) {
  ForwardICFG.mergeWith(Other.ForwardICFG);
}

void LLVMBasedBackwardsICFG::print(std::ostream &OS) const {
  ForwardICFG.print(OS);
}

void LLVMBasedBackwardsICFG::printAsDot(std::ostream &OS) const {
  ForwardICFG.printAsDot(OS);
}

nlohmann::json LLVMBasedBackwardsICFG::getAsJson() const {
  return ForwardICFG.getAsJson();
}

unsigned LLVMBasedBackwardsICFG::getNumOfVertices() {
  return ForwardICFG.getNumOfVertices();
}

unsigned LLVMBasedBackwardsICFG::getNumOfEdges() {
  return ForwardICFG.getNumOfEdges();
}

std::vector<const llvm::Function *>
LLVMBasedBackwardsICFG::getDependencyOrderedFunctions() {
  return ForwardICFG.getDependencyOrderedFunctions();
}

void LLVMBasedBackwardsICFG::collectGlobalCtors() {
  ForwardICFG.collectGlobalCtors();
}

void LLVMBasedBackwardsICFG::collectGlobalDtors() {
  ForwardICFG.collectGlobalDtors();
}

void LLVMBasedBackwardsICFG::collectGlobalInitializers() {
  ForwardICFG.collectGlobalInitializers();
}

void LLVMBasedBackwardsICFG::collectRegisteredDtors() {
  ForwardICFG.collectRegisteredDtors();
}

} // namespace psr
