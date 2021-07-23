#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "boost/program_options.hpp"

#include "nlohmann/json.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Type.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
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

nlohmann::json getGlobalsInfo(psr::ProjectIRDB &IR, psr::LLVMBasedICFG &Icfg) {
  nlohmann::json GlobalsJson;
  const llvm::Module *Mod = IR.getWPAModule();
  GlobalsJson["#globals"] = Mod->global_size();
  GlobalsJson["#analyzed-global-ctors"] = Icfg.getGlobalCtors().size();
  GlobalsJson["#analyzed-global-dtors"] = Icfg.getGlobalDtors().size();
  size_t NumGlobalVars = 0;
  size_t NumGlobalUses = 0;
  size_t NumIntegerTypes = 0;
  std::set<llvm::Type *> DistinctTys;
  for (const auto &Global : Mod->globals()) {
    if (!llvm::isa<llvm::GlobalVariable>(Global)) {
      continue;
    }
    ++NumGlobalVars;
    DistinctTys.insert(Global.getType());
    if (llvm::isa<llvm::IntegerType>(Global.getType()) ||
        (Global.getType()->isPointerTy() &&
         llvm::isa<llvm::IntegerType>(
             Global.getType()->getPointerElementType()))) {
      ++NumIntegerTypes;
    }
    NumGlobalUses += Global.getNumUses();
  }
  GlobalsJson["#global-vars"] = NumGlobalVars;
  GlobalsJson["#global-uses"] = NumGlobalUses;
  GlobalsJson["#global-int-typed"] = NumIntegerTypes;
  GlobalsJson["#global-distinct-types"] = DistinctTys.size();
  return GlobalsJson;
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
    ("verbose,v", "Print output to the command line (=default is false)")
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
  std::cout << "Module      : " << Vars["module"].as<std::string>() << '\n';
  std::cout << "Entry points: ";
  std::stringstream StrStr;
  std::vector<std::string> EntryPointsAsVec;
  if (Vars.count("entry-points")) {
    EntryPointsAsVec = Vars["entry-points"].as<std::vector<std::string>>();
  } else {
    EntryPointsAsVec.emplace_back("main");
  }
  for (const auto &EntryPoint : EntryPointsAsVec) {
    StrStr << EntryPoint;
    if (EntryPoint != EntryPointsAsVec.back()) {
      StrStr << ", ";
    } else {
      StrStr << '\n';
    }
  }
  std::cout << StrStr.str();
  std::cout << "Auto globals: " << Vars["auto-globals"].as<bool>() << '\n';
  std::cout << "Output file : " << Vars["output-file"].as<std::string>()
            << '\n';
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
  // Analysis start
  auto start = std::chrono::steady_clock::now();
  psr::LLVMTypeHierarchy Th(IR);
  psr::LLVMPointsToSet Pts(IR, true);
  bool UseAutoGlobals = Vars["auto-globals"].as<bool>();
  psr::LLVMBasedICFG Icfg(IR, psr::CallGraphAnalysisType::OTF, EntryPoints, &Th,
                          &Pts, psr::Soundness::Sound, UseAutoGlobals);

  const auto *GlobalCRuntimeModel =
      IR.getFunctionDefinition(psr::LLVMBasedICFG::GlobalCRuntimeModelName);
  std::set<std::string> GlobalCRuntimeModelEntries;
  if (GlobalCRuntimeModel) {
    GlobalCRuntimeModelEntries.insert(
        psr::LLVMBasedICFG::GlobalCRuntimeModelName.str());
  }

  psr::IDELinearConstantAnalysis Lca(
      &IR, &Th, &Icfg, &Pts,
      (GlobalCRuntimeModel) ? GlobalCRuntimeModelEntries : EntryPoints,
      UseAutoGlobals);

  psr::IDESolver Solver(Lca);
  Solver.solve();
  // Analysis end
  auto end = std::chrono::steady_clock::now();
  if (Vars.count("verbose")) {
    Solver.dumpResults();
  }
  nlohmann::json ResultsJson;
  // Set program under analysis
  ResultsJson["program"] = Vars["module"].as<std::string>();
  ResultsJson["runtime-in-seconds"] =
      std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
  std::string Eps = StrStr.str();
  Eps.erase(std::remove(Eps.begin(), Eps.end(), '\n'), Eps.end());
  ResultsJson["entry-points"] = Eps;
  ResultsJson["auto-globals"] = Vars["auto-globals"].as<bool>();
  // Determine general information on globals and usage
  auto GlobalsInfoJson = getGlobalsInfo(IR, Icfg);
  ResultsJson.insert(GlobalsInfoJson.begin(), GlobalsInfoJson.end());
  // Determine interesting data-flow information on globals
  std::set<const llvm::GlobalVariable *> GenGlobals;
  const auto *Mod = IR.getWPAModule();
  for (const auto &G : Mod->globals()) {
    if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
      if (GV->hasInitializer()) {
        if (const auto *ConstInt =
                llvm::dyn_cast<llvm::ConstantInt>(GV->getInitializer())) {
          GenGlobals.insert(GV);
        }
      }
    }
  }
  ResultsJson["#required-globals-generation"] = GenGlobals.size();
  // Check effects of global ctors on global integer variables
  const auto *Main = IR.getFunctionDefinition("main");
  assert(Main);
  const auto *FirstMainInst = &Main->front().front();
  const auto *LastMainInst = &Main->back().back();
  size_t NumNonTopValuesAtStart = 0;
  size_t NumNonTopValuesAtEnd = 0;
  for (const auto *GenGlobal : GenGlobals) {
    auto ResAtStart = Solver.resultsAt(FirstMainInst);
    if (ResAtStart.find(GenGlobal) != ResAtStart.end()) {
      auto ValAtStart = Solver.resultAt(FirstMainInst, GenGlobal);
      if (ValAtStart != Lca.topElement()) {
        ++NumNonTopValuesAtStart;
      }
    }
    auto ResAtEnd = Solver.resultsAt(LastMainInst);
    if (ResAtEnd.find(GenGlobal) != ResAtEnd.end()) {
      auto ValAtEnd = Solver.resultAt(LastMainInst, GenGlobal);
      if (ValAtEnd != Lca.topElement()) {
        ++NumNonTopValuesAtEnd;
      }
    }
  }
  ResultsJson["#non-top-vals-at-start"] = NumNonTopValuesAtStart;
  ResultsJson["#non-top-vals-at-end"] = NumNonTopValuesAtEnd;
  // Check effects of main on global integer variable
  if (Vars.count("verbose")) {
    std::cout << "Results:\n";
    std::cout << "========\n";
    std::cout << ResultsJson.dump(2) << '\n';
  }
  std::ofstream Ofs(Vars["output-file"].as<std::string>());
  if (!Ofs) {
    std::cout << "Could not create output file '"
              << Vars["output-file"].as<std::string>() << "'\n";
    return 1;
  }
  Ofs << ResultsJson.dump(2) << '\n';
  return 0;
}
