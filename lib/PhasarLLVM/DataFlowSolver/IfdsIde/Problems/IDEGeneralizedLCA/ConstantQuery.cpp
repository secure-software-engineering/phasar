/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/ConstantQuery.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"

namespace psr {

IDEGeneralizedLCA::v_t getConstantValue(
    IDESolver<const llvm::Instruction *, const llvm::Value *,
              const llvm::Function *, const llvm::StructType *,
              const llvm::Value *, EdgeValueSet, LLVMBasedICFG> *solver,
    const llvm::Instruction *inst, const llvm::Value *val) {
  EdgeValue ev(val);
  if (!ev.isTop())
    return {ev};
  else
    return solver->resultAt(inst, val);
  // TODO: implement
}

} // namespace psr
