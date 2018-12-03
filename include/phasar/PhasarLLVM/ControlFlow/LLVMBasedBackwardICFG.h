/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDICFG_H_

#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/PointsToGraph.h>

namespace llvm {
class Instruction;
class Function;
class Module;
class Instruction;
class BitCastInst;
} // namespace llvm

namespace psr {

class Resolver;
class ProjectIRDB;
class LLVMTypeHierarchy;

class LLVMBasedBackwardsICFG
    : public ICFG<const llvm::Instruction *, const llvm::Function *>,
      public virtual LLVMBasedBackwardCFG {
private:
public:
  LLVMBasedICFG ForwardICFG;
  LLVMBasedBackwardsICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB);

  LLVMBasedBackwardsICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                         CallGraphAnalysisType CGType,
                         const std::vector<std::string> &EntryPoints = {
                             "main"});

  LLVMBasedBackwardsICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                         const llvm::Module &M, CallGraphAnalysisType CGType,
                         std::vector<std::string> EntryPoints = {});

  virtual ~LLVMBasedBackwardsICFG() = default;

  std::set<const llvm::Function *> getAllMethods();

  bool isVirtualFunctionCall(llvm::ImmutableCallSite CS);

  const llvm::Function *getMethod(const std::string &fun) override;

  std::set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *n) override;

  std::set<const llvm::Instruction *>
  getCallersOf(const llvm::Function *m) override;

  std::set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *m) override;

  std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *m) override;

  std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *fun) override;

  std::set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *n) override;

  bool isCallStmt(const llvm::Instruction *stmt) override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() override;

  const llvm::Instruction *getLastInstructionOf(const std::string &name);

  std::vector<const llvm::Instruction *>
  getAllInstructionsOfFunction(const std::string &name);

  void mergeWith(const LLVMBasedBackwardsICFG &other);

  bool isPrimitiveFunction(const std::string &name);

  void print();

  void printAsDot(const std::string &filename);

  void printInternalPTGAsDot(const std::string &filename);

  json getAsJson() override;

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  void exportPATBCJSON();

  PointsToGraph &getWholeModulePTG();

  std::vector<std::string> getDependencyOrderedFunctions();
};

} // namespace psr

#endif
