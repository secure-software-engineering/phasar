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

#include <phasar/Config/Configuration.h>
#include <phasar/Controller/AnalysisController.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/Strategies.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h>
#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>

using namespace std;
using namespace psr;

namespace psr {

template <typename T> static set<T> vectorToSet(const vector<T> &v) {
  set<T> s;
  for_each(v.being(), v.end(), [&s](T t) { s.insert(t) });
  return s;
}

AnalysisController::AnalysisController()
    : IRDB(PhasarConfig::VariablesMap()["module"]
               .as<std::vector<std::string>>()),
      TH(IRDB), PT(IRDB), ICF(TH, IRDB),
      EntryPoints(vectorToSet(PhasarConfig::VariablesMap()["entry-points"]
                                  .as<std::vector<std::string>>())) {
  executeAs(to_AnalysisStrategy(
      PhasarConfig::VariablesMap()["analysis-strategy"].as<std::string>()));
}

void AnalysisController::executeAs(AnalysisStrategy Strategy) {
  switch (Strategy) {
  case AnalysisStrategy::DemandDriven:
    assert(false && "AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::Incremental:
    assert(false && "AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::ModuleWise:
    assert(false && "AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::Variational:
    assert(false && "AnalysisStrategy not supported, yet!");
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
  for (auto DataFlowAnalysisName :
       PhasarConfig::VariablesMap()["data-flow-analysis"]
           .as<std::vector<std::string>>()) {
    auto DataFlowAnalysis = to_DataFlowAnalysisType(DataFlowAnalysisName);
    switch (DataFlowAnalysis) {
    case DataFlowAnalysisType::IFDSUninitializedVariables: {
      WholeProgramAnalysis<
          IFDSSolver<
              IFDSUninitializedVariables::n_t, IFDSUninitializedVariables::d_t,
              IFDSUninitializedVariables::m_t, IFDSUninitializedVariables::i_t>,
          IFDSUninitializedVariables>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSConstAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
                     IFDSConstAnalysis::m_t, IFDSConstAnalysis::i_t>,
          IFDSConstAnalysis>
          WPA(IRDB, EntryPoints, PT, ICF, TH);
    } break;
    case DataFlowAnalysisType::IFDSTaintAnalysis: {
      // TODO needs configuration
      // WholeProgramAnalysis<
      //     IFDSSolver<IFDSTaintAnalysis::n_t, IFDSTaintAnalysis::d_t,
      //                IFDSTaintAnalysis::m_t, IFDSTaintAnalysis::i_t>,
      //     IFDSTaintAnalysis>
      //     WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDETaintAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<IDETaintAnalysis::n_t, IDETaintAnalysis::d_t,
                     IDETaintAnalysis::m_t, IDETaintAnalysis::v_t,
                     IDETaintAnalysis::i_t>,
          IDETaintAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDETypeStateAnalysis: {
      // TODO needs configuration
      // WholeProgramAnalysis<
      //     IFDSSolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
      //                IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::v_t,
      //                IDETypeStateAnalysis::i_t>,
      //     IDETypeStateAnalysis>
      //     WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSTypeAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<IFDSTypeAnalysis::n_t, IFDSTypeAnalysis::d_t,
                     IFDSTypeAnalysis::m_t, IFDSTypeAnalysis::i_t>,
          IFDSTypeAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSSolverTest: {
      WholeProgramAnalysis<IFDSSolver<IFDSSolverTest::n_t, IFDSSolverTest::d_t,
                                      IFDSSolverTest::m_t, IFDSSolverTest::i_t>,
                           IFDSSolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSLinearConstantAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<
              IFDSLinearConstantAnalysis::n_t, IFDSLinearConstantAnalysis::d_t,
              IFDSLinearConstantAnalysis::m_t, IFDSLinearConstantAnalysis::i_t>,
          IFDSLinearConstantAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSFieldSensTaintAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<
              IFDSFieldSensTaintAnalysis::n_t, IFDSFieldSensTaintAnalysis::d_t,
              IFDSFieldSensTaintAnalysis::m_t, IFDSFieldSensTaintAnalysis::i_t>,
          IFDSFieldSensTaintAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDELinearConstantAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<
              IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
              IDELinearConstantAnalysis::m_t, IDELinearConstantAnalysis::v_t,
              IDELinearConstantAnalysis::i_t>,
          IDELinearConstantAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDESolverTest: {
      WholeProgramAnalysis<
          IFDSSolver<IDESolverTest::n_t, IDESolverTest::d_t, IDESolverTest::m_t,
                     IDESolverTest::v_t, IDESolverTest::i_t>,
          IDESolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IntraMonoFullConstantPropagation: {
      WholeProgramAnalysis<IFDSSolver<IntraMonoFullConstantPropagation::n_t,
                                      IntraMonoFullConstantPropagation::d_t,
                                      IntraMonoFullConstantPropagation::m_t,
                                      IntraMonoFullConstantPropagation::i_t>,
                           IntraMonoFullConstantPropagation>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IntraMonoSolverTest: {
      WholeProgramAnalysis<
          IFDSSolver<IntraMonoSolverTest::n_t, IntraMonoSolverTest::d_t,
                     IntraMonoSolverTest::m_t, IntraMonoSolverTest::i_t>,
          IntraMonoSolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::InterMonoSolverTest: {
      WholeProgramAnalysis<
          IFDSSolver<InterMonoSolverTest::n_t, InterMonoSolverTest::d_t,
                     InterMonoSolverTest::m_t, InterMonoSolverTest::i_t>,
          InterMonoSolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::InterMonoTaintAnalysis: {
      // TODO needs configuration
      // WholeProgramAnalysis<
      //     IFDSSolver<InterMonoTaintAnalysis::n_t,
      //     InterMonoTaintAnalysis::d_t,
      //                InterMonoTaintAnalysis::m_t,
      //                InterMonoTaintAnalysis::i_t>,
      //     InterMonoTaintAnalysis>
      //     WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::Plugin:
      break;
    default:
      break;
    }
  }
}

} // namespace psr
