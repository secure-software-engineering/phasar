#pragma once

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>
#include <z3++.h>

namespace psr {
class LLVMBasedVariationalICFG
    : public virtual VariationalICFG<const llvm::Instruction *,
                                     const llvm::Function *, z3::expr>,
      public virtual LLVMBasedICFG,
      public virtual LLVMBasedVariationalCFG {
public:
  LLVMBasedVariationalICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB)
      : LLVMBasedICFG(STH, IRDB) {}

  LLVMBasedVariationalICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                           CallGraphAnalysisType CGType,
                           const std::set<std::string> &EntryPoints = {"main"})
      : LLVMBasedICFG(STH, IRDB, CGType, EntryPoints) {}

  LLVMBasedVariationalICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                           const llvm::Module &M, CallGraphAnalysisType CGType,
                           std::set<std::string> EntryPoints = {})
      : LLVMBasedICFG(STH, IRDB, M, CGType, EntryPoints) {}
};
} // namespace psr