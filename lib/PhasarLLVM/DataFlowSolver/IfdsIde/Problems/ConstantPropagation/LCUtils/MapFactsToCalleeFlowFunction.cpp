#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/ConstantHelper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/MapFactsToCalleeFlowFunction.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::LCUtils {

MapFactsToCalleeFlowFunction::MapFactsToCalleeFlowFunction(
    llvm::ImmutableCallSite cs, const llvm::Function *destMthd)
    : cs(cs), destMthd(destMthd) {

  for (unsigned idx = 0; idx < cs.getNumArgOperands(); ++idx) {
    actuals.push_back(cs.getArgOperand(idx));
  }
  // Set up the formal parameters
  for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
    auto frm = psr::getNthFunctionArgument(destMthd, idx);
    assert(frm && "Invalid formal");
    formals.push_back(frm);
  }
}
std::set<const llvm::Value *>
MapFactsToCalleeFlowFunction::computeTargets(const llvm::Value *source) {
  // most copied from phasar
  // if (!psr::LLVMZeroValue::getInstance()->isLLVMZeroValue(source)) {
  std::set<const llvm::Value *> res;
  // Handle C-style varargs functions
  if (destMthd->isVarArg()) {
    // Map actual parameter into corresponding formal parameter.
    for (unsigned idx = 0; idx < actuals.size(); ++idx) {
      if (source == actuals[idx] ||
          (psr::LLVMZeroValue::getInstance()->isLLVMZeroValue(source) &&
           psr::LCUtils::isConstant(actuals[idx]))) {
        if (idx >= destMthd->arg_size() && !destMthd->isDeclaration()) {
          // Over-approximate by trying to add the
          //   alloca [1 x %struct.__va_list_tag], align 16
          // to the results
          // find the allocated %struct.__va_list_tag and generate it
          for (auto &BB : *destMthd) {
            for (auto &I : BB) {
              if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                if (Alloc->getAllocatedType()->isArrayTy() &&
                    Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                    Alloc->getAllocatedType()
                        ->getArrayElementType()
                        ->isStructTy() &&
                    Alloc->getAllocatedType()
                            ->getArrayElementType()
                            ->getStructName() == "struct.__va_list_tag") {
                  res.insert(Alloc);
                }
              }
            }
          }
        } else {
          res.insert(formals[idx]); // corresponding formal
        }
      }
    }
    if (psr::LLVMZeroValue::getInstance()->isLLVMZeroValue(source))
      res.insert(source);
    return res;
  } else {
    // Handle ordinary case
    // Map actual parameter into corresponding formal parameter.
    for (unsigned idx = 0; idx < actuals.size(); ++idx) {
      if (source == actuals[idx] ||
          (psr::LLVMZeroValue::getInstance()->isLLVMZeroValue(source) &&
           psr::LCUtils::isConstant(actuals[idx]))) {
        res.insert(formals[idx]); // corresponding formal
        // std::cout << "Map actual to formal: " < < < < std::endl;
        // llvm::outs() << "Map actual " << *actuals[idx] << " to formal "
        //            << *formals[idx] << "\n";
      }
    }
    if (psr::LLVMZeroValue::getInstance()->isLLVMZeroValue(source))
      res.insert(source);
    return res;
  }
}
} // namespace psr::LCUtils