#include "phasar/PhasarLLVM/VarStaticRenaming.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include <type_traits>

namespace psr {
static llvm::StringRef extractStringFromLLVMValue(const llvm::Value *Op) {
  const auto *GV =
      llvm::cast<llvm::GlobalVariable>(Op->stripPointerCastsAndAliases());
  const auto *Init = llvm::cast<llvm::ConstantDataArray>(GV->getInitializer());
  return Init->getAsCString();
}

stringstringmap_t extractStaticRenaming(const LLVMProjectIRDB *IRDB) {
  assert(IRDB);
  // in function static_initializer:
  // calls to __static_renaming(new, old)
  stringstringmap_t Ret;

  const llvm::Function *RenamingFn = IRDB->getFunction("__static_renaming");
  if (!RenamingFn) {
    return Ret;
  }

  for (const auto *User : RenamingFn->users()) {
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(User)) {
      auto NewOp = extractStringFromLLVMValue(Call->getArgOperand(0));
      auto OldOp = extractStringFromLLVMValue(Call->getArgOperand(1));

      Ret[OldOp] = NewOp;
    }
  }

  return Ret;
}

std::pair<stringstringmap_t, stringstringmap_t>
extractBiDiStaticRenaming(const LLVMProjectIRDB *IRDB) {
  assert(IRDB);

  // Allocate the maps as pair to guarantee the use of RVO
  std::pair<stringstringmap_t, stringstringmap_t> Ret;

  const llvm::Function *RenamingFn = IRDB->getFunction("__static_renaming");
  if (!RenamingFn) {
    return Ret;
  }

  for (const auto *User : RenamingFn->users()) {
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(User)) {

      auto NewOp = extractStringFromLLVMValue(Call->getArgOperand(0));
      auto OldOp = extractStringFromLLVMValue(Call->getArgOperand(1));

      Ret.first[OldOp] = NewOp;
      Ret.second[NewOp] = OldOp;
    }
  }

  return Ret;
}

std::string
getDemangledFunctionName(llvm::StringRef Name,
                         const stringstringmap_t *StaticBackwardRenaming) {
  auto FnName = llvm::demangle(Name.str());

  if (!StaticBackwardRenaming) {
    return FnName;
  }

  if (auto It = StaticBackwardRenaming->find(FnName);
      It != StaticBackwardRenaming->end()) {
    return It->getValue().str();
  }

  return FnName;
}
} // namespace psr