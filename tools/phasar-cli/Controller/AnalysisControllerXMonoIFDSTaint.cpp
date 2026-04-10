/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CGSCCs.h"
#include "phasar/DataFlow/MonoIfds/MonoIFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/FunctionCompressor.h"
#include "phasar/PhasarLLVM/DataFlow/MonoIfds/Problems/MonoIFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/CachedLLVMAliasIterator.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasIterator.h"
#include "phasar/PhasarLLVM/Utils/UsedGlobals.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeMonoIFDSTaint(AnalysisController &Data) {
  auto Config = makeTaintConfig(Data);

  auto &IRDB = Data.HA->getProjectIRDB();
  auto &ICF = Data.HA->getICFG();

  const auto &CG = ICF.getCallGraph();
  auto FC =
      compressFunctions(CG, psr::getEntryFunctions(IRDB, Data.EntryPoints));

  auto SCCs = computeCGSCCs(CG, ICF, FC);
  auto SCCCallers = computeCGSCCCallers(CG, ICF, FC, SCCs);

  auto UsedGlobals = computeUsedGlobals(IRDB, FC, SCCs, SCCCallers);

  auto AI = Data.HA->getAliasInfo();
  FilteredLLVMAliasIterator FAI(AI);
  CachedLLVMAliasIterator CAI(&FAI);
  monoifds::TaintAnalysis TA(&Config, &UsedGlobals, &CAI);
  monoifds::MonoIFDSSolver Solver(&TA, &ICF);
  Solver.setCGSCCs(&SCCs).setFunctionCompressor(&FC);

  {
    std::optional<Timer> MeasureTime;
    if (Data.EmitterOptions &
        AnalysisControllerEmitterOptions::EmitStatisticsAsText) {
      MeasureTime.emplace([](auto Elapsed) {
        llvm::outs() << "Elapsed: " << hms{Elapsed} << '\n';
      });
    }

    Solver.solve();
  }

  emitRequestedDataFlowResults(Data, Solver);
}
