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

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/Utils/SoundnessFlag.h>

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
class PointsToGraph;

class LLVMBasedBackwardsICFG
    : public ICFG<const llvm::Instruction *, const llvm::Function *>,
      public virtual LLVMBasedBackwardCFG {
private:
  LLVMBasedICFG ForwardICFG;

public:
  LLVMBasedBackwardsICFG(LLVMBasedICFG &ICFG);

  LLVMBasedBackwardsICFG(ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                         const std::set<std::string> &EntryPoints = {},
                         LLVMTypeHierarchy *TH = nullptr,
                         LLVMPointsToInfo *PT = nullptr,
                         SoundnessFlag SF = SoundnessFlag::SOUNDY);

  ~LLVMBasedBackwardsICFG() override = default;

  std::set<const llvm::Function *> getAllFunctions() const override;

  bool isCallStmt(const llvm::Instruction *stmt) const override;

  bool isIndirectFunctionCall(const llvm::Instruction *stmt) const override;

  bool isVirtualFunctionCall(const llvm::Instruction *stmt) const override;

  const llvm::Function *getFunction(const std::string &fun) const override;

  std::set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *n) const override;

  std::set<const llvm::Instruction *>
  getCallersOf(const llvm::Function *m) const override;

  std::set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *m) const override;

  std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *m) const override;

  std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *fun) const override;

  std::set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *n) const override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() const override;

  const llvm::Instruction *getLastInstructionOf(const std::string &name);

  std::vector<const llvm::Instruction *>
  getAllInstructionsOfFunction(const std::string &name);

  void mergeWith(const LLVMBasedBackwardsICFG &other);

  bool isPrimitiveFunction(const std::string &name);

  using LLVMBasedBackwardCFG::print; // tell the compiler we wish to have both
                                     // prints
  void print(std::ostream &OS) const override;

  void printAsDot(std::ostream &OS) const;

  void printInternalPTGAsDot(std::ostream &OS) const;

  using LLVMBasedBackwardCFG::getAsJson; // tell the compiler we wish to have
                                         // both prints
  nlohmann::json getAsJson() const override;

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  const PointsToGraph &getWholeModulePTG() const;

  std::vector<const llvm::Function *> getDependencyOrderedFunctions();
};

} // namespace psr

#endif
