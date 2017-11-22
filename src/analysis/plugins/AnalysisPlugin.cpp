#include "AnalysisPlugin.h"

const set<string> AvailablePluginInterfaces{
    "IFDSTabulationProblemPlugin", "IDETabulationProblemPlugin",
    "IntraMonotoneProblemPlugin", "InterMonotoneProblemPlugin"};

AnalysisPlugin::AnalysisPlugin(const string &AnalysisIface,
                               const string &AnalysisPlugin,
                               LLVMBasedICFG &ICFG,
                               vector<string> EntryPoints) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Start analysis plugin";
  SOL PluginSOL(AnalysisPlugin);
  if (AnalysisIface == "IFDSTabulationProblemPlugin") {
    auto ProblemFactory =
        PluginSOL.loadSymbol<unique_ptr<IFDSTabulationProblemPlugin> (*)(
            LLVMBasedICFG &, vector<string>)>(
            "createIFDSTabulationProblemPlugin");
    unique_ptr<IFDSTabulationProblemPlugin> AnalysisProblem =
        ProblemFactory(ICFG, EntryPoints);
    LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> PluginSolver(
        *AnalysisProblem, true);
    PluginSolver.solve();
  } else if (AnalysisIface == "IDETabulationProblemPlugin") {
    throw runtime_error(AnalysisIface + " not implemented yet!");
    //   auto ProblemFactory =
    //   PluginSOL.loadSymbol<unique_ptr<IDETabulationProblemPlugin>(*)(LLVMBasedICFG&,
    //   vector<string>)>("createIDETabulationProblemPlugin");
    //   unique_ptr<IDETabulationProblemPlugin> AnalysisProblem =
    //   ProblemFactory(ICFG, EntryPoints);
    //   LLVMIDESolver<const llvm::Value*, LLVMBasedICFG&>
    //   PluginSolver(*AnalysisProblem, true);
    //   PluginSolver.solve();
  } else if (AnalysisIface == "IntraMonotoneProblemPlugin") {
    throw runtime_error(AnalysisIface + " not implemented yet!");
    //   auto ProblemFactory =
    //   PluginSOL.loadSymbol<unique_ptr<IntraMonotoneProblemPlugin>(*)(LLVMBasedICFG&,
    //   vector<string>)>("createIntraMonotoneProblemPlugin");
    //   unique_ptr<IntraMonotoneProblemPlugin> AnalysisProblem =
    //   ProblemFactory(ICFG, EntryPoints);
    //   LLVMIntraMonotoneSolver<const llvm::Value*, LLVMBasedICFG&>
    //   PluginSolver(*AnalysisProblem, true);
    //   PluginSolver.solve();
  } else if (AnalysisIface == "InterMonotoneProblemPlugin") {
    throw runtime_error(AnalysisIface + " not implemented yet!");
    //   auto ProblemFactory =
    //   PluginSOL.loadSymbol<unique_ptr<InterProblemPlugin>(*)(LLVMBasedICFG&,
    //   vector<string>)>("createInterMonotoneProblemPlugin");
    //   unique_ptr<InterMonotoneProblemPlugin> AnalysisProblem =
    //   ProblemFactory(ICFG, EntryPoints);
    //   LLVMInterMonotoneSolver<const llvm::Value*, LLVMBasedICFG&>
    //   PluginSolver(*AnalysisProblem, true);
    //   PluginSolver.solve();
  } else {
    throw runtime_error("Analysis interface not valid!");
  }
}
