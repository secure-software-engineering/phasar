/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

#include <filesystem>
#include <string>

using namespace psr;

int main(int Argc, const char **Argv) {
  using namespace std::string_literals;

  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "myphasartool\n"
                    "A small PhASAR-based example program\n\n"
                    "Usage: myphasartool <LLVM IR file>\n";
    return 1;
  }

  std::vector EntryPoints = {"main"s};

  HelperAnalyses HA(Argv[1], EntryPoints);
  DIBasedTypeHierarchy Test(HA.getProjectIRDB());
  // TODO: alle type_hierarchy tests printen.
  // Dann myphasartool reverten
  // git checkout von myphasartool damit nicht noch ne Änderung dazukommt
  std::string Start = "../../test/llvm_test_code/type_hierarchies/"
                      "type_hierarchy_";
  std::string End = "_cpp_dbg.ll";

  for (int I = 1; I <= 18; I++) {
    HelperAnalyses Curr(Start + std::to_string(I) + End, EntryPoints);
    DIBasedTypeHierarchy CurrDI(Curr.getProjectIRDB());
    llvm::outs() << "\n--------------------------------------------\n"
                 << Start + std::to_string(I) + End
                 << "\n--------------------------------------------\n";
    llvm::outs().flush();
    CurrDI.print();
  }

  HelperAnalyses Twenty(Start + std::to_string(20) + End, EntryPoints);
  DIBasedTypeHierarchy Curr1DI(Twenty.getProjectIRDB());
  llvm::outs() << "\n--------------------------------------------\n"
               << Start + std::to_string(20) + End
               << "\n--------------------------------------------\n";
  llvm::outs().flush();
  Curr1DI.print();
  HelperAnalyses Curr2(Start + std::to_string(21) + End, EntryPoints);
  DIBasedTypeHierarchy Curr2DI(Curr2.getProjectIRDB());
  llvm::outs() << "\n--------------------------------------------\n"
               << Start + std::to_string(21) + End << "\n"
               << "\n--------------------------------------------\n";
  llvm::outs().flush();
  Curr2DI.print();

  llvm::outs() << "All done\n";
  llvm::outs().flush();

  return 0;
}
