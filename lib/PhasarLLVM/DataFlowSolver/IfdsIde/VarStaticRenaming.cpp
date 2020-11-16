#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <type_traits>

namespace psr {
stringstringmap_t extractStaticRenaming(const ProjectIRDB *IRDB) {
  assert(IRDB);
  // in function static_initializer:
  // calls to __static_renaming(new, old)
  stringstringmap_t ret;

  const llvm::Function *renamingFn = IRDB->getFunction("__static_renaming");
  if (!renamingFn)
    return ret;

  for (auto user : renamingFn->users()) {
    if (auto call = llvm::dyn_cast<llvm::CallBase>(user)) {
      constexpr auto conv = [](llvm::Value *op) {
        auto gep = llvm::cast<llvm::ConstantExpr>(op);
        auto gv = llvm::cast<llvm::GlobalVariable>(gep->getOperand(0));
        auto init = llvm::cast<llvm::ConstantDataArray>(gv->getInitializer());
        return init->getAsCString();
      };
      auto newOp = conv(call->getArgOperand(0));
      auto oldOp = conv(call->getArgOperand(1));

      ret[oldOp] = newOp;
    }
  }

  return ret;
}

std::pair<stringstringmap_t, stringstringmap_t>
extractBiDiStaticRenaming(const ProjectIRDB *IRDB) {
  assert(IRDB);

  // Allocate the maps as pair to guarantee the use of RVO
  std::pair<stringstringmap_t, stringstringmap_t> ret;

  const llvm::Function *renamingFn = IRDB->getFunction("__static_renaming");
  if (!renamingFn)
    return ret;

  for (auto user : renamingFn->users()) {
    if (auto call = llvm::dyn_cast<llvm::CallBase>(user)) {
      constexpr auto conv = [](llvm::Value *op) {
        auto gep = llvm::cast<llvm::ConstantExpr>(op);
        auto gv = llvm::cast<llvm::GlobalVariable>(gep->getOperand(0));
        auto init = llvm::cast<llvm::ConstantDataArray>(gv->getInitializer());
        return init->getAsCString();
      };
      auto newOp = conv(call->getArgOperand(0));
      auto oldOp = conv(call->getArgOperand(1));

      ret.first[oldOp] = newOp;
      ret.second[newOp] = oldOp;
    }
  }

  return ret;
}
} // namespace psr