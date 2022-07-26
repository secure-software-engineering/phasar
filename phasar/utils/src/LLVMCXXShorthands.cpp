#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"

#include "phasar/Utils/LLVMCXXShorthands.h"
#include "phasar/Utils/Logger.h"

namespace psr {
// Setting up the vtable is counted towards the initialization of an
// object - the object stays immutable.
// To identifiy such a store instruction, we need to check the stored
// value, which is of i32 (...)** type, e.g.
//   i32 (...)** bitcast (i8** getelementptr inbounds ([3 x i8*], [3 x i8*]*
//                           @_ZTV5Child, i32 0, i32 2) to i32 (...)**)
//
// WARNING: The VTT could also be stored, which would make this analysis
// fail
bool isTouchVTableInst(const llvm::StoreInst *Store) {
  if (const auto *CE =
          llvm::dyn_cast<llvm::ConstantExpr>(Store->getValueOperand())) {
    // llvm::ConstantExpr *CE = const_cast<llvm::ConstantExpr *>(ConstCE);
    auto *CEInst = CE->getAsInstruction();
    if (auto *CF = llvm::dyn_cast<llvm::ConstantExpr>(CEInst->getOperand(0))) {
      auto *CFInst = CF->getAsInstruction();
      if (auto *VTable =
              llvm::dyn_cast<llvm::GlobalVariable>(CFInst->getOperand(0))) {
        if (VTable->hasName() &&
            llvm::demangle(VTable->getName().str()).find("vtable") !=
                std::string::npos) {
          PHASAR_LOG_LEVEL(
              DEBUG, "Store Instruction sets up or updates vtable - ignored!");
          CEInst->deleteValue();
          CFInst->deleteValue();
          return true;
        }
      }
      CFInst->deleteValue();
    }
    CEInst->deleteValue();
  } /* end vtable set-up instruction */
  return false;
}
} // namespace psr
