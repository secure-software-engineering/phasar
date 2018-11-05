/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonoSolver.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMIntraMonoSolver.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/InterMonoProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/IntraMonoProblemPlugin.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/SOL.h>

#include <phasar/PhasarLLVM/Plugins/AnalysisPluginController.h>
using namespace std;
using namespace psr;

namespace psr {

AnalysisPluginController::AnalysisPluginController(
    vector<string> AnalysisPlygins, LLVMBasedICFG &ICFG,
    vector<string> EntryPoints, json &Results)
    : FinalResultsJson(Results) {
  auto &lg = lg::get();
  for (const auto &AnalysisPlugin : AnalysisPlygins) {
    SOL SharedLib(AnalysisPlugin);
    if (!IDETabulationProblemPluginFactory.empty()) {
      for (auto Problem : IDETabulationProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                      << "Solving plugin: " << Problem.first);
      }
    }
    if (!IFDSTabulationProblemPluginFactory.empty()) {
      for (auto &Problem : IFDSTabulationProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                      << "Solving plugin: " << Problem.first);
        unique_ptr<IFDSTabulationProblemPlugin> plugin(
            Problem.second(ICFG, EntryPoints));
        cout << "DONE" << endl;
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmifdstestsolver(
            *plugin, true);
        llvmifdstestsolver.solve();
        FinalResultsJson += llvmifdstestsolver.getAsJson();
      }
    }
    if (!InterMonoProblemPluginFactory.empty()) {
      for (auto Problem : InterMonoProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                      << "Solving plugin: " << Problem.first);
      }
    }
    if (!IntraMonoProblemPluginFactory.empty()) {
      for (auto Problem : IntraMonoProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                      << "Solving plugin: " << Problem.first);
      }
    }
  }
}

} // namespace psr
