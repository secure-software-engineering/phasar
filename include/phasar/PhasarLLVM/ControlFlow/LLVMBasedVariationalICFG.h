#pragma once

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>
#include <z3++.h>

namespace psr {
class LLVMBasedVariationalICFG
    : // public virtual LLVMBasedICFG,
      public virtual VariationalICFG<const llvm::Instruction *,
                                     const llvm::Function *, z3::expr>,
      public virtual LLVMBasedVariationalCFG {
  // Cannot inherit from LLVMBasedICFG for whatever reason, so delegate to field
  // icfg
  LLVMBasedICFG icfg;

public:
  LLVMBasedVariationalICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB);

  LLVMBasedVariationalICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                           CallGraphAnalysisType CGType,
                           const std::set<std::string> &EntryPoints = {"main"});

  LLVMBasedVariationalICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                           const llvm::Module &M, CallGraphAnalysisType CGType,
                           std::set<std::string> EntryPoints = {});

#pragma region LLVMBasedICFG
  std::set<const llvm::Function *> getAllFunctions() const override;

  bool isIndirectFunctionCall(const llvm::Instruction *n) const override;

  bool isVirtualFunctionCall(const llvm::Instruction *n) const override;

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

  bool isCallStmt(const llvm::Instruction *stmt) const override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() const override;

  const llvm::Instruction *getLastInstructionOf(const std::string &name);

  std::vector<const llvm::Instruction *>
  getAllInstructionsOfFunction(const std::string &name);

  void mergeWith(const LLVMBasedICFG &other);

  bool isPrimitiveFunction(const std::string &name);

  void print();

  void printAsDot(const std::string &filename);

  void printInternalPTGAsDot(const std::string &filename);

  nlohmann::json getAsJson() const override;

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  void exportPATBCJSON();

  const PointsToGraph &getWholeModulePTG() const;

  std::vector<std::string> getDependencyOrderedFunctions();
#pragma endregion
};
} // namespace psr