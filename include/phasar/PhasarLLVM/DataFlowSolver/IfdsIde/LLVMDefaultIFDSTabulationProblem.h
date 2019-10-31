/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DefaultIFDSTabulationProblem.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMDEFAULTIFDSTABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMDEFAULTIFDSTABULATIONPROBLEM_H_

#include <memory>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

namespace psr {

template <typename D, typename I>
class LLVMDefaultIFDSTabulationProblem
    : public DefaultIFDSTabulationProblem<const llvm::Instruction *, D,
                                          const llvm::Function *, I> {
protected:
  const LLVMTypeHierarchy &th;
  const ProjectIRDB &irdb;

public:
  LLVMDefaultIFDSTabulationProblem(I icfg, const LLVMTypeHierarchy &th,
                                   const ProjectIRDB &irdb)
      : DefaultIFDSTabulationProblem<const llvm::Instruction *, D,
                                     const llvm::Function *, I>(icfg),
        th(th), irdb(irdb) {}

  ~LLVMDefaultIFDSTabulationProblem() override = default;
};

} // namespace psr

#endif
