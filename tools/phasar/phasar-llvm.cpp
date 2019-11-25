/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <wise_enum.h>

#include <phasar/Config/Configuration.h>
#include <phasar/Controller/AnalysisController.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/Utils/Logger.h>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
using namespace psr;

static constexpr string MORE_PHASAR_LLVM_HELP(
#include "../phasar-llvm_more_help.txt"
);

// namespace boost {
// void throw_exception(std::exception const &e) {}
// } // namespace boost

template <typename T>
ostream &operator<<(ostream &os, const std::vector<T> &v) {
  copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
  return os;
}

int main(int argc, const char **argv) {
  // handling the command line parameters
  try {
    std::string ConfigFile;
    // Declare a group of options that will be allowed only on command line
    bpo::options_description Generic("Command-line options");
    // clang-format off
		Generic.add_options()
      ("version,v","Print PhASAR version")
			("help,h", "Print help message")
      ("more-help", "Print more help")
		  ("config,c", bpo::value<std::string>(&ConfigFile)->notifier(), "Path to the configuration file, options can be specified as 'parameter = option'")
      ("silent,s", "Suppress any non-result output");
    // clang-format on
    // Declare a group of options that will be allowed both on command line
    // and in config file
    bpo::options_description Config("Configuration file options");
    // clang-format off
    Config.add_options()
			("function,F", bpo::value<std::string>(), "Function under analysis (a mangled function name)")
			("module,m", bpo::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamModule), "Path to the module(s) under analysis")
      ("entry-points,E", bpo::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing(), "Set the entry point(s) to be used")
      ("output,O", bpo::value<std::string>()->notifier(validateParamOutput)->default_value("results.json"), "Filename for the results")
			("data-flow-analysis,D", bpo::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamDataFlowAnalysis), "Set the analysis to be run")
			("analysis-config", bpo::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(nullptr), "Set the analysis's configuration (if required)")
      ("pointer-analysis,P", bpo::value<std::string>()->notifier(validateParamPointerAnalysis), "Set the points-to analysis to be used (CFLSteens, CFLAnders)")
      ("callgraph-analysis,C", bpo::value<std::string>()->notifier(validateParamCallGraphAnalysis), "Set the call-graph algorithm to be used (CHA, RTA, DTA, VTA, OTF)")
			("classhierachy-analysis,H", "Class-hierarchy analysis")
			("vtable-analysis,V", "Virtual function table analysis")
			("statistical-analysis,S", "Statistics")
			//("export,E", bpo::value<std::string>()->notifier(validateParamExport), "Export mode (TODO: yet to implement!)")
			("mwa,M", "Enable Modulewise-program analysis mode")
			("mem2reg", "Promote memory to register pass")
			("printedgerec,R", "Print exploded-super-graph edge recorder")
      ("log,L", "Enable logging")
      ("emit-ir", "Emit preprocessed and annotated IR of analysis target")
      ("emit-raw-results", "Emit unprocessed/raw solver results")
      ("emit-esg-as-dot", "Emit the Exploded super-graph (ESG) as DOT graph")
      #ifdef PHASAR_PLUGINS_ENABLED
			("analysis-plugin", bpo::value<std::vector<std::string>>()->notifier(validateParamAnalysisPlugin), "Analysis plugin(s) (absolute path to the shared object file(s))")
      ("callgraph-plugin", bpo::value<std::string>()->notifier(validateParamICFGPlugin), "ICFG plugin (absolute path to the shared object file)")
      #endif
      ("project-id,I", bpo::value<std::string>()->default_value("myphasarproject")->notifier(validateParamProjectID), "Project Id used for the database")
      ("graph-id,G", bpo::value<std::string>()->default_value("123456")->notifier(validateParamGraphID), "Graph Id used by the visualization framework")
      ("pamm-out,A", bpo::value<std::string>()->notifier(validateParamOutput)->default_value("PAMM_data.json"), "Filename for PAMM's gathered data");
    // clang-format on
    bpo::options_description CmdlineOptions;
    CmdlineOptions.add(Generic).add(Config);
    bpo::options_description ConfigFileOptions;
    ConfigFileOptions.add(Config);
    bpo::options_description Visible("Allowed options");
    Visible.add(Generic).add(Config);
    bpo::store(
        bpo::command_line_parser(argc, argv).options(CmdlineOptions).run(),
        PhasarConfig::VariablesMap());
    bpo::notify(PhasarConfig::VariablesMap());
    initializeLogger(PhasarConfig::VariablesMap().count("log"));
    ifstream ifs(ConfigFile.c_str());
    if (!ifs) {
    } else {
      bpo::store(bpo::parse_config_file(ifs, ConfigFileOptions),
                 PhasarConfig::VariablesMap());
      bpo::notify(PhasarConfig::VariablesMap());
    }

    // print PhASER version
    if (PhasarConfig::VariablesMap().count("version")) {
      std::cout << "PhASAR " << PhasarConfig::PhasarVersion() << "\n";
      return 0;
    }

    // Vanity header
    if (!PhasarConfig::VariablesMap().count("silent")) {
      std::cout << "PhASAR " << PhasarConfig::PhasarVersion()
                << "\nA LLVM-based static analysis framework\n\n";
    }
    // check if we have anything at all or a call for help
    if (PhasarConfig::VariablesMap().count("help") &&
        !PhasarConfig::VariablesMap().count("silent")) {
      std::cout << Visible << '\n';
      if (PhasarConfig::VariablesMap().count("more-help")) {
        std::cout << MORE_PHASAR_LLVM_HELP << "\n";
      }
      return 0;
    }
    if (!PhasarConfig::VariablesMap().count("silent")) {
      // Print current configuration
      if (PhasarConfig::VariablesMap().count("more-help")) {
        std::cout << Visible << '\n';
        std::cout << MORE_PHASAR_LLVM_HELP << '\n';
        return 0;
      }

#ifdef PHASAR_PLUGINS_ENABLED
  // Check if user has specified an analysis plugin
  if (!IDETabulationProblemPluginFactory.empty() ||
      !IFDSTabulationProblemPluginFactory.empty() ||
      !IntraMonoProblemPluginFactory.empty() ||
      !InterMonoProblemPluginFactory.empty()) {
    ChosenDataFlowAnalyses.push_back(DataFlowAnalysisType::Plugin);
  }
#endif

  // setup the analysis controller which executes the analyses
  AnalysisController Controller;
return 0;
}
