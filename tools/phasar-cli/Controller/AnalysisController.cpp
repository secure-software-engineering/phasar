/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "AnalysisController.h"

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "AnalysisControllerInternal.h"

namespace psr {

static void
emitRequestedHelperAnalysisResults(AnalysisController::ControllerData &Data) {
  auto WithResultFileOrStdout = [&ResultDirectory = Data.ResultDirectory](
                                    const auto &FileName, auto Callback) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + FileName)) {
        Callback(*OFS);
      }
    } else {
      Callback(llvm::outs());
    }
  };

  auto EmitterOptions = Data.EmitterOptions;
  auto &HA = *Data.HA;

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitIR) {
    WithResultFileOrStdout("/psr-preprocess-ir.ll", [&HA](auto &OS) {
      HA.getProjectIRDB().emitPreprocessedIR(OS);
    });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsText) {
    WithResultFileOrStdout(
        "/psr-th.txt", [&HA](auto &OS) { HA.getTypeHierarchy().print(OS); });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsDot) {
    WithResultFileOrStdout("/psr-th.dot", [&HA](auto &OS) {
      HA.getTypeHierarchy().printAsDot(OS);
    });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsJson) {
    WithResultFileOrStdout("/psr-th.json", [&HA](auto &OS) {
      HA.getTypeHierarchy().printAsJson(OS);
    });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText) {
    WithResultFileOrStdout("/psr-pta.txt",
                           [&HA](auto &OS) { HA.getAliasInfo().print(OS); });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) {
    WithResultFileOrStdout("/psr-pta.dot",
                           [&HA](auto &OS) { HA.getAliasInfo().print(OS); });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) {
    WithResultFileOrStdout("/psr-pta.json", [&HA](auto &OS) {
      HA.getAliasInfo().printAsJson(OS);
    });
  }

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsDot) {
    WithResultFileOrStdout("/psr-cg.txt",
                           [&HA](auto &OS) { HA.getICFG().print(OS); });
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsJson) {
    WithResultFileOrStdout("/psr-cg.json",
                           [&HA](auto &OS) { OS << HA.getICFG().getAsJson(); });
  }

  if (EmitterOptions &
      (AnalysisControllerEmitterOptions::EmitStatisticsAsJson |
       AnalysisControllerEmitterOptions::EmitStatisticsAsText)) {

    auto &IRDB = HA.getProjectIRDB();
    GeneralStatisticsAnalysis GSA;
    const auto &Stats = GSA.runOnModule(*IRDB.getModule());

    if (EmitterOptions &
        AnalysisControllerEmitterOptions::EmitStatisticsAsText) {
      llvm::outs() << Stats << '\n';
    }

    if (EmitterOptions &
        AnalysisControllerEmitterOptions::EmitStatisticsAsJson) {
      WithResultFileOrStdout("/psr-IrStatistics.json",
                             [&Stats](auto &OS) { Stats.printAsJson(OS); });
    }
  }
}

static void executeDemandDriven(AnalysisController::ControllerData & /*Data*/) {
  llvm::report_fatal_error(
      "AnalysisStrategy 'demand-driven' not supported, yet!");
}
static void executeIncremental(AnalysisController::ControllerData & /*Data*/) {
  llvm::report_fatal_error(
      "AnalysisStrategy 'incremental' not supported, yet!");
}
static void executeModuleWise(AnalysisController::ControllerData & /*Data*/) {
  llvm::report_fatal_error(
      "AnalysisStrategy 'module-wise' not supported, yet!");
}
static void executeVariational(AnalysisController::ControllerData & /*Data*/) {
  llvm::report_fatal_error(
      "AnalysisStrategy 'variational' not supported, yet!");
}

