#include "phasar/PhasarLLVM/DB.h"     // For LLVMProjectIRDB
#include "phasar/PhasarLLVM/Passes.h" // For GeneralStatisticsAnalysis
#include "phasar/PhasarLLVM/Utils.h"  // For llvmIRToString()

#include "llvm/IR/InstIterator.h" // For llvm::instructions()

static void printIRStats(psr::LLVMProjectIRDB &IRDB);

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: load-llvm-ir <LLVM-IR file>\n";
    return 1;
  }

  // The LLVMProjectIRDB loads and manages an LLVM-IR module.
  // You can load both .ll (human readable) and .bc (smaller, faster load-times)
  // files.
  // If you already have an llvm::Module*, you can also pass it here.
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    // If phasar yould not load the IR, we should exit.
    // Phasar has already printed an error message to the terminal.
    return 1;
  }

  // ========
  // Now, you can work with the module

  printIRStats(IRDB);

  // Inspect the module (see also llvm-hello-world)

  auto *F = IRDB.getFunctionDefinition("main");
  if (!F) {
    llvm::errs() << "error: could not find function 'main'\n";
    return 1;
  }

  llvm::outs() << "--------------- Instructions of 'main' ---------------\n";

  for (const auto &Inst : llvm::instructions(F)) {
    // Phasar annotates all instructions (and global variables) with IRDB-wide
    // unique integer Ids:
    auto InstId = IRDB.getInstructionId(&Inst);

    llvm::outs() << '#' << InstId << ": " << psr::llvmIRToString(&Inst) << '\n';

    // TODO: Analyze instruction 'Inst' here.
  }
}

static void printIRStats(psr::LLVMProjectIRDB &IRDB) {
  psr::GeneralStatisticsAnalysis Stats;
  llvm::outs() << Stats.runOnModule(*IRDB.getModule()) << '\n';
}
