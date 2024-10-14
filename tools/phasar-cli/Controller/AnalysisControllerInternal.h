/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNAL_H
#define PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNAL_H

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/Utils/ChronoUtils.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Timer.h"

#include "llvm/Support/Compiler.h"

#include "AnalysisController.h"

namespace psr {
template <typename T, typename U> class IDESolver;
} // namespace psr

namespace psr::controller {

LLVM_LIBRARY_VISIBILITY void executeIFDSUninitVar(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIFDSConst(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIFDSTaint(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIFDSType(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIFDSSolverTest(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void
executeIFDSFieldSensTaint(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIDEXTaint(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIDEOpenSSLTS(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIDECSTDIOTS(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIDELinearConst(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIDESolverTest(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeIDEIIA(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void
executeIntraMonoFullConstant(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void
executeIntraMonoSolverTest(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeSparseIFDSTaint(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void
executeInterMonoSolverTest(AnalysisController &Data);
LLVM_LIBRARY_VISIBILITY void executeInterMonoTaint(AnalysisController &Data);

///
/// \brief The maximum length of the CallStrings used in the InterMonoSolver
///
static constexpr unsigned K = 3;

[[nodiscard]] LLVM_LIBRARY_VISIBILITY LLVMTaintConfig
makeTaintConfig(AnalysisController &Data);

template <typename T>
static void statsEmitter(llvm::raw_ostream & /*OS*/, const T & /*Solver*/) {}
template <typename T, typename U>
static void statsEmitter(llvm::raw_ostream &OS, const IDESolver<T, U> &Solver);

template <typename T>
static void emitRequestedDataFlowResults(AnalysisController &Data, T &Solver) {
  auto EmitterOptions = Data.EmitterOptions;
  const auto &ResultDirectory = Data.ResultDirectory;

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTextReport) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-report.txt")) {
        Solver.emitTextReport(*OFS);
      }
    } else {
      Solver.emitTextReport(llvm::outs());
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitGraphicalReport) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-report.html")) {
        Solver.emitGraphicalReport(*OFS);
      }
    } else {
      Solver.emitGraphicalReport(llvm::outs());
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitRawResults) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() +
                                    "/psr-raw-results.txt")) {
        Solver.dumpResults(*OFS);
      }
    } else {
      Solver.dumpResults(llvm::outs());
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitESGAsDot) {
    llvm::outs() << "Front-end support for 'EmitESGAsDot' to be implemented\n";
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitStatisticsAsText) {

    statsEmitter(llvm::outs(), Solver);
  }
}

} // namespace psr::controller

#endif // PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNAL_H
