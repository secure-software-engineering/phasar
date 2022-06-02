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
#include <fstream>
#include <functional>
#include <set>
#include <utility>

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/Controller/AnalysisController.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEProtoAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSFieldSensTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSProtoAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSignAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTypeAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/Plugins/PluginFactories.h"
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
    ProjectIRDB &IRDB, std::vector<DataFlowAnalysisKind> DataFlowAnalyses,
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
  for (const auto &DFA : DataFlowAnalyses) {
    std::string AnalysisConfigPath =
        (ConfigIdx < AnalysisConfigs.size()) ? AnalysisConfigs[ConfigIdx] : "";
    if (std::holds_alternative<DataFlowAnalysisType>(DFA)) {
      auto getTaintConfig // NOLINT
          = [&]() {
              if (!AnalysisConfigPath.empty()) {
                return TaintConfig(IRDB, parseTaintConfig(AnalysisConfigPath));
              }
              return TaintConfig(IRDB);
            };
      auto DataFlowAnalysis = std::get<DataFlowAnalysisType>(DFA);
      switch (DataFlowAnalysis) {
      case DataFlowAnalysisType::IFDSUninitializedVariables: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSUninitializedVariables>,
                             IFDSUninitializedVariables>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSConstAnalysis: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSConstAnalysis>, IFDSConstAnalysis>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<IFDSSolver_P<IFDSTaintAnalysis>, IFDSTaintAnalysis>
            WPA(SolverConfig, IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDEExtendedTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<IDESolver_P<IDEExtendedTaintAnalysis<>>,
                             IDEExtendedTaintAnalysis<>>
            WPA(SolverConfig, IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDETaintAnalysis: {
        /// TODO: The IDETaintAnalysis seems not to be implemented at all.
        /// So, keep the error-message until we have an implementation

        // WholeProgramAnalysis<IDESolver_P<IDETaintAnalysis>, IDETaintAnalysis>
        //     WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        // WPA.solve();
        // emitRequestedDataFlowResults(WPA);
        // WPA.releaseAllHelperAnalyses();
        llvm::errs()
            << "The IDETaintAnalysis is currently not available! Please "
               "use one of the other taint analyses.\n";
      } break;
      case DataFlowAnalysisType::IDEOpenSSLTypeStateAnalysis: {
        OpenSSLEVPKDFDescription TSDesc;
        WholeProgramAnalysis<IDESolver_P<IDETypeStateAnalysis>,
                             IDETypeStateAnalysis>
            WPA(SolverConfig, IRDB, &TSDesc, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDECSTDIOTypeStateAnalysis: {
        CSTDFILEIOTypeStateDescription TSDesc;
        WholeProgramAnalysis<IDESolver_P<IDETypeStateAnalysis>,
                             IDETypeStateAnalysis>
            WPA(SolverConfig, IRDB, &TSDesc, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSTypeAnalysis: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSTypeAnalysis>, IFDSTypeAnalysis>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSSolverTest: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSSolverTest>, IFDSSolverTest> WPA(
            SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSLinearConstantAnalysis: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSLinearConstantAnalysis>,
                             IFDSLinearConstantAnalysis>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSFieldSensTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<IFDSSolver_P<IFDSFieldSensTaintAnalysis>,
                             IFDSFieldSensTaintAnalysis>
            WPA(SolverConfig, IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDELinearConstantAnalysis: {
        WholeProgramAnalysis<IDESolver_P<IDELinearConstantAnalysis>,
                             IDELinearConstantAnalysis>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDESolverTest: {
        WholeProgramAnalysis<IDESolver_P<IDESolverTest>, IDESolverTest> WPA(
            SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDEInstInteractionAnalysis: {
        WholeProgramAnalysis<IDESolver_P<IDEInstInteractionAnalysis>,
                             IDEInstInteractionAnalysis>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IntraMonoFullConstantPropagation: {
        WholeProgramAnalysis<
            IntraMonoSolver_P<IntraMonoFullConstantPropagation>,
            IntraMonoFullConstantPropagation>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IntraMonoSolverTest: {
        WholeProgramAnalysis<IntraMonoSolver_P<IntraMonoSolverTest>,
                             IntraMonoSolverTest>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::InterMonoSolverTest: {
        WholeProgramAnalysis<InterMonoSolver_P<InterMonoSolverTest, 3>,
                             InterMonoSolverTest>
            WPA(SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::InterMonoTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<InterMonoSolver_P<InterMonoTaintAnalysis, 3>,
                             InterMonoTaintAnalysis>
            WPA(SolverConfig, IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      default:
        break;
      }
    } else if (std::holds_alternative<IFDSPluginConstructor>(DFA)) {
      auto Problem = std::get<IFDSPluginConstructor>(DFA)(&IRDB, &TH, &ICF, &PT,
                                                          EntryPoints);
      Problem->setIFDSIDESolverConfig(SolverConfig);
      IFDSSolver_P<std::remove_reference<decltype(*Problem)>::type> Solver(
          *Problem);
      Solver.solve();
      emitRequestedDataFlowResults(Solver);
    } else if (std::holds_alternative<IDEPluginConstructor>(DFA)) {
      auto Problem = std::get<IDEPluginConstructor>(DFA)(&IRDB, &TH, &ICF, &PT,
                                                         EntryPoints);
      Problem->setIFDSIDESolverConfig(SolverConfig);
      IDESolver_P<std::remove_reference<decltype(*Problem)>::type> Solver(
          *Problem);
      Solver.solve();
      emitRequestedDataFlowResults(Solver);
    } else if (std::holds_alternative<IntraMonoPluginConstructor>(DFA)) {

      auto Problem = std::get<IntraMonoPluginConstructor>(DFA)(
          &IRDB, &TH, &ICF, &PT, EntryPoints);
      IntraMonoSolver_P<std::remove_reference<decltype(*Problem)>::type> Solver(
          *Problem);
      Solver.solve();
      emitRequestedDataFlowResults(Solver);
    } else if (std::holds_alternative<InterMonoPluginConstructor>(DFA)) {
      auto Problem = std::get<InterMonoPluginConstructor>(DFA)(
          &IRDB, &TH, &ICF, &PT, EntryPoints);
      InterMonoSolver_P<std::remove_reference<decltype(*Problem)>::type, K>
          Solver(*Problem);
      Solver.solve();
      emitRequestedDataFlowResults(Solver);
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
