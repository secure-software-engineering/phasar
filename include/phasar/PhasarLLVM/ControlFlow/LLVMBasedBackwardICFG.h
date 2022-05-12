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

#include "llvm/IR/LLVMContext.h"

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/Utils/Soundness.h"

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
  LLVMBasedICFG &ForwardICFG;
  static inline const std::unique_ptr<llvm::LLVMContext> LLVMBackwardRetCTX =
      std::make_unique<llvm::LLVMContext>();

  class LLVMBackwardRet {
  private:
    const llvm::ReturnInst *Instance;

  public:
    LLVMBackwardRet()
        : Instance(llvm::ReturnInst::Create(*LLVMBackwardRetCTX)){};
    [[nodiscard]] const llvm::ReturnInst *getInstance() const {
      return Instance;
    }
  };
  std::unordered_map<const llvm::Function *, LLVMBackwardRet> BackwardRets;
  llvm::DenseMap<const llvm::Instruction *, const llvm::Function *>
      BackwardRetToFunction;

public:
  LLVMBasedBackwardsICFG(LLVMBasedICFG &ICFG);

  ~LLVMBasedBackwardsICFG() override = default;

  std::set<const llvm::Function *> getAllFunctions() const override;

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
  getReturnSitesOfCallAt(const llvm::Instruction *N) const override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() const override;

  [[nodiscard]] const llvm::Function *
  getFunctionOf(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *Fun) const override;

  [[nodiscard]] bool isExitInst(const llvm::Instruction *Stmt) const override;

  void mergeWith(const LLVMBasedBackwardsICFG &Other);

  using LLVMBasedBackwardCFG::print; // tell the compiler we wish to have both
                                     // prints
  void print(llvm::raw_ostream &OS) const override;

  void printAsDot(llvm::raw_ostream &OS) const;

  using LLVMBasedBackwardCFG::getAsJson; // tell the compiler we wish to have
                                         // both prints
  nlohmann::json getAsJson() const override;

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  std::vector<const llvm::Function *> getDependencyOrderedFunctions();

private:
  void createBackwardRets();

protected:
  void collectGlobalCtors() override;

  void collectGlobalDtors() override;

  void collectGlobalInitializers() override;

  void collectRegisteredDtors() override;
};

} // namespace psr

#endif
