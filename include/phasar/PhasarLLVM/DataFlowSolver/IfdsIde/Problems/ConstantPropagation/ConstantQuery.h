#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"

namespace psr {
IDELinearConstantPropagation::v_t getConstantValue(
    psr::IDESolver<const llvm::Instruction *, const llvm::Value *,
                   const llvm::Function *, IDELinearConstantPropagation::v_t,
                   psr::LLVMBasedICFG &> *solver,
    const llvm::Instruction *inst, const llvm::Value *val);
}