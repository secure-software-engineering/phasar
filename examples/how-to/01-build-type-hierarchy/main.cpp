#include "phasar/PhasarLLVM/DB.h"
#include "phasar/PhasarLLVM/TypeHierarchy.h"

#include "llvm/Demangle/Demangle.h"

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: build-type-hierarchy <LLVM-IR file>\n";
    return 1;
  }

  // Load the IR
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  // Build the type hierarchy.
  // Note that this DIBasedTypeHierarchy requires debug information (DI) to be
  // embedded into the LLVM IR. You can achieve this by passing -g to clang.
  psr::DIBasedTypeHierarchy TH(IRDB);

  for (const auto *ClassTy : TH.getAllTypes()) {
    llvm::outs() << "Found class type " << ClassTy->getName() << " ("
                 << TH.getTypeName(ClassTy) << ")\n";
    llvm::outs() << "> demangled name: "
                 << llvm::demangle(TH.getTypeName(ClassTy).str()) << '\n';
  }
  llvm::outs() << '\n';

  // Try to find class 'A'
  const auto *ClassA = TH.getType("_ZTS1A");

  // If TH does not find, it returns nullptr.

  if (ClassA != nullptr) {
    // Get the (transitive) sub-types of ClassA
    for (const auto *ClassTy : TH.subTypesOf(ClassA)) {
      llvm::outs() << "Class " << ClassTy->getName()
                   << " is a (transitive) sub-type of A\n";

      // You can also check, whether a type is a (transitive) sub-type of
      // another type:
      assert(TH.isSubType(ClassA, ClassTy));
    }
  }
}
