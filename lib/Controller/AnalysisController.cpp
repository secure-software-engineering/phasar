/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"

#include "phasar/AnalysisStrategy/Strategies.h"
#include "phasar/Controller/AnalysisControllerEmitterOptions.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>
#include <filesystem>
#include <functional>
#include <set>
#include <utility>

namespace psr {

AnalysisController::AnalysisController(
    HelperAnalyses &HA, std::vector<DataFlowAnalysisType> DataFlowAnalyses,
    std::vector<std::string> AnalysisConfigs,
    std::vector<std::string> EntryPoints, AnalysisStrategy Strategy,
    AnalysisControllerEmitterOptions EmitterOptions,
    IFDSIDESolverConfig SolverConfig, std::string ProjectID,
    std::string OutDirectory)
    : HA(HA), DataFlowAnalyses(std::move(DataFlowAnalyses)),
      AnalysisConfigs(std::move(AnalysisConfigs)),
      EntryPoints(std::move(EntryPoints)), Strategy(Strategy),
      EmitterOptions(EmitterOptions), ProjectID(std::move(ProjectID)),
      ResultDirectory(std::move(OutDirectory)), SolverConfig(SolverConfig) {
  if (!ResultDirectory.empty()) {
    // create directory for results
    ResultDirectory /= this->ProjectID + "-" + createTimeStamp();
    std::filesystem::create_directory(ResultDirectory);
  }
  emitRequestedHelperAnalysisResults();
  executeAs(Strategy);
}

void AnalysisController::executeAs(AnalysisStrategy Strategy) {
  switch (Strategy) {
  case AnalysisStrategy::None:
    return;
  case AnalysisStrategy::DemandDriven:
  case AnalysisStrategy::Incremental:
  case AnalysisStrategy::ModuleWise:
  case AnalysisStrategy::Variational:
    llvm::report_fatal_error("AnalysisStrategy not supported, yet!");
    return;
  case AnalysisStrategy::WholeProgram:
    executeWholeProgram();
    return;
  }
  llvm_unreachable(
      "All AnalysisStrategy variants should be handled in the switch above!");
}

void AnalysisController::executeDemandDriven() {}

void AnalysisController::executeIncremental() {}

void AnalysisController::executeModuleWise() {}

void AnalysisController::executeVariational() {}

void AnalysisController::executeWholeProgram() {
  for (auto DataFlowAnalysis : DataFlowAnalyses) {
    switch (DataFlowAnalysis) {
    case DataFlowAnalysisType::None:
      continue;
    case DataFlowAnalysisType::IFDSUninitializedVariables:
      executeIFDSUninitVar();
      continue;
    case DataFlowAnalysisType::IFDSConstAnalysis:
      executeIFDSConst();
      continue;
    case DataFlowAnalysisType::IFDSTaintAnalysis:
      executeIFDSTaint();
      continue;
    case DataFlowAnalysisType::IDEExtendedTaintAnalysis:
      executeIDEXTaint();
      continue;
    case DataFlowAnalysisType::IDEOpenSSLTypeStateAnalysis:
      executeIDEOpenSSLTS();
      continue;
    case DataFlowAnalysisType::IDECSTDIOTypeStateAnalysis:
      executeIDECSTDIOTS();
      continue;
    case DataFlowAnalysisType::IFDSTypeAnalysis:
      executeIFDSType();
      continue;
    case DataFlowAnalysisType::IFDSSolverTest:
      executeIFDSSolverTest();
      continue;
    case DataFlowAnalysisType::IFDSFieldSensTaintAnalysis:
      executeIFDSFieldSensTaint();
      continue;
    case DataFlowAnalysisType::IDELinearConstantAnalysis:
      executeIDELinearConst();
      continue;
    case DataFlowAnalysisType::IDESolverTest:
      executeIDESolverTest();
      continue;
    case DataFlowAnalysisType::IDEInstInteractionAnalysis:
      executeIDEIIA();
      continue;
    case DataFlowAnalysisType::IntraMonoFullConstantPropagation:
      executeIntraMonoFullConstant();
      continue;
    case DataFlowAnalysisType::IntraMonoSolverTest:
      executeIntraMonoSolverTest();
      continue;
    case DataFlowAnalysisType::InterMonoSolverTest:
      executeInterMonoSolverTest();
      continue;
    case DataFlowAnalysisType::InterMonoTaintAnalysis:
      executeInterMonoTaint();
      continue;
    }

    llvm_unreachable("All possible DataFlowAnalysisType variants should be "
                     "handled in the switch above!");
  }
}

void AnalysisController::emitRequestedHelperAnalysisResults() {
  auto WithResultFileOrStdout = [this](const auto &FileName, auto Callback) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + FileName)) {
        Callback(*OFS);
      }
    } else {
      Callback(llvm::outs());
    }
  };

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitIR) {
    WithResultFileOrStdout("/psr-preprocess-ir.ll", [this](auto &OS) {
      HA.getProjectIRDB().emitPreprocessedIR(OS);
    });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsText) {
    WithResultFileOrStdout(
        "/psr-th.txt", [this](auto &OS) { HA.getTypeHierarchy().print(OS); });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsDot) {
    WithResultFileOrStdout("/psr-th.dot", [this](auto &OS) {
      HA.getTypeHierarchy().printAsDot(OS);
    });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsJson) {
    WithResultFileOrStdout("/psr-th.json", [this](auto &OS) {
      HA.getTypeHierarchy().printAsJson(OS);
    });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText) {
    WithResultFileOrStdout("/psr-pta.txt",
                           [this](auto &OS) { HA.getAliasInfo().print(OS); });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) {
    WithResultFileOrStdout("/psr-pta.dot",
                           [this](auto &OS) { HA.getAliasInfo().print(OS); });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) {
    WithResultFileOrStdout("/psr-pta.json", [this](auto &OS) {
      HA.getAliasInfo().printAsJson(OS);
    });
  }

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsDot) {
    WithResultFileOrStdout("/psr-cg.txt",
                           [this](auto &OS) { HA.getICFG().print(OS); });
  }

  if (EmitterOptions &
      (AnalysisControllerEmitterOptions::EmitStatisticsAsJson |
       AnalysisControllerEmitterOptions::EmitStatisticsAsText)) {

    auto &IRDB = HA.getProjectIRDB();
    GeneralStatisticsAnalysis GSA;
    const auto &Stats = GSA.runOnModule(*IRDB.getModule());

    if (EmitterOptions &
        AnalysisControllerEmitterOptions::EmitStatisticsAsText) {
      llvm::outs() << "Module " << IRDB.getModule()->getName() << ":\n";
      llvm::outs() << "> LLVM IR instructions:\t" << IRDB.getNumInstructions()
                   << "\n";
      llvm::outs() << "> Functions:\t\t" << IRDB.getModule()->size() << "\n";
      llvm::outs() << "> Global variables:\t" << IRDB.getModule()->global_size()
                   << "\n";
      llvm::outs() << "> Alloca instructions:\t"
                   << Stats.getAllocaInstructions().size() << "\n";
      llvm::outs() << "> Call Sites:\t\t" << Stats.getFunctioncalls() << "\n";
    }

    if (EmitterOptions &
        AnalysisControllerEmitterOptions::EmitStatisticsAsJson) {
      WithResultFileOrStdout("/psr-IrStatistics.json",
                             [&Stats](auto &OS) { Stats.printAsJson(OS); });
    }
  }
}

LLVMTaintConfig AnalysisController::makeTaintConfig() {
  std::string AnalysisConfigPath =
      !AnalysisConfigs.empty() ? AnalysisConfigs[0] : "";
  return !AnalysisConfigPath.empty()
             ? LLVMTaintConfig(HA.getProjectIRDB(),
                               parseTaintConfig(AnalysisConfigPath))
             : LLVMTaintConfig(HA.getProjectIRDB());
}

} // namespace psr
