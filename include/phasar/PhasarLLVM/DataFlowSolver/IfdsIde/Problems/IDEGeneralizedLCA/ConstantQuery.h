/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_CONSTANTQUERY_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_CONSTANTQUERY_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"

namespace psr {

IDEGeneralizedLCA::l_t getConstantValue(
    IDESolver<const llvm::Instruction *, const llvm::Value *,
              const llvm::Function *, const llvm::StructType *,
              const llvm::Value *, EdgeValueSet, LLVMBasedICFG> *solver,
    const llvm::Instruction *inst, const llvm::Value *val);
}

#endif
