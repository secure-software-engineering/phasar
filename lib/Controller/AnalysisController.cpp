/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"

#include "phasar/Controller/AnalysisControllerEmitterOptions.h"
#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/HelperAnalyses.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
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

using namespace std;
using namespace psr;

namespace std {

template <> struct hash<pair<const llvm::Value *, unsigned>> {
  size_t operator()(const pair<const llvm::Value *, unsigned> &P) const {
    std::hash<const llvm::Value *> HashPtr;
    std::hash<unsigned> HashUnsigned;
    size_t Hp = HashPtr(P.first);
    size_t Hu = HashUnsigned(P.second);
    return Hp ^ (Hu << 1);
  }
};

} // namespace std

namespace psr {

AnalysisController::AnalysisController(
    HelperAnalyses &HA, std::vector<DataFlowAnalysisType> DataFlowAnalyses,
    std::vector<std::string> AnalysisConfigs,
    std::vector<std::string> EntryPoints, AnalysisStrategy Strategy,
    AnalysisControllerEmitterOptions EmitterOptions,
    IFDSIDESolverConfig SolverConfig, const std::string &ProjectID,
    const std::string &OutDirectory)
    : HA(HA), DataFlowAnalyses(std::move(DataFlowAnalyses)),
      AnalysisConfigs(std::move(AnalysisConfigs)),
      EntryPoints(std::move(EntryPoints)), Strategy(Strategy),
      EmitterOptions(EmitterOptions), ProjectID(ProjectID),
      ResultDirectory(OutDirectory), SolverConfig(SolverConfig) {
  if (!OutDirectory.empty()) {
    // create directory for results
    ResultDirectory = OutDirectory;
    ResultDirectory /= ProjectID + "-" + createTimeStamp();
    std::filesystem::create_directory(ResultDirectory);
  }
  emitRequestedHelperAnalysisResults();
  executeAs(Strategy);
}

void AnalysisController::executeAs(AnalysisStrategy Strategy) {
  switch (Strategy) {
  case AnalysisStrategy::DemandDriven:
    llvm::report_fatal_error("AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::Incremental:
    llvm::report_fatal_error("AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::ModuleWise:
    llvm::report_fatal_error("AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::Variational:
    llvm::report_fatal_error("AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::WholeProgram:
    executeWholeProgram();
    break;
  default:
    break;
  }
}

void AnalysisController::executeDemandDriven() {}

void AnalysisController::executeIncremental() {}

void AnalysisController::executeModuleWise() {}

void AnalysisController::executeVariational() {}

void AnalysisController::executeWholeProgram() {
  size_t ConfigIdx = 0;
  for (const auto &DataFlowAnalysis : DataFlowAnalyses) {
    switch (DataFlowAnalysis) {
    case DataFlowAnalysisType::IFDSUninitializedVariables: {
      executeIFDSUninitVar();
    } break;
    case DataFlowAnalysisType::IFDSConstAnalysis: {
      executeIFDSConst();
    } break;
    case DataFlowAnalysisType::IFDSTaintAnalysis: {
      executeIFDSTaint();
    } break;
    case DataFlowAnalysisType::IDEExtendedTaintAnalysis: {
      executeIDEXTaint();
    } break;
    case DataFlowAnalysisType::IDEOpenSSLTypeStateAnalysis: {
      executeIDEOpenSSLTS();
    } break;
    case DataFlowAnalysisType::IDECSTDIOTypeStateAnalysis: {
      executeIDECSTDIOTS();
    } break;
    case DataFlowAnalysisType::IFDSTypeAnalysis: {
      executeIFDSType();
    } break;
    case DataFlowAnalysisType::IFDSSolverTest: {
      executeIFDSSolverTest();
    } break;
    case DataFlowAnalysisType::IFDSFieldSensTaintAnalysis: {
      executeIFDSFieldSensTaint();
    } break;
    case DataFlowAnalysisType::IDELinearConstantAnalysis: {
      executeIDELinearConst();
    } break;
    case DataFlowAnalysisType::IDESolverTest: {
      executeIDESolverTest();
    } break;
    case DataFlowAnalysisType::IDEInstInteractionAnalysis: {
      executeIDEIIA();
    } break;
    case DataFlowAnalysisType::IntraMonoFullConstantPropagation: {
      executeIntraMonoFullConstant();
    } break;
    case DataFlowAnalysisType::IntraMonoSolverTest: {
      executeIntraMonoSolverTest();
    } break;
    case DataFlowAnalysisType::InterMonoSolverTest: {
      executeInterMonoSolverTest();
    } break;
    case DataFlowAnalysisType::InterMonoTaintAnalysis: {
      executeInterMonoTaint();
    } break;
    default:
      break;
    }
  }
}

void AnalysisController::emitRequestedHelperAnalysisResults() {
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitIR) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() +
                                    "/psr-preprocess-ir.ll")) {
        HA.getProjectIRDB().emitPreprocessedIR(*OFS);
      }
    } else {
      HA.getProjectIRDB().emitPreprocessedIR(llvm::outs());
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsText) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + "/psr-th.txt")) {
        HA.getTypeHierarchy().print(*OFS);
      }
    } else {
      HA.getTypeHierarchy().print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + "/psr-th.dot")) {
        HA.getTypeHierarchy().printAsDot(*OFS);
      }
    } else {
      HA.getTypeHierarchy().printAsDot();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsJson) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-th.json")) {
        HA.getTypeHierarchy().printAsJson(*OFS);
      }
    } else {
      HA.getTypeHierarchy().printAsJson();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-pta.txt")) {
        HA.getPointsToInfo().print(*OFS);
      }
    } else {
      HA.getPointsToInfo().print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-pta.dot")) {
        HA.getPointsToInfo().print(*OFS);
      }
    } else {
      HA.getPointsToInfo().print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-pta.json")) {
        HA.getPointsToInfo().printAsJson(*OFS);
      }
    } else {
      HA.getPointsToInfo().printAsJson(llvm::outs());
    }
  }

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + "/psr-cg.txt")) {
        HA.getICFG().print(*OFS);
      }
    } else {
      HA.getICFG().print();
    }
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
      if (!ResultDirectory.empty()) {
        if (auto OFS = openFileStream(ResultDirectory.string() +
                                      "/psr-IrStatistics.json")) {
          Stats.printAsJson(*OFS);
        }
      } else {
        Stats.printAsJson();
      }
    }
  }
}

TaintConfig AnalysisController::makeTaintConfig() {
  std::string AnalysisConfigPath =
      !AnalysisConfigs.empty() ? AnalysisConfigs[0] : "";
  return !AnalysisConfigPath.empty()
             ? TaintConfig(HA.getProjectIRDB(),
                           parseTaintConfig(AnalysisConfigPath))
             : TaintConfig(HA.getProjectIRDB());
  ;
}

} // namespace psr
