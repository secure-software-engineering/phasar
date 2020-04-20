#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"

namespace psr {

IDEGeneralizedLCA::v_t getConstantValue(
    IDESolver<const llvm::Instruction *, const llvm::Value *,
              const llvm::Function *, const llvm::StructType *,
              const llvm::Value *, EdgeValueSet, LLVMBasedICFG> *solver,
    const llvm::Instruction *inst, const llvm::Value *val);
}
