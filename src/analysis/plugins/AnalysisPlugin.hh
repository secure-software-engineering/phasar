#ifndef ANALYSIS_PLUGIN_HH_
#define ANALYSIS_PLUGIN_HH_

#include <string>
#include <vector>
#include <set>
#include "../icfg/LLVMBasedICFG.hh"
#include "../../utils/SOL.hh"
#include "plugin_ifaces/IDETabulationProblemPlugin.hh"
#include "plugin_ifaces/IFDSTabulationProblemPlugin.hh"
#include "plugin_ifaces/InterMonotoneProblemPlugin.hh"
#include "plugin_ifaces/IntraMonotoneProblemPlugin.hh"
#include "../ifds_ide/solver/LLVMIFDSSolver.hh"
#include "../ifds_ide/solver/LLVMIDESolver.hh"
#include "../monotone/solver/LLVMInterMonotoneSolver.hh"
#include "../monotone/solver/LLVMIntraMonotoneSolver.hh"
using namespace std;

extern const set<string> AvailablePluginInterfaces;

class AnalysisPlugin {
 public:
  AnalysisPlugin(const string& AnalysisIface, const string& AnalysisPlugin,
                 LLVMBasedICFG& ICFG, vector<string> EntryPoints);
};

#endif