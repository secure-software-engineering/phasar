/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <filesystem>
#include <fstream>
#include <llvm/IR/Instructions.h>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

namespace llvm {
class Value;
} // namespace llvm

using namespace psr;

int main(int Argc, const char **Argv) {
  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "myphasartool\n"
                    "A small PhASAR-based example program\n\n"
                    "Usage: myphasartool <LLVM IR file>\n";
    return 1;
  }
  ProjectIRDB DB({Argv[1]});
  if (const auto *F = DB.getFunctionDefinition("main")) {
    LLVMTypeHierarchy H(DB);
    LLVMPointsToSet P(DB);
    LLVMBasedICFG I(DB, CallGraphAnalysisType::OTF, {"main"}, &H, &P);
    // IFDS template parametrization test
    llvm::outs() << "Testing IFDS:\n";
    TaintConfig TConfig(DB);
    IFDSTaintAnalysis TA(&DB, &H, &I, &P, TConfig, {"main"});
    IFDSSolver S(TA);
    S.solve();
    S.dumpResults();
    llvm::outs() << "Submit further seeds!\n";
    InitialSeeds<IFDSTaintAnalysis::n_t, IFDSTaintAnalysis::d_t,
                 IFDSTaintAnalysis::l_t>
        FurtherSeeds;
    FurtherSeeds.addSeed(getNthInstruction(F, 4), getNthInstruction(F, 4));
    S.submitFurtherSeedsAndSolve(FurtherSeeds);
    S.dumpResults();
  } else {
    llvm::errs() << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
