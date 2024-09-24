
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariablesIndexed.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include <filesystem>
#include <llvm-14/llvm/Support/raw_ostream.h>

using namespace psr;


int main(int Argc, const char **Argv) {
  using namespace std::string_literals;
  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "ifds uninitialized Variables Test\n"
                    "A changed Version of the one in phasar-cli to include aggregate types\n\n"
                    "Usage: myphasartool <LLVM IR file>\n";
    return 1;
  }

  llvm::errs() << "Starting uninitialized Variables Test\n";

  std::vector EntryPoints = {"main"s};

  HelperAnalyses HA(Argv[1], EntryPoints);
  if (!HA.getProjectIRDB().isValid()) {
    return 1;
  }
  llvm::errs() << "HelperAnalyses initialized and valid \n";

  if (const auto *F = HA.getProjectIRDB().getFunctionDefinition("main")) {
    auto L = createAnalysisProblem<IFDSUninitializedVariablesIndexed>(HA, EntryPoints);
    llvm::errs() << "Problem initialized \n";
    IFDSSolver S(L, &HA.getICFG());
    llvm::errs() << "Solver initialized \n";
    auto IFDSResults = S.solve();
    llvm::errs() << "Problem Sovled \n";
    L.emitTextReport(IFDSResults, llvm::errs());
  } else {
    llvm::errs() << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}