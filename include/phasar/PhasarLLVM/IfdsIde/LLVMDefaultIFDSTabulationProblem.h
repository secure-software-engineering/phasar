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
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>

namespace psr {

template <typename D, typename I>
class LLVMDefaultIFDSTabulationProblem : public DefaultIFDSTabulationProblem<const llvm::Instruction *, D, const llvm::Function *, I> {
protected:
  I icfg;
  const LLVMTypeHierarchy &th;
  const ProjectIRDB &irdb;
  virtual D createZeroValue() = 0;
  D zerovalue;

public:
  LLVMDefaultIFDSTabulationProblem(I icfg, const LLVMTypeHierarchy &th, const ProjectIRDB &irdb) : icfg(icfg), th(th), irdb(irdb) {
    this->solver_config.followReturnsPastSeeds = false;
    this->solver_config.autoAddZero = true;
    this->solver_config.computeValues = true;
    this->solver_config.recordEdges = true;
    this->solver_config.computePersistedSummaries = true;
  }

  virtual ~LLVMDefaultIFDSTabulationProblem() = default;

};

} // namespace psr

#endif
