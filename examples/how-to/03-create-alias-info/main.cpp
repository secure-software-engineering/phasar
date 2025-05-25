#include "phasar/PhasarLLVM/DB.h"
#include "phasar/PhasarLLVM/Pointer.h"
#include "phasar/PhasarLLVM/Utils.h"

#include "llvm/IR/InstIterator.h"

#include <cassert>

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: create-alias-info <LLVM-IR file>\n";
    return 1;
  }

  // Load the IR
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  // The easiest way of getting alias information is using the LLVMAliasSet:
  psr::LLVMAliasSet AS(&IRDB, /*UseLazyEvaluation=*/false);

  // PhASAR APIs usually do not care, which alias-info implementation you use.
  // They take a type-erased reference to any alias-info object.
  // You can implicitly convert a pointer to any compatible alias-info object to
  // an LLVMAliasInfoRef.
  // Since the LLVMAliasInfoRef is a non-owning reference, you must make sure
  // that the actual LLVMAliasSet object outlives any use of the references to
  // it.
  psr::LLVMAliasInfoRef ASRef = &AS;

  // You can print and load alias information from/to JSON:
  AS.printAsJson();

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

    // Retrieve the aliases of the result of the instruction Inst (first
    // parameter) at the program location determined by Inst (second parameter).
    //
    // Implementations may ignore the second parameter.
    auto AliasesOfInstAtInst = AS.getAliasSet(&Inst, &Inst);

    llvm::outs() << "For pointer " << psr::llvmIRToString(&Inst) << ":\n";
    for (const auto *Alias : *AliasesOfInstAtInst) {
      llvm::outs() << ">  aliasing " << psr::llvmIRToShortString(Alias) << '\n';

      // You can also check, whether two pointers are (potentially) aliasing:
      assert(AS.alias(&Inst, Alias, &Inst));
    }

    // Retrieve a filtered alias set only containing allocation-sites for the
    // aliases of the result of the instruction Inst (first parameter), further
    // filtered to not contain allocation-sites from other functions (second
    // parameter), at the program location determined by Inst (third
    // parameter).
    //
    // Implementations may ignore the third parameter.
    auto ReachableAllocSites =
        AS.getReachableAllocationSites(&Inst, /*IntraProcOnly=*/true, &Inst);
    for (const auto *AllocSite : *ReachableAllocSites) {
      llvm::outs() << ">  reachable alloc-site "
                   << psr::llvmIRToShortString(AllocSite) << '\n';

      // You can also check, whether a pointer is in the reachable
      // allocation-sites of another pointer:
      assert(AS.isInReachableAllocationSites(&Inst, AllocSite,
                                             /*IntraProcOnly=*/true, &Inst));
    }
    llvm::outs() << '\n';
  }
}
