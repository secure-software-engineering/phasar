/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/MonoIfds/MonoIFDSSolver.h"
#include "phasar/PhasarLLVM/DataFlow/MonoIfds/Problems/MonoIFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasIterator.h"

#include "AnalysisController.h"
#include "AnalysisControllerInternal.h"

using namespace psr;

void controller::executeMonoIFDSTaint(AnalysisController &Data) {

  FilteredLLVMAliasIterator FAI(Data.HA->getAliasInfo());

  auto Config = makeTaintConfig(Data);
  monoifds::TaintAnalysis TA(&Config, &Data.HA->getUsedGlobals(), &FAI);

  // monoifds::MonoIFDSSolver Solver(&TA, &Data.HA->getICFG());
  // Solver //
  //     .setCGSCCs(&Data.HA->getCGSCCs())
  //     .setFunctionCompressor(&Data.HA->getCompressedFunctions());

  monoifds::MonoIFDSSolver Solver(&TA, *Data.HA);

  {
    std::optional<Timer> MeasureTime;
    if (Data.EmitterOptions &
        AnalysisControllerEmitterOptions::EmitStatisticsAsText) {
      MeasureTime.emplace([](auto Elapsed) {
        llvm::outs() << "MonoIFDSSolver Elapsed: " << hms{Elapsed} << '\n';
      });
    }

    Solver.solve();
  }

  emitRequestedDataFlowResults(Data, Solver);
}
