/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h"
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

bool needsToEmitPTA(AnalysisControllerEmitterOptions EmitterOptions) {
  return (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) ||
         (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) ||
         (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText);
}

AnalysisController::AnalysisController(
    LLVMProjectIRDB &IRDB, std::vector<DataFlowAnalysisType> DataFlowAnalyses,
    std::vector<std::string> AnalysisConfigs, PointerAnalysisType PTATy,
    CallGraphAnalysisType CGTy, Soundness SoundnessLevel,
    bool AutoGlobalSupport, std::vector<std::string> EntryPoints,
    AnalysisStrategy Strategy, AnalysisControllerEmitterOptions EmitterOptions,
    IFDSIDESolverConfig SolverConfig, const std::string &ProjectID,
    std::filesystem::path OutDirectory,
    const nlohmann::json &PrecomputedPointsToInfo)
    : IRDB(IRDB), TH(IRDB),
      PT(PrecomputedPointsToInfo.empty()
             ? LLVMPointsToSet(IRDB, !needsToEmitPTA(EmitterOptions), PTATy)
             : LLVMPointsToSet(IRDB, PrecomputedPointsToInfo)),
      ICF(&IRDB, CGTy, EntryPoints, &TH, &PT, SoundnessLevel,
          AutoGlobalSupport),
      DataFlowAnalyses(std::move(DataFlowAnalyses)),
      AnalysisConfigs(std::move(AnalysisConfigs)),
      EntryPoints(std::move(EntryPoints)), Strategy(Strategy),
      EmitterOptions(EmitterOptions), ProjectID(ProjectID),
      ResultDirectory(std::move(OutDirectory)), SolverConfig(SolverConfig),
      SoundnessLevel(SoundnessLevel), AutoGlobalSupport(AutoGlobalSupport) {

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
        *OFS << *IRDB.getModule();
      }
    } else {
      IRDB.dump();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsText) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + "/psr-th.txt")) {
        TH.print(*OFS);
      }
    } else {
      TH.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + "/psr-th.dot")) {
        TH.printAsDot(*OFS);
      }
    } else {
      TH.printAsDot();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsJson) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-th.json")) {
        TH.printAsJson(*OFS);
      }
    } else {
      TH.printAsJson();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-pta.txt")) {
        PT.print(*OFS);
      }
    } else {
      PT.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-pta.dot")) {
        PT.print(*OFS);
      }
    } else {
      PT.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) {
    if (!ResultDirectory.empty()) {
      if (auto OFS =
              openFileStream(ResultDirectory.string() + "/psr-pta.json")) {
        PT.printAsJson(*OFS);
      }
    } else {
      PT.printAsJson();
    }
  }

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream(ResultDirectory.string() + "/psr-cg.dot")) {
        ICF.print(*OFS);
      }
    } else {
      ICF.print();
    }
  }
}

} // namespace psr
