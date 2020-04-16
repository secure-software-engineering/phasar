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

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/Utils/SoundnessFlag.h"

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
class LLVMPointsToGraph;

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

  bool isCallStmt(const llvm::Instruction *Stmt) const override;

  bool isIndirectFunctionCall(const llvm::Instruction *Stmt) const override;

  bool isVirtualFunctionCall(const llvm::Instruction *Stmt) const override;

  const llvm::Function *getFunction(const std::string &Fun) const override;

  std::set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *N) const override;

  std::set<const llvm::Instruction *>
  getCallersOf(const llvm::Function *M) const override;

  std::set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *M) const override;

  std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *M) const override;

  std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *Fun) const override;

  std::set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *N) const override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() const override;

  const llvm::Instruction *getLastInstructionOf(const std::string &Name);

  std::vector<const llvm::Instruction *>
  getAllInstructionsOfFunction(const std::string &Name);

  void mergeWith(const LLVMBasedBackwardsICFG &other);

  bool isPrimitiveFunction(const std::string &Name);

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

  const LLVMPointsToGraph &getWholeModulePTG() const;

  std::vector<const llvm::Function *> getDependencyOrderedFunctions();
};

} // namespace psr

#endif
