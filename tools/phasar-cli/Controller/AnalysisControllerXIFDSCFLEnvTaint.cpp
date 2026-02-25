/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/CFLFieldSensIFDSProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/IO.h"

#include "llvm/Support/raw_ostream.h"

#include "AnalysisController.h"
#include "AnalysisControllerEmitterOptions.h"
#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeIFDSCFLEnvTaint(AnalysisController &Data) {
  auto Config = makeTaintConfig(Data);

  auto UserProblem = createAnalysisProblem<IFDSTaintAnalysis>(
      *Data.HA, &Config, Data.EntryPoints, /*TaintMainArgs*/ false,
      /*EnableStrongUpdateStore*/ false);
  auto Printer = UserProblem.consumePrinter();
  auto FieldSensProblem = CFLFieldSensIFDSProblem(&UserProblem);

  IterativeIDESolver Solver(&FieldSensProblem, &Data.HA->getICFG());

  SimpleTimer MeasureTime;

  auto Results = Solver.solve();

  if (Data.EmitterOptions &
      AnalysisControllerEmitterOptions::EmitStatisticsAsText) {

    llvm::outs() << "Elapsed: " << MeasureTime.elapsed() << '\n';
  }

  // emitRequestedDataFlowResults(Data, Solver);

  // TODO: Once we have properly migrated IterativeIDESolver into phasar-cli, we
  // should use emitRequestedDataFlowResults here, instead of hand-rolling the
  // output!

  const auto WithOutStream = [&Data,
                              HasResultsDir = !Data.ResultDirectory.empty()](
                                 const llvm::Twine &FileName, auto Handler) {
    if (HasResultsDir) {
      if (auto OFS = openFileStream(Data.ResultDirectory.string() + FileName)) {
        Handler(*OFS);
      }
    } else {
      Handler(llvm::outs());
    }
  };

  using enum AnalysisControllerEmitterOptions;

  auto EmitterOptions = Data.EmitterOptions;

  if (EmitterOptions & EmitTextReport) {
    EmitterOptions &= ~EmitTextReport;
    WithOutStream("/psr-report.txt", [&](llvm::raw_ostream &OS) {
      Printer->onInitialize();
      bool HasResults = false;

      cfl_fieldsens::filterFieldSensFacts(
          Results, UserProblem.Leaks, [&](auto Inst, auto Fact) {
            HasResults = true;
            Printer->onResult(Inst, Fact,
                              DataFlowAnalysisType::IFDSCFLEnvTaintAnalysis);
          });

      Printer->onFinalize(OS);
      if (!HasResults) {
        OS << "No leaks found!\n";
      }
    });
  }

  if (EmitterOptions & EmitRawResults) {
    EmitterOptions &= ~EmitRawResults;
    WithOutStream("/psr-raw-results.txt", [&](llvm::raw_ostream &OS) {
      OS << Config << '\n';
      Solver.dumpResults(OS);
    });
  }

  if (EmitterOptions != AnalysisControllerEmitterOptions{}) {
    llvm::errs() << "Some emit-*** options may be ignored, because they have "
                    "not been implemented yet for ifds-fieldsens-taint";
  }
}
