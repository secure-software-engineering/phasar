#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "boost/program_options.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Soundness.h"

void validateParamModule(const std::string &Module) {
  if (Module.empty()) {
    throw boost::program_options::error_with_option_name(
        "At least one LLVM target module is required!");
  }
  std::filesystem::path ModulePath(Module);
  if (!(std::filesystem::exists(ModulePath) &&
        !std::filesystem::is_directory(ModulePath) &&
        (ModulePath.extension() == ".ll" || ModulePath.extension() == ".bc"))) {
    throw boost::program_options::error_with_option_name(
        "LLVM module '" + Module + "' does not exist!");
  }
}

void validateParamOutput(const std::string &Output) {
  if (Output.empty() || std::filesystem::is_directory(Output)) {
    throw boost::program_options::error_with_option_name("Invalid file name '" +
                                                         Output + "'!");
  }
}

int main(int Argc, char **Argv) {
  psr::initializeLogger(false);
  // Declare a group of options that will be allowed only on command line
  boost::program_options::options_description Generic("Command-line options");
  Generic.add_options()("help,h", "Print help message");
  // Declare a group of options that will be allowed both on command line
  // and in config file
  boost::program_options::options_description Config(
      "Configuration file options");
  // clang-format off
  Config.add_options()
    ("module,m", boost::program_options::value<std::string>()->multitoken()->zero_tokens()->composing()->notifier(&validateParamModule), "Path to the module under analysis")
    ("entry-points,E", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing(), "Set the entry point(s) to be used (=default is 'main')")
    ("auto-globals,g", boost::program_options::value<bool>()->default_value(false), "Enable automated handling of global variables (=default is false)")
    ("output-file,o", boost::program_options::value<std::string>()->notifier(&validateParamOutput), "Output file; all results are written to the specified output file");
  // clang-format on
  boost::program_options::options_description CmdlineOptions;
  CmdlineOptions.add(Generic).add(Config);
  boost::program_options::options_description Visible("Allowed options");
  Visible.add(Generic).add(Config);
  boost::program_options::variables_map Vars;
  try {
    boost::program_options::store(
        boost::program_options::command_line_parser(Argc, Argv)
            .options(CmdlineOptions)
            .run(),
        Vars);
    boost::program_options::notify(Vars);
  } catch (boost::program_options::error &Err) {
    std::cerr << "Could not parse command-line arguments!\n"
              << "Error: " << Err.what() << '\n';
    return 1;
  }
  // Perform some simple sanity checks on the parsed command-line arguments.
  if (Vars.empty() || Vars.count("help")) {
    std::cout << Visible << '\n';
    return 0;
  }
  if (!Vars.count("module")) {
    std::cout << "Need to specify an LLVM (.ll/.bc) module for analysis.\n";
    return 0;
  }
  if (!Vars.count("output-file")) {
    std::cout << "Need to specify an output file for writing the results.\n";
    return 0;
  }
  // Output command-line arguments
  std::cout << "Command-line options:\n";
  std::cout << "=====================\n";
  std::cout << "Module:       " << Vars["module"].as<std::string>() << '\n';
  std::cout << "Entry points: ";
  std::vector<std::string> EntryPointsAsVec;
  if (Vars.count("entry-points")) {
    EntryPointsAsVec = Vars["entry-points"].as<std::vector<std::string>>();
  } else {
    EntryPointsAsVec.emplace_back("main");
  }
  for (const auto &EntryPoint : EntryPointsAsVec) {
    std::cout << EntryPoint;
    if (EntryPoint != EntryPointsAsVec.back()) {
      std::cout << ", ";
    } else {
      std::cout << '\n';
    }
  }
  std::cout << "Auto globals: " << Vars["auto-globals"].as<bool>() << '\n';
  std::cout << "Output file:  " << Vars["output-file"].as<std::string>() << '\n';
  // Parse LLVM IR
  psr::ProjectIRDB IR({Vars["module"].as<std::string>()});
  // If entry points other than 'main' are specified better check their
  // existence.
  std::set<std::string> EntryPoints;
  if (Vars.count("entry-points")) {
    for (const auto &EntryPoint : EntryPointsAsVec) {
      if (!IR.getFunctionDefinition(EntryPoint)) {
        std::cout << "Cannot retrieve entry point '" << EntryPoint << "'\n";
        return 0;
      }
      EntryPoints.insert(EntryPoint);
    }
  } else {
    if (!IR.getFunctionDefinition("main")) {
      std::cout << "Cannot retrieve entry point 'main'\n";
      return 0;
    }
    EntryPoints.insert("main");
  }
  psr::LLVMTypeHierarchy Th(IR);
  psr::LLVMPointsToSet Pts(IR, true);
  bool UseAutoGlobals = Vars["auto-globals"].as<bool>();
  psr::LLVMBasedICFG Icfg(IR, psr::CallGraphAnalysisType::OTF, EntryPoints, &Th,
                          &Pts, psr::Soundness::Sound, UseAutoGlobals);
  psr::IDELinearConstantAnalysis Lca(&IR, &Th, &Icfg, &Pts, EntryPoints);
  psr::IDESolver Solver(Lca);
  Solver.solve();
  Solver.dumpResults();
  return 0;
}
