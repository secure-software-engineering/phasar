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
#include "phasar/Utils/Logger.h"
#include "phasar/VarAlyzerExperiments/VarAlyzerUtils.h"

using namespace psr;

int main(int argc, char **argv) {
  if (argc < 5) {
    std::cerr << "Usage:\n"
                 "\t<varalyzer-comp>\n"
                 "\t<enable logging: l, L>\n"
                 "\t<analysis: \"CIPHER\", \"MAC\", \"MD\">\n"
                 "\t<SuperC-desugared SPL LLVM IR file>\n"
                 "\t<SP_1 LLVM IR file> <...>\n"
              << std::endl;
    return 1;
  }
  // handle command-line arguments
  std::cout << "Hello, VarAlyzerComp!" << std::endl;
  std::string Log = argv[1];
  std::string AnalysisTypeStr = argv[2];
  boost::filesystem::path DesugeredSPLIRFile = argv[3];
  std::vector<boost::filesystem::path> SPIRFiles;
  for (int i = 4; i < argc; ++i) {
    SPIRFiles.emplace_back(argv[i]);
  }
  // have some rudimentary checks
  if (!(Log == "l" || Log == "L")) {
    std::cerr << "error: logging must be one of {l, L}\n";
    return 1;
  }
  if (!(AnalysisTypeStr == "MAC" || AnalysisTypeStr == "CIPHER" ||
        AnalysisTypeStr == "MD")) {
    std::cerr << "error: analysis type must be one of {MAC, MD, CIPHER}\n";
    return 1;
  }
  if (!isValidLLVMIRFile(DesugeredSPLIRFile)) {
    std::cerr << "error: '" << DesugeredSPLIRFile.string()
              << "' is not a valid LLVM IR file" << std::endl;
    return 1;
  }
  for (const auto &SPIRFile : SPIRFiles) {
    if (!isValidLLVMIRFile(SPIRFile)) {
      std::cerr << "error: '" << SPIRFile.string()
                << "' is not a valid LLVM IR file" << std::endl;
      return 1;
    }
  }
  initializeLogger(Log == "L");
  // constant data
  const OpenSSLEVPAnalysisType AnalysisType =
      to_OpenSSLEVPAnalysisType(AnalysisTypeStr);
  // compute helper analyses for the desugared IR file
  ProjectIRDB DesugaredIR({DesugeredSPLIRFile.string()}, IRDBOptions::WPA);
  auto [ForwardRenaming, BackwardRenaming] =
      extractBiDiStaticRenaming(&DesugaredIR);
  LLVMTypeHierarchy DesugaredTH(DesugaredIR);
  LLVMPointsToSet DesugaredPT(DesugaredIR);
  LLVMBasedVarICFG DesugaredICF(DesugaredIR, CallGraphAnalysisType::OTF, {},
                                &DesugaredTH, &DesugaredPT, &BackwardRenaming);

  int numViolations = 0;

  if (AnalysisType == OpenSSLEVPAnalysisType::CIPHER) {
    auto typenameOfInterest = extractDesugaredTypeNameOfInterestOrFail(
        "EVP_CIPHER_CTX", DesugaredIR, ForwardRenaming,
        "error: analysis target EVP_CIPHER_CTX not found in the LLVM IR "
        "file\n");
    OpenSSLEVPCIPHERCTXDescription VarCipherCTXDesc(&ForwardRenaming,
                                                    typenameOfInterest);
    auto VarAnalysisEntryPoints = getEntryPointsForCallersOfDesugared(
        "EVP_CIPHER_CTX_new", DesugaredIR, DesugaredICF, ForwardRenaming,
        typenameOfInterest);
    if (VarAnalysisEntryPoints.empty()) {
      std::cerr << "warning: could not retrieve analysis' entry points because "
                   "the module does not use the EVP library: "
                << DesugeredSPLIRFile.string() << "\n";
      return 0;
    }
    IDETypeStateAnalysis VarTSProblem(&DesugaredIR, &DesugaredTH, &DesugaredICF,
                                      &DesugaredPT, VarCipherCTXDesc,
                                      VarAnalysisEntryPoints);
    IDEVarTabulationProblem_P<IDETypeStateAnalysis> VarVarProblem(VarTSProblem,
                                                                  DesugaredICF);
    IDESolver VarSolver(VarVarProblem);
    VarSolver.solve();
    VarSolver.dumpResults();
    auto VarBreaches = VarTSProblem.getProtocolBreaches();
    // have one large loop that computes all required information for the
    // sampled software products
    for (const auto &SPIRFile : SPIRFiles) {
      // compute results on concrete software products in a
      // variability-oblivious manner
      ProjectIRDB SPIR({SPIRFile.string()}, IRDBOptions::WPA);
      LLVMTypeHierarchy SPTH(SPIR);
      LLVMPointsToSet SPPT(SPIR);
      LLVMBasedICFG SPICF(SPIR, CallGraphAnalysisType::OTF, {}, &SPTH, &SPPT);
      OpenSSLEVPCIPHERCTXDescription CipherCTXDesc;
      auto AnalysisEntryPoints =
          getEntryPointsForCallersOf("EVP_CIPHER_CTX_new", SPIR, SPICF,
                                     CipherCTXDesc.getTypeNameOfInterest());
      if (AnalysisEntryPoints.empty()) {
        std::cerr
            << "warning: could not retrieve analysis' entry points because "
               "the module does not use the EVP library: "
            << SPIRFile.string() << "\n";
        continue;
      }
      IDETypeStateAnalysis TSProblem(&SPIR, &SPTH, &SPICF, &SPPT, CipherCTXDesc,
                                     AnalysisEntryPoints);
      IDESolver Solver(TSProblem);
      Solver.solve();
      Solver.dumpResults();
      // do the comparison
      //  (i) clear function name (name of the function in which the error
      //  occurred) (ii) errornous transition (state before error and token that
      //  caused the error (clear name))
      auto NonVarBreaches = TSProblem.getProtocolBreaches();
      for (const auto &NonVarBreach : NonVarBreaches) {
        // check if every NonVarBreach can be found in VarBreaches
        if (!VarBreaches.count(NonVarBreach)) {
          std::cerr << "error: Did not find NonVarBreach " << NonVarBreach
                    << " in VarBreaches\n";
          numViolations++;
        }
      }
    }
  }
  if (AnalysisType == OpenSSLEVPAnalysisType::MD ||
      AnalysisType == OpenSSLEVPAnalysisType::MAC) {
    auto typenameOfInterest = extractDesugaredTypeNameOfInterestOrFail(
        "EVP_MD_CTX", DesugaredIR, ForwardRenaming,
        "error: analysis target EVP_MD_CTX not found in the LLVM IR "
        "file\n");
    OpenSSLEVPMDCTXDescription VarMdCTXDesc(&ForwardRenaming,
                                            typenameOfInterest);
    auto VarAnalysisEntryPoints = getEntryPointsForCallersOfDesugared(
        "EVP_MD_CTX_new", DesugaredIR, DesugaredICF, ForwardRenaming,
        typenameOfInterest);
    if (VarAnalysisEntryPoints.empty()) {
      std::cerr << "warning: could not retrieve analysis' entry points because "
                   "the module does not use the EVP library: "
                << DesugeredSPLIRFile.string() << "\n";
      return 0;
    }
    IDETypeStateAnalysis VarTSProblem(&DesugaredIR, &DesugaredTH, &DesugaredICF,
                                      &DesugaredPT, VarMdCTXDesc,
                                      VarAnalysisEntryPoints);
    IDEVarTabulationProblem_P<IDETypeStateAnalysis> VarVarProblem(VarTSProblem,
                                                                  DesugaredICF);
    IDESolver VarSolver(VarVarProblem);
    VarSolver.solve();
    VarSolver.dumpResults();
    auto VarBreaches = VarTSProblem.getProtocolBreaches();
    // have one large loop that computes all required information for the
    // sampled software products
    for (const auto &SPIRFile : SPIRFiles) {
      // compute results on concrete software products in a
      // variability-oblivious manner
      ProjectIRDB SPIR({SPIRFile.string()}, IRDBOptions::WPA);
      LLVMTypeHierarchy SPTH(SPIR);
      LLVMPointsToSet SPPT(SPIR);
      LLVMBasedICFG SPICF(SPIR, CallGraphAnalysisType::OTF, {}, &SPTH, &SPPT);
      OpenSSLEVPMDCTXDescription MdCTXDesc;
      auto AnalysisEntryPoints = getEntryPointsForCallersOf(
          "EVP_MD_CTX_new", SPIR, SPICF, MdCTXDesc.getTypeNameOfInterest());
      if (AnalysisEntryPoints.empty()) {
        std::cerr
            << "warning: could not retrieve analysis' entry points because "
               "the module does not use the EVP library: "
            << SPIRFile.string() << "\n";
        continue;
      }
      IDETypeStateAnalysis TSProblem(&SPIR, &SPTH, &SPICF, &SPPT, MdCTXDesc,
                                     AnalysisEntryPoints);
      IDESolver Solver(TSProblem);
      Solver.solve();
      Solver.dumpResults();
      // do the comparison
      auto NonVarBreaches = TSProblem.getProtocolBreaches();
      for (const auto &NonVarBreach : NonVarBreaches) {
        // check if every NonVarBreach can be found in VarBreaches
        if (!VarBreaches.count(NonVarBreach)) {
          std::cerr << "error: Did not find NonVarBreach " << NonVarBreach
                    << " in VarBreaches\n";
          numViolations++;
        }
      }
    }
  }
  std::cerr.flush();
  std::cout << "completed" << std::endl;
  return numViolations;
}