static void executeWholeProgram(AnalysisController::ControllerData &Data) {
  for (auto DataFlowAnalysis : Data.DataFlowAnalyses) {
    using namespace controller;
    switch (DataFlowAnalysis) {
    case DataFlowAnalysisType::None:
      continue;
    case DataFlowAnalysisType::IFDSUninitializedVariables:
      executeIFDSUninitVar(Data);
      continue;
    case DataFlowAnalysisType::IFDSConstAnalysis:
      executeIFDSConst(Data);
      continue;
    case DataFlowAnalysisType::IFDSTaintAnalysis:
      executeIFDSTaint(Data);
      continue;
    case DataFlowAnalysisType::IDEExtendedTaintAnalysis:
      executeIDEXTaint(Data);
      continue;
    case DataFlowAnalysisType::IDEOpenSSLTypeStateAnalysis:
      executeIDEOpenSSLTS(Data);
      continue;
    case DataFlowAnalysisType::IDECSTDIOTypeStateAnalysis:
      executeIDECSTDIOTS(Data);
      continue;
    case DataFlowAnalysisType::IFDSTypeAnalysis:
      executeIFDSType(Data);
      continue;
    case DataFlowAnalysisType::IFDSSolverTest:
      executeIFDSSolverTest(Data);
      continue;
    case DataFlowAnalysisType::IDELinearConstantAnalysis:
      executeIDELinearConst(Data);
      continue;
    case DataFlowAnalysisType::IDESolverTest:
      executeIDESolverTest(Data);
      continue;
    case DataFlowAnalysisType::IDEInstInteractionAnalysis:
      executeIDEIIA(Data);
      continue;
    case DataFlowAnalysisType::IntraMonoFullConstantPropagation:
      executeIntraMonoFullConstant(Data);
      continue;
    case DataFlowAnalysisType::IntraMonoSolverTest:
      executeIntraMonoSolverTest(Data);
      continue;
    case DataFlowAnalysisType::InterMonoSolverTest:
      executeInterMonoSolverTest(Data);
      continue;
    case DataFlowAnalysisType::InterMonoTaintAnalysis:
      executeInterMonoTaint(Data);
      continue;
    }

    llvm_unreachable("All possible DataFlowAnalysisType variants should be "
                     "handled in the switch above!");
  }
}

static void executeAs(AnalysisController::ControllerData &Data,
                      AnalysisStrategy Strategy) {
  switch (Strategy) {
  case AnalysisStrategy::None:
    return;
  case AnalysisStrategy::DemandDriven:
    executeDemandDriven(Data);
    return;
  case AnalysisStrategy::Incremental:
    executeIncremental(Data);
    return;
  case AnalysisStrategy::ModuleWise:
    executeModuleWise(Data);
    return;
  case AnalysisStrategy::Variational:
    executeVariational(Data);
    return;
  case AnalysisStrategy::WholeProgram:
    executeWholeProgram(Data);
    return;
  }
  llvm_unreachable(
      "All AnalysisStrategy variants should be handled in the switch above!");
}

AnalysisController::AnalysisController(
    HelperAnalyses &HA, std::vector<DataFlowAnalysisType> DataFlowAnalyses,
    std::vector<std::string> AnalysisConfigs,
    std::vector<std::string> EntryPoints, AnalysisStrategy Strategy,
    AnalysisControllerEmitterOptions EmitterOptions,
    IFDSIDESolverConfig SolverConfig, std::string ProjectID,
    std::string OutDirectory)
    : Data{
          &HA,
          std::move(DataFlowAnalyses),
          std::move(AnalysisConfigs),
          std::move(EntryPoints),
          Strategy,
          EmitterOptions,
          std::move(ProjectID),
          std::move(OutDirectory),
          SolverConfig,
      } {
  if (!Data.ResultDirectory.empty()) {
    // create directory for results
    Data.ResultDirectory /= Data.ProjectID + "-" + createTimeStamp();
    std::filesystem::create_directory(Data.ResultDirectory);
  }
  emitRequestedHelperAnalysisResults(Data);
  executeAs(Data, Strategy);
}

LLVMTaintConfig
controller::makeTaintConfig(AnalysisController::ControllerData &Data) {
  std::string AnalysisConfigPath =
      !Data.AnalysisConfigs.empty() ? Data.AnalysisConfigs[0] : "";
  return !AnalysisConfigPath.empty()
             ? LLVMTaintConfig(Data.HA->getProjectIRDB(),
                               parseTaintConfig(AnalysisConfigPath))
             : LLVMTaintConfig(Data.HA->getProjectIRDB());
}

} // namespace psr
