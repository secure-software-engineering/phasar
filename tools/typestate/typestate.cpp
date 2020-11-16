#include <iostream>
#include <set>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/filesystem/path.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPCIPHERCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/VarAlyzerExperiments/VarAlyzerUtils.h"

using namespace psr;

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Usage:\n"
                 "\t<varalyzer>\n"
                 "\t<analysis: \"MD\", \"CIPHER\", \"MAC\">\n"
                 "\t<SuperC-desugared SPL LLVM IR file>\n";
    return 1;
  }
  // handle command-line arguments
  std::cout << "Hello, Typestate!\n";
  std::string AnalysisTypeStr = argv[1];
  boost::filesystem::path DesugeredSPLIRFile = argv[2];
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
  // constant data
  const OpenSSLEVPAnalysisType AnalysisType =
      to_OpenSSLEVPAnalysisType(AnalysisTypeStr);
  const std::set<std::string> EntryPoints({"main"});
  // compute helper analyses for the desugared IR file
  ProjectIRDB IR({DesugeredSPLIRFile.string()}, IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IR);
  LLVMPointsToSet PT(IR);
  LLVMBasedICFG ICF(IR, CallGraphAnalysisType::OTF, EntryPoints, &TH, &PT);
  if (AnalysisType == OpenSSLEVPAnalysisType::CIPHER ||
      AnalysisType == OpenSSLEVPAnalysisType::ALL) {
    OpenSSLEVPCIPHERCTXDescription CipherCTXDesc;
    IDETypeStateAnalysis Problem(&IR, &TH, &ICF, &PT, CipherCTXDesc,
                                 EntryPoints);
    IDESolver Solver(Problem);
    Solver.solve();
  }
  if (AnalysisType == OpenSSLEVPAnalysisType::MD ||
      AnalysisType == OpenSSLEVPAnalysisType::MAC ||
      AnalysisType == OpenSSLEVPAnalysisType::ALL) {
    OpenSSLEVPMDCTXDescription MdCTXDesc;
    IDETypeStateAnalysis Problem(&IR, &TH, &ICF, &PT, MdCTXDesc, EntryPoints);
    IDESolver Solver(Problem);
    Solver.solve();
  }
  return 0;
}
