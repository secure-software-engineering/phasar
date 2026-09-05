#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/GlobalCtorsDtorsModel.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/IR/Module.h"

#include <concepts>

namespace {
constexpr llvm::StringLiteral AllEntryPoints = "__ALL__";

[[nodiscard]] bool
isAllEntryPoints(llvm::ArrayRef<std::string> EntryPoints) noexcept {
  return EntryPoints.size() == 1 && EntryPoints.front() == AllEntryPoints;
}

template <typename FunctionTy,
          std::invocable<const llvm::Function *> LookupFn = psr::IdentityFn>
[[nodiscard]] std::vector<FunctionTy *>
collectAllEntryFunctions(auto AllFunctions, const llvm::Module &Mod,
                         LookupFn Lookup = {}) {
  auto CtorsDtors = psr::GlobalCtorsDtorsModel::collectGlobalCtorsDtors(Mod);

  std::vector<FunctionTy *> Ret;
  Ret.reserve(Mod.size());

  for (const auto *Fun : AllFunctions) {
    if (psr::isExternallyCallable(*Fun) && !CtorsDtors.contains(Fun)) {
      Ret.push_back(Lookup(Fun));
    }
  }

  return Ret;
}

template <typename FunctionTy>
[[nodiscard]] std::vector<FunctionTy *>
collectNamedEntryFunctions(llvm::ArrayRef<std::string> EntryPoints,
                           std::invocable<llvm::StringRef> auto Lookup) {
  std::vector<FunctionTy *> Ret;
  Ret.reserve(EntryPoints.size());

  for (const auto &EntryPoint : EntryPoints) {
    if (auto *F = Lookup(EntryPoint)) {
      Ret.push_back(F);
      continue;
    }

    PHASAR_LOG_LEVEL(WARNING, "Could not retrieve function for entry point '"
                                  << EntryPoint << "'");
  }

  return Ret;
}
} // namespace

bool psr::isExternallyCallable(const llvm::Function &F) noexcept {
  if (F.isDeclaration() || !F.hasName()) {
    return false;
  }

  // The actual definition of an available_externally function lives in a
  // different module; this one is only a copy for the optimizer.
  if (F.hasAvailableExternallyLinkage()) {
    return false;
  }

  // Admits linkonce_odr/weak_odr, under which clang emits the inline and
  // template parts of a library's API.
  if (F.hasLocalLinkage()) {
    return false;
  }

  return F.getVisibility() != llvm::GlobalValue::HiddenVisibility;
}

std::vector<const llvm::Function *>
psr::getEntryFunctions(const LLVMProjectIRDB &IRDB,
                       llvm::ArrayRef<std::string> EntryPoints) {
  if (isAllEntryPoints(EntryPoints)) {
    return collectAllEntryFunctions<const llvm::Function>(
        IRDB.getAllFunctions(), *IRDB.getModule());
  }

  return collectNamedEntryFunctions<const llvm::Function>(
      EntryPoints, IRDBGetFunctionDef(&IRDB));
}

std::vector<const llvm::Function *>
psr::getEntryFunctions(const LLVMBasedICFG &ICF,
                       llvm::ArrayRef<std::string> EntryPoints) {
  if (isAllEntryPoints(EntryPoints)) {
    return collectAllEntryFunctions<const llvm::Function>(
        ICF.getAllFunctions(), *ICF.getIRDB()->getModule());
  }

  return collectNamedEntryFunctions<const llvm::Function>(
      EntryPoints, [&ICF](llvm::StringRef Name) -> const llvm::Function * {
        const auto *F = ICF.getFunction(Name);
        return F && !F->isDeclaration() ? F : nullptr;
      });
}

std::vector<llvm::Function *>
psr::getEntryFunctionsMut(LLVMProjectIRDB &IRDB,
                          llvm::ArrayRef<std::string> EntryPoints) {
  if (isAllEntryPoints(EntryPoints)) {
    return collectAllEntryFunctions<llvm::Function>(
        IRDB.getAllFunctions(), *IRDB.getModule(),
        [&IRDB](const llvm::Function *F) {
          // to avoid const_cast:
          return IRDB.getFunctionDefinition(F->getName());
        });
  }

  return collectNamedEntryFunctions<llvm::Function>(EntryPoints,
                                                    IRDBGetFunctionDef(&IRDB));
}

std::vector<std::string>
psr::getDefaultEntryPoints(const LLVMProjectIRDB &IRDB) {
  if (IRDB.getFunctionDefinition(GlobalCtorsDtorsModel::ModelName)) {
    return {GlobalCtorsDtorsModel::ModelName.str()};
  }
  if (IRDB.getFunctionDefinition("main")) {
    return {"main"};
  }
  return {AllEntryPoints.str()};
}
