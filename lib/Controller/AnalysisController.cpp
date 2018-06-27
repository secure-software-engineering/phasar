/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>


#include <phasar/Controller/AnalysisController.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IDESummaries.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDESolverTest.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSConstAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSSolverTest.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTypeAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMMWAIFDSSolver.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonotoneSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/Mono/Problems/IntraMonotoneSolverTest.h>
#include <phasar/PhasarLLVM/Plugins/AnalysisPluginController.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Pointer/VTable.h>
#include <phasar/Utils/SOL.h>

using namespace std;
using namespace psr;

namespace psr {

const std::map<std::string, ExportType> StringToExportType = {
    {"json", ExportType::JSON}};

const std::map<ExportType, std::string> ExportTypeToString = {
    {ExportType::JSON, "json"}};

std::ostream &operator<<(std::ostream &os, const ExportType &E) {
  return os << ExportTypeToString.at(E);
}

AnalysisController::AnalysisController(
    ProjectIRDB &&IRDB, std::vector<DataFlowAnalysisType> Analyses,
    bool WPA_MODE, bool PrintEdgeRecorder, std::string graph_id)
    : FinalResultsJson() {
  PAMM_FACTORY;
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Constructed the analysis controller.";
  BOOST_LOG_SEV(lg, INFO) << "Found the following IR files for this project: ";
  for (auto file : IRDB.getAllSourceFiles()) {
    BOOST_LOG_SEV(lg, INFO) << "\t" << file;
  }
  // Check if the chosen entry points are valid
  BOOST_LOG_SEV(lg, INFO) << "Check for chosen entry points.";
  vector<string> EntryPoints = {"main"};
  if (VariablesMap.count("entry_points")) {
    std::vector<std::string> invalidEntryPoints;
    for (auto &entryPoint : VariablesMap["entry_points"].as<vector<string>>()) {
      if (IRDB.getFunction(entryPoint) == nullptr) {
        invalidEntryPoints.push_back(entryPoint);
      }
    }
    if (invalidEntryPoints.size()) {
      for (auto &invalidEntryPoint : invalidEntryPoints) {
        BOOST_LOG_SEV(lg, ERROR)
            << "Entry point '" << invalidEntryPoint << "' is not valid.";
      }
      throw logic_error("invalid entry points");
    }
    if (VariablesMap["entry_points"].as<vector<string>>().size()) {
      EntryPoints = VariablesMap["entry_points"].as<vector<string>>();
    }
  }
  if (WPA_MODE) {
    // here we link every llvm module into a single module containing the entire
    // IR
    BOOST_LOG_SEV(lg, INFO)
        << "link all llvm modules into a single module for WPA ...\n";
    // START_TIMER("Link to WPA Module");
    IRDB.linkForWPA();
    // STOP_TIMER("Link to WPA Module");
  }
  IRDB.preprocessIR();
  // START_TIMER("DB Start Up");
  // DBConn &db = DBConn::getInstance();
  // STOP_TIMER("DB Start Up");
  // START_TIMER("DB Store IRDB");
  // db.storeProjectIRDB("myphasarproject", IRDB);
  // STOP_TIMER("DB Store IRDB");
  // Reconstruct the inter-modular class hierarchy and virtual function tables
  BOOST_LOG_SEV(lg, INFO) << "Reconstruct the class hierarchy.";
  START_TIMER("LTH Construction");
  LLVMTypeHierarchy CH(IRDB);
  STOP_TIMER("LTH Construction");
  BOOST_LOG_SEV(lg, INFO) << "Reconstruction of class hierarchy completed.";
  // START_TIMER("DB Store LTH");
  // db.storeLLVMTypeHierarchy(CH,"myphasarproject");
  // STOP_TIMER("DB Store LTH");
  // CH.printAsDot();
  FinalResultsJson += CH.getAsJson();
  if (VariablesMap.count("classhierarchy_analysis")) {
    CH.print();
    CH.printAsDot("ch.dot");
  }
  // Call graph construction stategy
  CallGraphAnalysisType CGType(
      (VariablesMap.count("callgraph_analysis"))
          ? StringToCallGraphAnalysisType.at(
                VariablesMap["callgraph_analysis"].as<string>())
          : CallGraphAnalysisType::OTF);
  WalkerStrategy CGWalker;
  ResolveStrategy CGResolve;
  switch (CGType) {
  case CallGraphAnalysisType::CHA:
    CGWalker = WalkerStrategy::Simple;
    CGResolve = ResolveStrategy::CHA;
    break;
  case CallGraphAnalysisType::RTA:
    CGWalker = WalkerStrategy::Simple;
    CGResolve = ResolveStrategy::RTA;
    break;
  case CallGraphAnalysisType::DTA:
    CGWalker = WalkerStrategy::DeclaredType;
    CGResolve = ResolveStrategy::TA;
    break;
  case CallGraphAnalysisType::VTA:
    CGWalker = WalkerStrategy::VariableType;
    CGResolve = ResolveStrategy::TA;
    break;
  case CallGraphAnalysisType::OTF:
    CGWalker = WalkerStrategy::Pointer;
    CGResolve = ResolveStrategy::OTF;
    break;
  }
  // Perform whole program analysis (WPA) analysis
  if (WPA_MODE) {
    START_TIMER("ICFG Construction");
    LLVMBasedICFG ICFG(CH, IRDB, CGWalker, CGResolve, EntryPoints);

    if (VariablesMap.count("callgraph_plugin")) {
      // TODO write a lambda to replace the built-in callgraph with the
      // callgraph provided by the plugin
      SOL so(VariablesMap["callgraph_plugin"].as<string>());
    }
    STOP_TIMER("ICFG Construction");
    ICFG.printAsDot("call_graph.dot");
    // Add the ICFG to final results
    FinalResultsJson += ICFG.getAsJson();
    if (VariablesMap.count("callgraph_analysis")) {
      ICFG.print();
      ICFG.printAsDot("icfg.dot");
    }
    FinalResultsJson += ICFG.getWholeModulePTG().getAsJson();
    if (VariablesMap.count("pointer_analysis")) {
      ICFG.getWholeModulePTG().print();
      ICFG.getWholeModulePTG().printAsDot("wptg.dot");
    }
    // CFG is only needed for intra-procedural monotone framework
    LLVMBasedCFG CFG;
    /*
     * Perform all the analysis that the user has chosen.
     */
    for (DataFlowAnalysisType analysis : Analyses) {
      BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis;
      START_TIMER("DFA Runtime");
      switch (analysis) {
      case DataFlowAnalysisType::IFDS_TaintAnalysis: {
        IFDSTaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtaintsolver(
            taintanalysisproblem, true);
        llvmtaintsolver.solve();
        FinalResultsJson += llvmtaintsolver.getAsJson();
        // Here we can get the leaks
        map<const llvm::Instruction *, set<const llvm::Value *>> Leaks =
            taintanalysisproblem.Leaks;
        BOOST_LOG_SEV(lg, INFO) << "Found the following leaks:";
        if (Leaks.empty()) {
          BOOST_LOG_SEV(lg, INFO) << "No leaks found!";
        } else {
          for (auto Leak : Leaks) {
            string ModuleName =
                getModuleFromVal(Leak.first)->getModuleIdentifier();
            BOOST_LOG_SEV(lg, INFO)
                << "At instruction: '" << llvmIRToString(Leak.first)
                << "' in file: '" << ModuleName << "'";
            for (auto LeakValue : Leak.second) {
              BOOST_LOG_SEV(lg, INFO) << llvmIRToString(LeakValue);
            }
          }
        }
        break;
      }
      case DataFlowAnalysisType::IDE_TaintAnalysis: {
        IDETaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, const llvm::Value *, LLVMBasedICFG &>
            llvmtaintsolver(taintanalysisproblem, true);
        llvmtaintsolver.solve();
        FinalResultsJson += llvmtaintsolver.getAsJson();
        break;
      }
      case DataFlowAnalysisType::IDE_TypeStateAnalysis: {
        IDETypeStateAnalysis typestateproblem(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, State, LLVMBasedICFG &>
            llvmtypestatesolver(typestateproblem, true);
        llvmtypestatesolver.solve();
        FinalResultsJson += llvmtypestatesolver.getAsJson();
        break;
      }
      case DataFlowAnalysisType::IFDS_TypeAnalysis: {
        IFDSTypeAnalysis typeanalysisproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtypesolver(
            typeanalysisproblem, true);
        llvmtypesolver.solve();
        FinalResultsJson += llvmtypesolver.getAsJson();
        break;
      }
      case DataFlowAnalysisType::IFDS_UninitializedVariables: {
        IFDSUnitializedVariables uninitializedvarproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmunivsolver(
            uninitializedvarproblem, true);
        llvmunivsolver.solve();
        FinalResultsJson += llvmunivsolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmunivsolver.exportJSONDataModel(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_LinearConstantAnalysis: {
        IFDSLinearConstantAnalysis lcaproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<LCAPair, LLVMBasedICFG &> llvmlcasolver(lcaproblem,
                                                               true);
        llvmlcasolver.solve();
        FinalResultsJson += llvmlcasolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmlcasolver.exportJSONDataModel(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IDE_LinearConstantAnalysis: {
        IDELinearConstantAnalysis lcaproblem(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmlcasolver(
            lcaproblem, true);
        llvmlcasolver.solve();
        FinalResultsJson += llvmlcasolver.getAsJson();
        break;
      }
      case DataFlowAnalysisType::IFDS_ConstAnalysis: {
        IFDSConstAnalysis constproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
            constproblem, true);
        llvmconstsolver.solve();
        FinalResultsJson += llvmconstsolver.getAsJson();
        constproblem.printInitMemoryLocations();
        REG_COUNTER_WITH_VALUE("Const Init Set Size",
                               constproblem.initMemoryLocationCount());
        constproblem.printInitMemoryLocations();
        START_TIMER("DFA Result Computation");
        // TODO need to consider object fields, i.e. getelementptr instructions
        // get all stack and heap alloca instructions
        std::set<const llvm::Value *> allMemoryLoc =
            IRDB.getAllocaInstructions();
        std::set<std::string> IgnoredGlobalNames = {"llvm.used",
                                                    "llvm.compiler.used",
                                                    "llvm.global_ctors",
                                                    "llvm.global_dtors",
                                                    "vtable",
                                                    "typeinfo"};
        // add global varibales to the memory location set, except the llvm
        // intrinsic global variables
        for (auto M : IRDB.getAllModules()) {
          for (auto &GV : M->globals()) {
            if (GV.hasName()) {
              string GVName = cxx_demangle(GV.getName().str());
              if (!IgnoredGlobalNames.count(
                      GVName.substr(0, GVName.find(' ')))) {
                allMemoryLoc.insert(&GV);
              }
            }
          }
        }
        BOOST_LOG_SEV(lg, DEBUG) << "-------------";
        BOOST_LOG_SEV(lg, DEBUG) << "Allocation Instructions:";
        for (auto memloc : allMemoryLoc) {
          BOOST_LOG_SEV(lg, DEBUG) << llvmIRToString(memloc);
        }
        BOOST_LOG_SEV(lg, DEBUG) << "-------------";
        BOOST_LOG_SEV(lg, DEBUG)
            << "Printing return/resume instruction + dataflow facts:";
        for (auto RR : IRDB.getRetResInstructions()) {
          std::set<const llvm::Value *> facts =
              llvmconstsolver.ifdsResultsAt(RR);
          // Empty facts means the return/resume statement is part of not
          // analyzed function - remove all allocas of that function
          if (facts.empty()) {
            const llvm::Function *F = RR->getParent()->getParent();
            for (auto mem_itr = allMemoryLoc.begin();
                 mem_itr != allMemoryLoc.end();) {
              if (auto Inst = llvm::dyn_cast<llvm::Instruction>(*mem_itr)) {
                if (Inst->getParent()->getParent() == F) {
                  mem_itr = allMemoryLoc.erase(mem_itr);
                } else {
                  ++mem_itr;
                }
              } else {
                ++mem_itr;
              }
            }
          } else {
            BOOST_LOG_SEV(lg, DEBUG) << "Instruction: " << llvmIRToString(RR);
            for (auto fact : facts) {
              if (isAllocaInstOrHeapAllocaFunction(fact) ||
                  llvm::isa<llvm::GlobalValue>(fact)) {
                BOOST_LOG_SEV(lg, DEBUG)
                    << "   Fact: " << constproblem.DtoString(fact);
                // remove allocas that are mutable, i.e. are valid facts
                allMemoryLoc.erase(fact);
              }
            }
          }
        }
        // write immutable locations to file
        bfs::path cfp(VariablesMap["config"].as<string>());
        // reduce the config path to just the filename - no path and no
        // extension
        std::string config = cfp.filename().string();
        std::size_t extensionPos = config.find(cfp.extension().string());
        config.replace(extensionPos, cfp.extension().size(), "");
        ofstream ResultFile;
        ResultFile.open(config + "_memlocs.txt");
        // BOOST_LOG_SEV(lg, INFO) << "-------------";
        // BOOST_LOG_SEV(lg, INFO) << "Immutable Stack/Heap Memory";
        for (auto memloc : allMemoryLoc) {
          if (auto memlocInst = llvm::dyn_cast<llvm::Instruction>(memloc)) {
            ResultFile << llvmIRToString(memlocInst) << " in function "
                       << memlocInst->getParent()->getParent()->getName().str()
                       << "\n";
          } else {
            ResultFile << llvmIRToString(memloc) << '\n';
          }
        }
        ResultFile.close();
        STOP_TIMER("DFA Result Computation");
        BOOST_LOG_SEV(lg, INFO) << "-------------";
        break;
      }
      case DataFlowAnalysisType::IFDS_SolverTest: {
        IFDSSolverTest ifdstest(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmifdstestsolver(
            ifdstest, true);
        llvmifdstestsolver.solve();
        FinalResultsJson += llvmifdstestsolver.getAsJson();
        break;
      }
      case DataFlowAnalysisType::IDE_SolverTest: {
        IDESolverTest idetest(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, const llvm::Value *, LLVMBasedICFG &>
            llvmidetestsolver(idetest, true);
        llvmidetestsolver.solve();
        llvmidetestsolver.getAsJson();
        break;
      }
      case DataFlowAnalysisType::MONO_Intra_FullConstantPropagation: {
        const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
        IntraMonoFullConstantPropagation intra(CFG, F);
        LLVMIntraMonotoneSolver<pair<const llvm::Value *, unsigned>,
                                LLVMBasedCFG &>
            solver(intra, true);
        solver.solve();
        break;
      }
      case DataFlowAnalysisType::MONO_Intra_SolverTest: {
        const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
        IntraMonotoneSolverTest intra(CFG, F);
        LLVMIntraMonotoneSolver<const llvm::Value *, LLVMBasedCFG &> solver(
            intra, true);
        solver.solve();
        break;
      }
      case DataFlowAnalysisType::MONO_Inter_SolverTest: {
        // InterMonotoneSolverTest inter(ICFG, EntryPoints);
        // //WARNING For testing purpose LLVMBasedICFG & -> LLVMBasedICFG
        // LLVMInterMonotoneSolver<const llvm::Value *, 3, LLVMBasedICFG> solver(
        //     inter, true);
        // solver.solve();
        break;
      }
      case DataFlowAnalysisType::Plugin: {
        vector<string> AnalysisPlugins =
            VariablesMap["analysis_plugin"].as<vector<string>>();
#ifdef PHASAR_PLUGINS_ENABLED
        AnalysisPluginController PluginController(
            AnalysisPlugins, ICFG, EntryPoints, FinalResultsJson);
#endif
        break;
      }
      case DataFlowAnalysisType::None: {
        break;
      }
      default:
        BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
        break;
      }
      STOP_TIMER("DFA Runtime");
    }
  }
  // Perform module-wise (MW) analysis
  else {
    map<const llvm::Module *, LLVMBasedICFG> MWICFGs;
    IDESummaries<const llvm::Instruction *, const llvm::Value *,
                 const llvm::Function *, BinaryDomain>
        Summaries;
    // We build all the call- and points-to graphs which can be used for
    // all of the analysis of course.
    BOOST_LOG_SEV(lg, INFO) << "Building module-wise icfgs";
    for (auto M : IRDB.getAllModules()) {
      BOOST_LOG_SEV(lg, INFO)
          << "Construct (partial) call-graph for: " << M->getModuleIdentifier();
      LLVMBasedICFG ICFG(CH, IRDB, *M, CGWalker, CGResolve);
      // store them away for later use

      //WARNING Doesn't store it, just copy it (just delete the copy constructor of ICFG to see it)
      //WARNING Possibly a huge performance lost
      //NOTE Should try to use move semantic or to use proper pointer
      // ex: MWICFGs.emplace(std::piecewise_construct,
      //         std::forward_as_tuple(M),
      //         std::forward_as_tuple(CH, IRDB, *M, CGWalker, CGResolve));
      // should work better (construct directly the object in the map, just check
      // that there is no copy of CH, IRDB, ... but normally, it's just reference)
      // MWICFGs.insert(make_pair(M, ICFG));
      MWICFGs.emplace(std::piecewise_construct,
              std::forward_as_tuple(M),
              std::forward_as_tuple(CH, IRDB, *M, CGWalker, CGResolve));
    }
    BOOST_LOG_SEV(lg, INFO) << "Call-graphs have been constructed";
    // Perform all the analysis that the user has chosen.
    for (DataFlowAnalysisType analysis : Analyses) {
      // switch to create the module wise results (TODO the code needs
      // improvement!)
      switch (analysis) {
      case DataFlowAnalysisType::IFDS_TaintAnalysis: {
        for (auto &entry : MWICFGs) {
          const llvm::Module *M = entry.first;
          LLVMBasedICFG &I = entry.second;
          // Only for modules which do not contain 'main'
          if (M->getFunction("main") == nullptr ||
              M->getFunction("main")->isDeclaration()) {
            BOOST_LOG_SEV(lg, INFO)
                << "Performing analysis: " << analysis
                << " on module: " << M->getModuleIdentifier();
            IFDSTaintAnalysis taintanalysisproblem(I, {});
            LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
                llvmtaintsolver(taintanalysisproblem,
                                SummaryGenerationStrategy::always_none, false);
            llvmtaintsolver.summarize();
            Summaries.addSummaries(llvmtaintsolver.getSummaries());
            BOOST_LOG_SEV(lg, INFO) << "Generated summaries!";
          }
        }
        break;
      }
      case DataFlowAnalysisType::IDE_TaintAnalysis: {
        throw invalid_argument("IDE summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::IFDS_TypeAnalysis: {
        throw invalid_argument("IFDS summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::IFDS_UninitializedVariables: {
        for (auto &entry : MWICFGs) {
          const llvm::Module *M = entry.first;
          LLVMBasedICFG &I = entry.second;
          // Only for modules which do not contain 'main'
          if (M->getFunction("main") == nullptr ||
              M->getFunction("main")->isDeclaration()) {
            BOOST_LOG_SEV(lg, INFO)
                << "Performing analysis: " << analysis
                << " on module: " << M->getModuleIdentifier();
            IFDSUnitializedVariables uninitproblem(I, {});
            LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
                llvmuninitsolver(uninitproblem,
                                 SummaryGenerationStrategy::always_none, false);
            llvmuninitsolver.summarize();
            Summaries.addSummaries(llvmuninitsolver.getSummaries());
            BOOST_LOG_SEV(lg, INFO) << "Generated summaries!";
          }
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_SolverTest: {
        throw invalid_argument("IFDS summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::IDE_SolverTest: {
        throw invalid_argument("IDE summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::MONO_Intra_FullConstantPropagation: {
        throw invalid_argument("Mono summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::MONO_Intra_SolverTest: {
        throw invalid_argument("Mono summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::MONO_Inter_SolverTest: {
        throw invalid_argument("Mono summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::Plugin: {
        throw invalid_argument("Plugin summary generation not supported yet");
        break;
      }
      case DataFlowAnalysisType::None: {
        // just return
        return;
        break;
      }
      default:
        BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
        break;
      }
      // switch for combining the results (TODO the code needs improvement,
      // currently just a hack!)
      // After every module has been analyzed the analyses results must be
      // merged and the final results must be computed
      switch (analysis) {
      case DataFlowAnalysisType::IFDS_TaintAnalysis: {
        auto M = IRDB.getModuleDefiningFunction("main");
        LLVMBasedICFG& I = MWICFGs.at(M);
        BOOST_LOG_SEV(lg, INFO) << "Combining module-wise icfgs";
        for (auto &entry : MWICFGs) {
          if (M != entry.first) {
            I.mergeWith(entry.second);
          }
        }
        I.printAsDot("call_graph.dot");
        BOOST_LOG_SEV(lg, INFO) << "Combining module-wise analysis results\n";
        IFDSTaintAnalysis problem(I, EntryPoints);
        LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &> solver(
            problem, SummaryGenerationStrategy::always_none, true);
        auto tab = Summaries.getSummaries();
        std::cout << "external TAB" << std::endl;
        std::cout << tab << std::endl;
        solver.setSummaries(tab);
        solver.combine();
        BOOST_LOG_SEV(lg, INFO)
            << "Combining module-wise results done, computation completed!";
        BOOST_LOG_SEV(lg, INFO) << "Data-flow analysis completed.";
        break;
      }
      case DataFlowAnalysisType::IFDS_UninitializedVariables: {
        auto M = IRDB.getModuleDefiningFunction("main");
        LLVMBasedICFG &I = MWICFGs.at(M);
        BOOST_LOG_SEV(lg, INFO) << "Combining module-wise icfgs";
        for (auto &entry : MWICFGs) {
          if (M != entry.first) {
            I.mergeWith(entry.second);
          }
        }
        I.printAsDot("call_graph.dot");
        BOOST_LOG_SEV(lg, INFO) << "Combining module-wise analysis results\n";
        IFDSUnitializedVariables problem(I, EntryPoints);
        LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &> solver(
            problem, SummaryGenerationStrategy::always_none, true);
        auto tab = Summaries.getSummaries();
        std::cout << "external TAB" << std::endl;
        std::cout << tab << std::endl;
        solver.setSummaries(tab);
        solver.combine();
        BOOST_LOG_SEV(lg, INFO)
            << "Combining module-wise results done, computation completed!";
        BOOST_LOG_SEV(lg, INFO) << "Data-flow analysis completed.";
        break;
      }
      default:
        throw invalid_argument(
            "Choosen analysis is not supported for MWA analysis, yet!");
        break;
      }
    }
  }
}

void AnalysisController::writeResults(std::string filename) {
  std::ofstream ofs(filename);
  ofs << FinalResultsJson.dump(1);
}

} // namespace psr
