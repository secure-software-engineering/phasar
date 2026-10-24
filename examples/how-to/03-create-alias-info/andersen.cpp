#include "phasar/PhasarLLVM/ControlFlow.h"
#include "phasar/PhasarLLVM/DB.h"
#include "phasar/PhasarLLVM/Pointer.h"
#include "phasar/PhasarLLVM/Utils.h"

#include "llvm/IR/InstIterator.h"

#include <cassert>

int main(int Argc, char *Argv[]) {
  using namespace std::string_literals;
  if (Argc < 2) {
    llvm::errs() << "USAGE: create-alias-info-andersen <LLVM-IR file>\n";
    return 1;
  }

  // Load the IR
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  // Mapping the entry-points (here, just the main function) to LLVM IR:
  auto Entrypoints = psr::getEntryFunctions(IRDB, {"main"s});

  // Computing the Andersen-stale alias information.
  auto Aliases = psr::computeAndersenOTF(IRDB, Entrypoints);

  // The Andersen alias result is compatible with the LLVMAliasIteratorRef
  // interface.
  psr::LLVMAliasIteratorRef AIt = &Aliases;

  const auto *MainF = IRDB.getFunctionDefinition("main");
  if (!MainF) {
    llvm::errs() << "Required function 'main' not found\n";
    return 1;
  }

  // Manually printing the alias sets:

  for (const auto &Inst : llvm::instructions(MainF)) {
    if (!Inst.getType()->isPointerTy()) {
      // For aliasing, we only care about pointers...
      continue;
    }

    llvm::outs() << "For pointer " << psr::llvmIRToString(&Inst) << ":\n";

    // Iterate over the aliases of the result of the instruction Inst (first
    // parameter) at the program location determined by Inst (second parameter).
    //
    // Implementations may ignore the second parameter.
    Aliases.forallAliasesOf(&Inst, &Inst, [&](const llvm::Value *Alias) {
      llvm::outs() << ">  aliasing " << psr::llvmIRToShortString(Alias) << '\n';

      // You can also check, whether two pointers are (potentially) aliasing:
      assert(Aliases.mayAlias(&Inst, Alias, &Inst));
    });

    llvm::outs() << '\n';
  }
}
