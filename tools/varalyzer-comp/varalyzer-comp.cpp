#include <iostream>
#include <set>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/filesystem/path.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVarTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPCIPHERCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/VarAlyzerExperiments/VarAlyzerUtils.h"

using namespace psr;

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cout << "Usage:\n"
                 "\t<varalyzer>\n"
                 "\t<analysis: \"MD\", \"CIPHER\", \"MAC\">\n"
                 "\t<SuperC-desugared SPL LLVM IR file>\n"
                 "\t<SP_1 LLVM IR file> <...>\n\n";
    return 1;
  }
  // handle command-line arguments
  std::cout << "Hello, VarAlyzerComp!\n";
  std::string AnalysisTypeStr = argv[1];
  boost::filesystem::path DesugeredSPLIRFile = argv[2];
  std::vector<boost::filesystem::path> SPIRFiles;
  for (int i = 3; i < argc; ++i) {
    SPIRFiles.emplace_back(argv[i]);
  }
  // have some rudimentary checks
  if (!(AnalysisTypeStr == "MD" || AnalysisTypeStr == "CIPHER" ||
        AnalysisTypeStr == "MAC" || AnalysisTypeStr == "ALL")) {
    std::cout << "error: analysis type must be one of {MD, CIPHER, MAC, ALL}\n";
    return 1;
  }
  if (!isValidLLVMIRFile(DesugeredSPLIRFile)) {
    std::cout << "error: '" << DesugeredSPLIRFile.string()
              << "' is not a valid LLVM IR file\n";
    return 1;
  }
  for (const auto &SPIRFile : SPIRFiles) {
    if (!isValidLLVMIRFile(SPIRFile)) {
      std::cout << "error: '" << SPIRFile.string()
                << "' is not a valid LLVM IR file\n";
    }
    return 1;
  }
  // constant data
  const OpenSSLEVPAnalysisType AnalysisType =
      to_OpenSSLEVPAnalysisType(AnalysisTypeStr);
  const std::set<std::string> EntryPoints({"main"});
  // compute helper analyses for the desugared IR file
  ProjectIRDB DesugaredIR({DesugeredSPLIRFile.string()}, IRDBOptions::WPA);
  LLVMTypeHierarchy DesugaredTH(DesugaredIR);
  LLVMPointsToSet DesugaredPT(DesugaredIR);
  LLVMBasedVarICFG DesugaredICF(DesugaredIR, CallGraphAnalysisType::OTF,
                                EntryPoints, &DesugaredTH, &DesugaredPT);
  if (AnalysisType == OpenSSLEVPAnalysisType::CIPHER ||
      AnalysisType == OpenSSLEVPAnalysisType::ALL) {
    OpenSSLEVPCIPHERCTXDescription VarCipherCTXDesc;
    IDETypeStateAnalysis VarTSProblem(&DesugaredIR, &DesugaredTH, &DesugaredICF,
                                      &DesugaredPT, VarCipherCTXDesc,
                                      EntryPoints);
    IDEVarTabulationProblem_P<IDETypeStateAnalysis> VarVarProblem(VarTSProblem,
                                                                  DesugaredICF);
    IDESolver VarSolver(VarVarProblem);
    VarSolver.solve();
    // auto VarBreaches = VarVarProblem.getProtocolBreaches();
    // have one large loop that computes all required information for the
    // sampled software products
    for (const auto &SPIRFile : SPIRFiles) {
      // compute results on concrete software products in a
      // variability-oblivious manner
      ProjectIRDB SPIR({SPIRFile.string()}, IRDBOptions::WPA);
      LLVMTypeHierarchy SPTH(SPIR);
      LLVMPointsToSet SPPT(SPIR);
      LLVMBasedICFG SPICF(SPIR, CallGraphAnalysisType::OTF, EntryPoints, &SPTH,
                          &SPPT);
      OpenSSLEVPCIPHERCTXDescription CipherCTXDesc;
      IDETypeStateAnalysis TSProblem(&SPIR, &SPTH, &SPICF, &SPPT, CipherCTXDesc,
                                     EntryPoints);
      IDESolver Solver(TSProblem);
      Solver.solve();
      // do the comparison
      //  (i) clear function name (name of the function in which the error
      //  occurred) (ii) errornous transition (state before error and token that
      //  caused the error (clear name))
      // auto NonVarBreaches = TSProblem.getProtocolBreaches();
      // for () {
      //   check if every NonVarBreach can be found in VarBreaches
      // }
    }
  }
  if (AnalysisType == OpenSSLEVPAnalysisType::MD ||
      AnalysisType == OpenSSLEVPAnalysisType::MAC ||
      AnalysisType == OpenSSLEVPAnalysisType::ALL) {
    OpenSSLEVPMDCTXDescription VarMdCTXDesc;
    IDETypeStateAnalysis VarTSProblem(&DesugaredIR, &DesugaredTH, &DesugaredICF,
                                      &DesugaredPT, VarMdCTXDesc, EntryPoints);
    IDEVarTabulationProblem_P<IDETypeStateAnalysis> VarVarProblem(VarTSProblem,
                                                                  DesugaredICF);
    IDESolver VarSolver(VarVarProblem);
    VarSolver.solve();
    // TODO
    // have one large loop that computes all required information for the
    // sampled
    // software products
    for (const auto &SPIRFile : SPIRFiles) {
      // compute results on concrete software products in a
      // variability-oblivious manner
      ProjectIRDB SPIR({SPIRFile.string()}, IRDBOptions::WPA);
      LLVMTypeHierarchy SPTH(SPIR);
      LLVMPointsToSet SPPT(SPIR);
      LLVMBasedICFG SPICF(SPIR, CallGraphAnalysisType::OTF, EntryPoints, &SPTH,
                          &SPPT);
      OpenSSLEVPMDCTXDescription MdCTXDesc;
      IDETypeStateAnalysis TSProblem(&SPIR, &SPTH, &SPICF, &SPPT, MdCTXDesc,
                                     EntryPoints);
      IDESolver Solver(TSProblem);
      Solver.solve();
      // TODO
      // do the comparison
    }
  }
  return 0;
}
