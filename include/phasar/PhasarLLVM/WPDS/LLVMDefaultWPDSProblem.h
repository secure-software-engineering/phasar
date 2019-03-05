/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_LLVMDEFAULTWPDSPROBLEM_H_
#define PHASAR_PHASARLLVM_WPDS_LLVMDEFAULTWPDSPROBLEM_H_

#include <vector>

#include <phasar/PhasarLLVM/WPDS/DefaultWPDSProblem.h>

namespace llvm {
	class Instruction;
	class Function;
} // namespace llvm

namespace psr {

class LLVMTypeHierarchy;
class ProjectIRDB;

template <typename D, typename V, typename I>
class LLVMDefaultWPDSProblem : public DefaultWPDSProblem<const llvm::Instruction *, D, const llvm::Function *, V, I> {
protected:
	const LLVMTypeHierarchy &th;
	const ProjectIRDB &irdb;

public:
  LLVMDefaultWPDSProblem(I ICFG, const LLVMTypeHierarchy &TH, const ProjectIRDB &IRDB, WPDSType WPDS, SearchDirection Direction,
              std::vector<const llvm::Instruction *> Stack = {}, bool Witnesses = false)
      : DefaultWPDSProblem<const llvm::Instruction *, D, const llvm::Function *, V, I>(ICFG, WPDS, Direction, move(Stack), Witnesses), th(TH), irdb(IRDB) {}
  ~LLVMDefaultWPDSProblem() override = default;
};

} // namespace psr

#endif
