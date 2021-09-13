/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <utility>

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
    CallGraphAnalysisType CGTy, Soundness S,
    const std::set<std::string> &EntryPoints, AnalysisStrategy Strategy,
    AnalysisControllerEmitterOptions EmitterOptions,
    const std::string &ProjectID, const std::string &OutDirectory)
    : IRDB(IRDB), TH(IRDB), PT(IRDB, !needsToEmitPTA(EmitterOptions), PTATy),
      ICF(IRDB, CGTy, EntryPoints, &TH, &PT),
      DataFlowAnalyses(std::move(DataFlowAnalyses)),
      AnalysisConfigs(std::move(AnalysisConfigs)), EntryPoints(EntryPoints),
      Strategy(Strategy), EmitterOptions(EmitterOptions), ProjectID(ProjectID),
      OutDirectory(OutDirectory), S(S) {
  if (!OutDirectory.empty()) {
    // create directory for results
    ResultDirectory = OutDirectory + "/" + ProjectID + "-" + createTimeStamp();
    boost::filesystem::create_directory(ResultDirectory);
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
  for (auto _DataFlowAnalysis : DataFlowAnalyses) {
    std::string AnalysisConfigPath =
        (ConfigIdx < AnalysisConfigs.size()) ? AnalysisConfigs[ConfigIdx] : "";
    if (std::holds_alternative<DataFlowAnalysisType>(_DataFlowAnalysis)) {
      auto getTaintConfig = [&]() {
        if (!AnalysisConfigPath.empty()) {
          return TaintConfig(IRDB, parseTaintConfig(AnalysisConfigPath));
        }
        return TaintConfig(IRDB);
      };
      auto DataFlowAnalysis = std::get<DataFlowAnalysisType>(_DataFlowAnalysis);
      switch (DataFlowAnalysis) {
      case DataFlowAnalysisType::IFDSUninitializedVariables: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSUninitializedVariables>,
                             IFDSUninitializedVariables>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSConstAnalysis: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSConstAnalysis>, IFDSConstAnalysis>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<IFDSSolver_P<IFDSTaintAnalysis>, IFDSTaintAnalysis>
            WPA(IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDEExtendedTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<IDESolver_P<IDEExtendedTaintAnalysis<>>,
                             IDEExtendedTaintAnalysis<>>
            WPA(IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDETaintAnalysis: {
        /// TODO: The IDETaintAnalysis seems not to be implemented at all.
        /// So, keep the error-message until we have an implementation

        // WholeProgramAnalysis<IDESolver_P<IDETaintAnalysis>, IDETaintAnalysis>
        //     WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        // WPA.solve();
        // emitRequestedDataFlowResults(WPA);
        // WPA.releaseAllHelperAnalyses();
        std::cerr << "The IDETaintAnalysis is currently not available! Please "
                     "use one of the other taint analyses.\n";
      } break;
      case DataFlowAnalysisType::IDEOpenSSLTypeStateAnalysis: {
        OpenSSLEVPKDFDescription TSDesc;
        WholeProgramAnalysis<IDESolver_P<IDETypeStateAnalysis>,
                             IDETypeStateAnalysis>
            WPA(IRDB, &TSDesc, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDECSTDIOTypeStateAnalysis: {
        CSTDFILEIOTypeStateDescription TSDesc;
        WholeProgramAnalysis<IDESolver_P<IDETypeStateAnalysis>,
                             IDETypeStateAnalysis>
            WPA(IRDB, &TSDesc, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSTypeAnalysis: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSTypeAnalysis>, IFDSTypeAnalysis>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSSolverTest: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSSolverTest>, IFDSSolverTest> WPA(
            IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSLinearConstantAnalysis: {
        WholeProgramAnalysis<IFDSSolver_P<IFDSLinearConstantAnalysis>,
                             IFDSLinearConstantAnalysis>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IFDSFieldSensTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<IFDSSolver_P<IFDSFieldSensTaintAnalysis>,
                             IFDSFieldSensTaintAnalysis>
            WPA(IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDELinearConstantAnalysis: {
        WholeProgramAnalysis<IDESolver_P<IDELinearConstantAnalysis>,
                             IDELinearConstantAnalysis>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDESolverTest: {
        WholeProgramAnalysis<IDESolver_P<IDESolverTest>, IDESolverTest> WPA(
            IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IDEInstInteractionAnalysis: {
        WholeProgramAnalysis<IDESolver_P<IDEInstInteractionAnalysis>,
                             IDEInstInteractionAnalysis>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IntraMonoFullConstantPropagation: {
        WholeProgramAnalysis<
            IntraMonoSolver_P<IntraMonoFullConstantPropagation>,
            IntraMonoFullConstantPropagation>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::IntraMonoSolverTest: {
        WholeProgramAnalysis<IntraMonoSolver_P<IntraMonoSolverTest>,
                             IntraMonoSolverTest>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::InterMonoSolverTest: {
        WholeProgramAnalysis<InterMonoSolver_P<InterMonoSolverTest, 3>,
                             InterMonoSolverTest>
            WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      case DataFlowAnalysisType::InterMonoTaintAnalysis: {
        auto Config = getTaintConfig();
        WholeProgramAnalysis<InterMonoSolver_P<InterMonoTaintAnalysis, 3>,
                             InterMonoTaintAnalysis>
            WPA(IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
        WPA.solve();
        emitRequestedDataFlowResults(WPA);
        WPA.releaseAllHelperAnalyses();
      } break;
      default:
        break;
      }
    } else if (std::holds_alternative<IFDSPluginConstructor>(
                   _DataFlowAnalysis)) {
      auto Problem = std::get<IFDSPluginConstructor>(_DataFlowAnalysis)(
          &IRDB, &TH, &ICF, &PT, EntryPoints);
      IFDSSolver_P<std::remove_reference<decltype(*Problem)>::type> Solver(
          *Problem);
      Solver.solve();
      emitRequestedDataFlowResults(Solver);
    } else if (std::holds_alternative<IDEPluginConstructor>(
                   _DataFlowAnalysis)) {
      auto Problem = std::get<IDEPluginConstructor>(_DataFlowAnalysis)(
          &IRDB, &TH, &ICF, &PT, EntryPoints);
      IDESolver_P<std::remove_reference<decltype(*Problem)>::type> Solver(
          *Problem);
      Solver.solve();
      emitRequestedDataFlowResults(Solver);
    } else if (std::holds_alternative<IntraMonoPluginConstructor>(
                   _DataFlowAnalysis)) {

      auto Problem = std::get<IntraMonoPluginConstructor>(_DataFlowAnalysis)(
          &IRDB, &TH, &ICF, &PT, EntryPoints);
      IntraMonoSolver_P<std::remove_reference<decltype(*Problem)>::type> Solver(
          *Problem);
      Solver.solve();
      emitRequestedDataFlowResults(Solver);
    } else if (std::holds_alternative<InterMonoPluginConstructor>(
                   _DataFlowAnalysis)) {
      auto Problem = std::get<InterMonoPluginConstructor>(_DataFlowAnalysis)(
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
      std::ofstream OFS(ResultDirectory.string() + "/psr-preprocess-ir.ll");
      IRDB.emitPreprocessedIR(OFS);
    } else {
      IRDB.emitPreprocessedIR();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsText) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-th.txt");
      TH.print(OFS);
    } else {
      TH.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsDot) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-th.dot");
      TH.printAsDot(OFS);
    } else {
      TH.printAsDot();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTHAsJson) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-th.json");
      TH.printAsJson(OFS);
    } else {
      TH.printAsJson();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-pta.txt");
      PT.print(OFS);
    } else {
      PT.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-pta.dot");
      PT.print(OFS);
    } else {
      PT.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-pta.json");
      PT.printAsJson(OFS);
    } else {
      PT.printAsJson();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsText) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-cg.txt");
      ICF.print(OFS);
    } else {
      ICF.print();
    }
  }
  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsDot) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-cg.dot");
      ICF.printAsDot(OFS);
    } else {
      ICF.printAsDot();
    }
  }

  if (EmitterOptions & AnalysisControllerEmitterOptions::EmitCGAsJson) {
    if (!ResultDirectory.empty()) {
      std::ofstream OFS(ResultDirectory.string() + "/psr-cg.json");
      ICF.printAsJson(OFS);
    } else {
      ICF.printAsJson();
    }
  }
}

} // namespace psr
