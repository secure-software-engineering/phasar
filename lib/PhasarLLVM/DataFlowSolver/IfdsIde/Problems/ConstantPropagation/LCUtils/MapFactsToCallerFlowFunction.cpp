#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/MapFactsToCallerFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/ConstantHelper.h"

namespace psr::LCUtils {
MapFactsToCallerFlowFunction::MapFactsToCallerFlowFunction(
    llvm::ImmutableCallSite cs, const llvm::Instruction *exitStmt,
    const llvm::Function *calleeMthd)
    : cs(cs), exitStmt(llvm::cast<llvm::ReturnInst>(exitStmt)),
      calleeMthd(calleeMthd) {
  for (unsigned idx = 0; idx < cs.getNumArgOperands(); ++idx) {
    actuals.push_back(cs.getArgOperand(idx));
  }
  // Set up the formal parameters
  for (unsigned idx = 0; idx < calleeMthd->arg_size(); ++idx) {
    formals.push_back(psr::getNthFunctionArgument(calleeMthd, idx));
  }
}
std::set<const llvm::Value *>
MapFactsToCallerFlowFunction::computeTargets(const llvm::Value *source) {
  // most copied from phasar
  std::set<const llvm::Value *> res;
  // Handle C-style varargs functions
  if (calleeMthd->isVarArg() && !calleeMthd->isDeclaration()) {
    const llvm::Instruction *AllocVarArg;
    // Find the allocation of %struct.__va_list_tag
    for (auto &BB : *calleeMthd) {
      for (auto &I : BB) {
        if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
          if (Alloc->getAllocatedType()->isArrayTy() &&
              Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
              Alloc->getAllocatedType()->getArrayElementType()->isStructTy() &&
              Alloc->getAllocatedType()
                      ->getArrayElementType()
                      ->getStructName() == "struct.__va_list_tag") {
            AllocVarArg = Alloc;
            // TODO break out this nested loop earlier (without goto ;-)
          }
        }
      }
    }
    // Generate the varargs things by using an over-approximation
    if (source == AllocVarArg && source->getType()->isPointerTy()) {
      for (unsigned idx = formals.size(); idx < actuals.size(); ++idx) {
        res.insert(actuals[idx]);
      }
    }
  }
  // Handle ordinary case
  // Map formal parameter into corresponding actual parameter.
  for (unsigned idx = 0; idx < formals.size(); ++idx) {
    if (source == formals[idx] && formals[idx]->getType()->isPointerTy()) {
      res.insert(actuals[idx]); // corresponding actual
    }
  }
  // Collect return value facts
  if (source == exitStmt->getReturnValue() ||
      (psr::LLVMZeroValue::getInstance()->isLLVMZeroValue(source) &&
       exitStmt->getReturnValue() && isConstant(exitStmt->getReturnValue()))) {
    res.insert(cs.getInstruction());
  }
  return res;
}
} // namespace psr::LCUtils