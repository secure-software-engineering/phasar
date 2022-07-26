/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>
#include <filesystem>
#include <functional>
#include <set>
#include <utility>

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/Controller/AnalysisController.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/Utilities.h"

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
    ProjectIRDB &IRDB, std::vector<DataFlowAnalysisType> DataFlowAnalyses,
    std::vector<std::string> AnalysisConfigs, PointerAnalysisType PTATy,
    CallGraphAnalysisType CGTy, Soundness SoundnessLevel,
    bool AutoGlobalSupport, const std::set<std::string> &EntryPoints,
    AnalysisStrategy Strategy, AnalysisControllerEmitterOptions EmitterOptions,
    IFDSIDESolverConfig SolverConfig, const std::string &ProjectID,
    const std::string &OutDirectory,
    const nlohmann::json &PrecomputedPointsToInfo)
    : IRDB(IRDB), TH(IRDB),
      PT(PrecomputedPointsToInfo.empty()
             ? LLVMPointsToSet(IRDB, !needsToEmitPTA(EmitterOptions), PTATy)
             : LLVMPointsToSet(IRDB, PrecomputedPointsToInfo)),
      ICF(IRDB, CGTy, EntryPoints, &TH, &PT, SoundnessLevel, AutoGlobalSupport),
      DataFlowAnalyses(std::move(DataFlowAnalyses)),
      AnalysisConfigs(std::move(AnalysisConfigs)), EntryPoints(EntryPoints),
      Strategy(Strategy), EmitterOptions(EmitterOptions), ProjectID(ProjectID),
      OutDirectory(OutDirectory), SolverConfig(SolverConfig),
      SoundnessLevel(SoundnessLevel), AutoGlobalSupport(AutoGlobalSupport) {
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
    case DataFlowAnalysisType::IDETaintAnalysis: {
      /// TODO: The IDETaintAnalysis seems not to be implemented at all.
      /// So, keep the error-message until we have an implementation

      // WholeProgramAnalysis<IDESolver_P<IDETaintAnalysis>, IDETaintAnalysis>
      //     WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
      // WPA.solve();
      // emitRequestedDataFlowResults(WPA);
      // WPA.releaseAllHelperAnalyses();
      llvm::errs() << "The IDETaintAnalysis is currently not available! Please "
                      "use one of the other taint analyses.\n";
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
    case DataFlowAnalysisType::IFDSLinearConstantAnalysis: {
      executeIFDSLinearConst();
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
      if (auto OFS = openFileStream("/psr-preprocess-ir.ll")) {
        IRDB.emitPreprocessedIR(*OFS);
      }
    } else {
      IRDB.emitPreprocessedIR();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsText) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-th.txt")) {
        TH.print(*OFS);
      }
    } else {
      TH.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-th.dot")) {
        TH.printAsDot(*OFS);
      }
    } else {
      TH.printAsDot();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsJson) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-th.json")) {
        TH.printAsJson(*OFS);
      }
    } else {
      TH.printAsJson();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-pta.txt")) {
        PT.print(*OFS);
      }
    } else {
      PT.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-pta.dot")) {
        PT.print(*OFS);
      }
    } else {
      PT.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-pta.json")) {
        PT.printAsJson(*OFS);
      }
    } else {
      PT.printAsJson();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsText) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-cg.txt")) {
        ICF.print(*OFS);
      }
    } else {
      ICF.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsDot) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-cg.dot")) {
        ICF.printAsDot(*OFS);
      }
    } else {
      ICF.printAsDot();
    }
  }

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsJson) {
    if (!ResultDirectory.empty()) {
      if (auto OFS = openFileStream("/psr-cg.json")) {
        ICF.printAsJson(*OFS);
      }
    } else {
      ICF.printAsJson();
    }
  }
}

std::unique_ptr<llvm::raw_fd_ostream>
AnalysisController::openFileStream(llvm::StringRef FilePathSuffix) {
  std::error_code EC;
  auto OFS = std::make_unique<llvm::raw_fd_ostream>(
      ResultDirectory.string() + FilePathSuffix.str(), EC);
  if (EC) {
    OFS = nullptr;
    llvm::errs() << "Failed to open file: "
                 << ResultDirectory.string() + FilePathSuffix << '\n';
    llvm::errs() << EC.message() << '\n';
  }
  return OFS;
}

} // namespace psr
