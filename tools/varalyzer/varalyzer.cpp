#include <iostream>
#include <set>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/filesystem/path.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVarTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;

bool isValidLLVMIRFile(const boost::filesystem::path &FilePath) {
  return boost::filesystem::exists(FilePath) &&
         !boost::filesystem::is_directory(FilePath) &&
         (FilePath.extension() == ".ll" || FilePath.extension() == ".bc");
}

std::vector<std::string> makeStringVectorFromPathVector(
    const std::vector<boost::filesystem::path> &Paths) {
  std::vector<std::string> Result;
  Result.reserve(Paths.size());
  for (const auto &Path : Paths) {
    Result.push_back(Path.string());
  }
  return Result;
}

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
  std::cout << "Hello, VarAlyzer!\n";
  std::string AnalysisType = argv[1];
  boost::filesystem::path DesugeredSPLIRFile = argv[2];
  std::vector<boost::filesystem::path> SPIRFiles;
  for (int i = 3; i < argc; ++i) {
    SPIRFiles.emplace_back(argv[i]);
  }
  // have some rudimentary checks
  if (!(AnalysisType == "MD" || AnalysisType == "CIPHER" ||
        AnalysisType == "MAC" || AnalysisType == "ALL")) {
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
  const std::set<std::string> EntryPoints({"main"});
  // compute helper analyses for the desugared IR file
  ProjectIRDB DesugaredIR({DesugeredSPLIRFile.string()}, IRDBOptions::WPA);
  LLVMTypeHierarchy DesugaredTH(DesugaredIR);
  LLVMPointsToSet DesugaredPT(DesugaredIR);
  // compute variability-oblivious results
  LLVMBasedICFG DesugaredVOICFG(DesugaredIR, CallGraphAnalysisType::OTF,
                                EntryPoints, &DesugaredTH, &DesugaredPT);

  // compute variability-aware results
  LLVMBasedVarICFG DesugaredVAICFG(DesugaredIR, CallGraphAnalysisType::OTF,
                                   EntryPoints, &DesugaredTH, &DesugaredPT);

  // compute results on concrete software products
  std::vector<ProjectIRDB> SPIRs;
  std::vector<LLVMTypeHierarchy> SPTHs;
  std::vector<LLVMPointsToSet> SPPTs;
  std::vector<LLVMBasedICFG> SPICFGs;
  // have one large loop that computes all required information for the sampled
  // software products
  //   for (const auto &SPIRFile : SPIRFiles) {
  //     SPIRs.push_back(ProjectIRDB({SPIRFile}, IRDBOptions::WPA));
  //     SPTHs.push_back(LLVMTypeHierarchy(SPIRs.back()));
  //     SPPTs.push_back(LLVMPointsToSet(SPIRs.back()));
  //     SPICFGs.push_back(LLVMBasedICFG(SPIRs.back(),
  //     CallGraphAnalysisType::OTF,
  //                                     EntryPoints, &SPTHs.back(),
  //                                     &SPPTs.back()));
  //   }

  return 0;
}
