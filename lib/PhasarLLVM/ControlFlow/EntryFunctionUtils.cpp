#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Logger.h"

std::vector<const llvm::Function *>
psr::getEntryFunctions(const LLVMProjectIRDB &IRDB,
                       llvm::ArrayRef<std::string> EntryPoints) {
  std::vector<const llvm::Function *> UserEntryPointFns;
  if (EntryPoints.size() == 1 && EntryPoints.front() == "__ALL__") {
    UserEntryPointFns.reserve(IRDB.getNumFunctions());
    // Handle the special case in which a user wishes to treat all functions as
    // entry points.
    for (const auto *Fun : IRDB.getAllFunctions()) {
      // Only functions with external linkage (or 'main') can be called from the
      // outside!
      if (!Fun->isDeclaration() && Fun->hasName() &&
          (Fun->hasExternalLinkage() || Fun->getName() == "main")) {
        UserEntryPointFns.push_back(Fun);
      }
    }
  } else {
    UserEntryPointFns.reserve(EntryPoints.size());
    for (const auto &EntryPoint : EntryPoints) {
      const auto *F = IRDB.getFunctionDefinition(EntryPoint);
      if (F == nullptr) {
        PHASAR_LOG_LEVEL(WARNING,
                         "Could not retrieve function for entry point '"
                             << EntryPoint << "'");
        continue;
      }
      UserEntryPointFns.push_back(F);
    }
  }
  return UserEntryPointFns;
}

[[nodiscard]] std::vector<llvm::Function *>
psr::getEntryFunctionsMut(LLVMProjectIRDB &IRDB,
                          llvm::ArrayRef<std::string> EntryPoints) {
  std::vector<llvm::Function *> UserEntryPointFns;
  if (EntryPoints.size() == 1 && EntryPoints.front() == "__ALL__") {
    UserEntryPointFns.reserve(IRDB.getNumFunctions());
    // Handle the special case in which a user wishes to treat all functions as
    // entry points.
    for (const auto *Fun : IRDB.getAllFunctions()) {
      // Only functions with external linkage (or 'main') can be called from the
      // outside!
      if (!Fun->isDeclaration() && Fun->hasName() &&
          (Fun->hasExternalLinkage() || Fun->getName() == "main")) {
        UserEntryPointFns.push_back(IRDB.getFunction(Fun->getName()));
      }
    }
  } else {
    UserEntryPointFns.reserve(EntryPoints.size());
    for (const auto &EntryPoint : EntryPoints) {
      auto *F = IRDB.getFunctionDefinition(EntryPoint);
      if (F == nullptr) {
        PHASAR_LOG_LEVEL(WARNING,
                         "Could not retrieve function for entry point '"
                             << EntryPoint << "'");
        continue;
      }
      UserEntryPointFns.push_back(F);
    }
  }
  return UserEntryPointFns;
}
