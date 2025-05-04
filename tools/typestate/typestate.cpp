#include "phasar/AnalysisStrategy/AnalysisSetup.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPCIPHERCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/VarAlyzerExperiments/VarAlyzerUtils.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Support/CommandLine.h"

#include <set>
#include <string>
#include <vector>

using namespace psr;

namespace cl = llvm::cl;

static cl::OptionCategory TSACat("Typestate Analysis");
static cl::opt<std::string> IRPath(cl::Positional, cl::Required,
                                   cl::cat(TSACat),
                                   cl::desc("The LLVM IR file to analyze"));
static cl::opt<bool> EnableLogging("log", cl::cat(TSACat),
                                   cl::desc("Whether to enable logging"));
static cl::alias EnableLoggingShort("L", cl::aliasopt(EnableLogging));

enum class TypeStateAnalysisKind {
  MessageDigest,
  Cipher,
  MAC,
};

static cl::opt<TypeStateAnalysisKind>
    AnalysisKind("analysis", cl::cat(TSACat), cl::Required,
                 cl::desc("The typestate analysis to run"),
                 cl::ValuesClass{
                     clEnumValN(TypeStateAnalysisKind::Cipher, "cipher",
                                "Tracking usage of OpenSSL's EVP_CIPHER_CTX"),
                     clEnumValN(TypeStateAnalysisKind::MessageDigest, "md",
                                "Tracking usage of OpenSSL's EVP_MD_CTX"),
                     clEnumValN(TypeStateAnalysisKind::MessageDigest, "mac",
                                "Currently the same as 'md'"),
                 });

int main(int argc, char **argv) {
  cl::HideUnrelatedOptions(TSACat);
  cl::ParseCommandLineOptions(argc, argv);

  if (EnableLogging) {
    Logger::initializeStderrLogger();
  }

  // compute helper analyses for the desugared IR file
  LLVMProjectIRDB IR(IRPath);
  if (!IR) {
    return 1;
  }
  auto [ForwardRenaming, BackwardRenaming] = extractBiDiStaticRenaming(&IR);
  DIBasedTypeHierarchy TH(IR);
  LLVMAliasSet PT(&IR);

  // by using an empty list of entry points, all functions are considered as
  // entry points
  LLVMBasedICFG ICF(&IR, CallGraphAnalysisType::OTF, {}, &TH, &PT);
  if (AnalysisKind == TypeStateAnalysisKind::Cipher) {
    auto typeNameOfInterest =
        ForwardRenaming.empty() ? "evp_cipher_ctx_st"
                                : extractDesugaredTypeNameOfInterestOrFail(
                                      "EVP_CIPHER_CTX", IR, ForwardRenaming,
                                      "error: could not retrieve desugared "
                                      "typenameOfInterest for EVP_CIPHER_CTX");
    // if (typeNameOfInterest == "") {
    //   return 0;
    // }
    OpenSSLEVPCIPHERCTXDescription CipherCTXDesc(
        ForwardRenaming.empty() ? nullptr : &ForwardRenaming,
        typeNameOfInterest);
    auto AnalysisEntryPoints = getEntryPointsForCallersOfDesugared(
        "EVP_CIPHER_CTX_new", IR, ICF, ForwardRenaming, typeNameOfInterest);

    if (AnalysisEntryPoints.empty()) {
      // std::cerr << "warning: could not retrieve analysis' entry points
      // because "
      //  "the module does not use the EVP library\n";
      return 0;
    }
    IDETypeStateAnalysis Problem(&IR, &PT, &CipherCTXDesc, AnalysisEntryPoints);
    IDESolver Solver(Problem, &ICF);
    Solver.solve();
    Solver.dumpResults();
  }
  if (AnalysisKind == TypeStateAnalysisKind::MessageDigest ||
      AnalysisKind == TypeStateAnalysisKind::MAC) {
    auto typeNameOfInterest = ForwardRenaming.empty()
                                  ? "evp_md_ctx_st"
                                  : extractDesugaredTypeNameOfInterestOrFail(
                                        "EVP_MD_CTX", IR, ForwardRenaming,
                                        "error: could not retrieve desugared "
                                        "typenameOfInterest for EVP_MD_CTX");
    // if (typeNameOfInterest.empty()) {
    //   return 0;
    // }
    OpenSSLEVPMDCTXDescription MdCTXDesc(
        ForwardRenaming.empty() ? nullptr : &ForwardRenaming,
        typeNameOfInterest);
    auto AnalysisEntryPoints = getEntryPointsForCallersOfDesugared(
        "EVP_MD_CTX_new", IR, ICF, ForwardRenaming,
        MdCTXDesc.getTypeNameOfInterest());

    if (AnalysisEntryPoints.empty()) {
      // std::cerr << "warning: could not retrieve analysis' entry points
      // because "
      //  "the module does not use the EVP library\n";
      return 0;
    }
    IDETypeStateAnalysis Problem(&IR, &PT, &MdCTXDesc, AnalysisEntryPoints);
    IDESolver Solver(Problem, &ICF);
    Solver.solve();
    Solver.dumpResults();
  }
  return 0;
}
