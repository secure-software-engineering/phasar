#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"

namespace psr {
IDELinearConstantPropagation::v_t
getConstantValue(IDESolver<const llvm::Instruction *, const llvm::Value *,
                           const llvm::Function *, const llvm::StructType *,
                           const llvm::Value *, LCUtils::EdgeValueSet,
                           LLVMBasedICFG> *solver,
                 const llvm::Instruction *inst, const llvm::Value *val);
}
