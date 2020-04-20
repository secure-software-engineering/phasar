/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include "boost/dll.hpp"
#include "boost/filesystem.hpp"

#include "llvm/Support/ErrorHandling.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Plugins/AnalysisPluginController.h"
#include "phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h"
#include "phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h"
#include "phasar/PhasarLLVM/Plugins/Interfaces/Mono/InterMonoProblemPlugin.h"
#include "phasar/PhasarLLVM/Plugins/Interfaces/Mono/IntraMonoProblemPlugin.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

namespace psr {

AnalysisPluginController::AnalysisPluginController(
    const std::vector<std::string> &AnalysisPlygins, const ProjectIRDB *IRDB,
    const LLVMTypeHierarchy *TH, const LLVMBasedICFG *ICF,
    const LLVMPointsToInfo *PT, const std::set<std::string> &EntryPoints) {
  for (const auto &AnalysisPlugin : AnalysisPlygins) {
    boost::filesystem::path LibPath(AnalysisPlugin);
    boost::system::error_code Err;
    boost::dll::shared_library SharedLib(LibPath,
                                         boost::dll::load_mode::rtld_lazy, Err);
    if (Err) {
      llvm::report_fatal_error(Err.message());
    }
    // if (!IDETabulationProblemPluginFactory.empty()) {
    //   for (auto Problem : IDETabulationProblemPluginFactory) {
    //     LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
    //                   << "Solving plugin: " << Problem.first);
    //   }
    // }
    //   if (!IFDSTabulationProblemPluginFactory.empty()) {
    //     for (auto &Problem : IFDSTabulationProblemPluginFactory) {
    //       LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
    //                     << "Solving plugin: " << Problem.first);
    //       unique_ptr<IFDSTabulationProblemPlugin> plugin(
    //           Problem.second(ICFG, EntryPoints));
    //       cout << "DONE" << endl;
    //       IFDSSolver<
    //         const llvm::Instruction *, const llvm::Value *,
    //         const llvm::Function *, LLVMBasedICFG> llvmifdstestsolver(
    //           *plugin);
    //       llvmifdstestsolver.solve();
    //       // llvmifdstestsolver.dumpResults();
    //       FinalResultsJson += llvmifdstestsolver.getAsJson();
    //     }
    //   }
    //   if (!InterMonoProblemPluginFactory.empty()) {
    //     for (auto Problem : InterMonoProblemPluginFactory) {
    //       LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
    //                     << "Solving plugin: " << Problem.first);
    //     }
    //   }
    //   if (!IntraMonoProblemPluginFactory.empty()) {
    //     for (auto Problem : IntraMonoProblemPluginFactory) {
    //       LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
    //                     << "Solving plugin: " << Problem.first);
    //     }
    //   }
  }
}

} // namespace psr
