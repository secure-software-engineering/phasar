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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/VarAlyzerExperiments/VarAlyzerUtils.h"

using namespace psr;

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cout << "Usage:\n"
                 "\t<typestate>\n"
                 "\t<enable logging: l, L>\n"
                 "\t<analysis: \"CIPHER\", \"MAC\", \"MD\">\n"
                 "\t<LLVM IR file>\n";
    return 1;
  }
  // handle command-line arguments
  std::cout << "Hello, Typestate!\n";
  std::string Log = argv[1];
  std::string AnalysisTypeStr = argv[2];
  boost::filesystem::path DesugeredSPLIRFile = argv[3];
  // have some rudimentary checks
  if (!(Log == "l" || Log == "L")) {
    std::cout << "error: logging must be one of {l, L}\n";
    return 1;
  }
  if (!(AnalysisTypeStr == "MD" || AnalysisTypeStr == "CIPHER" ||
        AnalysisTypeStr == "MAC")) {
    std::cout << "error: analysis type must be one of {MD, CIPHER, MAC}\n";
    return 1;
  }
  if (!isValidLLVMIRFile(DesugeredSPLIRFile)) {
    std::cout << "error: '" << DesugeredSPLIRFile.string()
              << "' is not a valid LLVM IR file\n";
    return 1;
  }
  initializeLogger(Log == "L");
  // constant data
  const OpenSSLEVPAnalysisType AnalysisType =
      to_OpenSSLEVPAnalysisType(AnalysisTypeStr);
  // compute helper analyses for the desugared IR file
  ProjectIRDB IR({DesugeredSPLIRFile.string()}, IRDBOptions::WPA);
  auto [ForwardRenaming, BackwardRenaming] = extractBiDiStaticRenaming(&IR);
  LLVMTypeHierarchy TH(IR);
  LLVMPointsToSet PT(IR);
  // by using an empty list of entry points, all functions are considered as
  // entry points
  LLVMBasedICFG ICF(IR, CallGraphAnalysisType::OTF, {}, &TH, &PT);
  if (AnalysisType == OpenSSLEVPAnalysisType::CIPHER) {
    OpenSSLEVPCIPHERCTXDescription CipherCTXDesc;
    auto AnalysisEntryPoints =
        getEntryPointsForCallersOf("EVP_CIPHER_CTX_new", IR, ICF);
    if (AnalysisEntryPoints.empty()) {
      // if AnalysisEntryPoints are empty, we must run on desugared code
      AnalysisEntryPoints = getEntryPointsForCallersOfDesugared(
          "EVP_CIPHER_CTX_new", IR, ICF, ForwardRenaming);
    }
    if (AnalysisEntryPoints.empty()) {
      std::cout << "error: could not retrieve analysis' entry points\n";
      return 1;
    }
    IDETypeStateAnalysis Problem(&IR, &TH, &ICF, &PT, CipherCTXDesc,
                                 AnalysisEntryPoints);
    IDESolver Solver(Problem);
    Solver.solve();
    Solver.dumpResults();
  }
  if (AnalysisType == OpenSSLEVPAnalysisType::MD ||
      AnalysisType == OpenSSLEVPAnalysisType::MAC) {
    OpenSSLEVPMDCTXDescription MdCTXDesc;
    auto AnalysisEntryPoints =
        getEntryPointsForCallersOf("EVP_MD_CTX_new", IR, ICF);
    if (AnalysisEntryPoints.empty()) {
      // if AnalysisEntryPoints are empty, we must run on desugared code
      AnalysisEntryPoints = getEntryPointsForCallersOfDesugared(
          "EVP_MD_CTX_new", IR, ICF, ForwardRenaming);
    }
    if (AnalysisEntryPoints.empty()) {
      std::cout << "error: could not retrieve analysis' entry points\n";
      return 1;
    }
    IDETypeStateAnalysis Problem(&IR, &TH, &ICF, &PT, MdCTXDesc,
                                 AnalysisEntryPoints);
    IDESolver Solver(Problem);
    Solver.solve();
    Solver.dumpResults();
  }
  return 0;
}
