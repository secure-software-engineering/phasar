#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/ConstantQuery.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValue.h"

namespace psr {
IDELinearConstantPropagation::v_t
getConstantValue(IDESolver<const llvm::Instruction *, const llvm::Value *,
                           const llvm::Function *, const llvm::StructType *,
                           const llvm::Value *, LCUtils::EdgeValueSet,
                           LLVMBasedICFG> *solver,
                 const llvm::Instruction *inst, const llvm::Value *val) {
  LCUtils::EdgeValue ev(val);
  if (!ev.isTop())
    return {ev};
  else
    return solver->resultAt(inst, val);
  // TODO: implement
}
} // namespace psr
