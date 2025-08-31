#include "phasar/AnalysisStrategy/AnalysisSetup.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/IDEVarTabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/Solver/GenericSolverResults.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/VarEdgeFunctions.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPCIPHERCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/PhasarLLVM/Utils/LLVMSourceManager.h"
#include "phasar/PhasarLLVM/VarAlyzerExperiments/VarAlyzerUtils.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Support/CommandLine.h"

#include <set>
#include <string>
#include <vector>

using namespace psr;

namespace cl = llvm::cl;

static cl::OptionCategory TSACat("Variability-Aware Typestate Analysis");
static cl::opt<std::string> IRPath(cl::Positional, cl::Required,
                                   cl::cat(TSACat),
                                   cl::desc("The LLVM IR file to analyze"));
static cl::opt<bool> EnableLogging("log", cl::cat(TSACat),
                                   cl::desc("Whether to enable logging"));
static cl::alias EnableLoggingShort("L", cl::aliasopt(EnableLogging));

static cl::opt<bool>
    EmitRawResults("emit-raw-results",
                   cl::desc("Dump the raw IDE solver state, instead of "
                            "printing a more condensed text report"));

namespace {
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

template <typename SolverResultsT, typename DescT>
void emitTextReport(const LLVMProjectIRDB &IRDB, SolverResultsT &&SR,
                    DescT &Desc) {
  LLVMSourceManager SrcMgr;

  llvm::DenseSet<const llvm::Value *> Seen;

  for (const auto *I : IRDB.getAllInstructions()) {
    if (I->isDebugOrPseudoInst()) {
      continue;
    }
    const auto &Results = SR.resultsAt(I);

    for (const auto &[Res, CondL] : Results) {
      if (!llvm::isa<llvm::AllocaInst>(Res)) {
        continue;
      }

      bool HasError = false;
      for (const auto &[Cond, TState] : CondL) {
        if (TState == Desc.error() && (HasError || Seen.insert(Res).second)) {
          HasError = true;
          // ERROR STATE DETECTED
          const auto *Inst = I->getPrevNonDebugInstruction()
                                 ? I->getPrevNonDebugInstruction()
                                 : I;
          if (auto Loc = SrcMgr.getDebugLocation(Inst)) {
            auto VarName = psr::getVarNameFromIR(Res);
            if (VarName.empty()) {
              VarName = llvmIRToString(Res);
            }
            SrcMgr.print(llvm::errs(), *Loc, llvm::SourceMgr::DK_Warning,
                         "Detected type-state error for object '" +
                             llvm::Twine(VarName) +
                             "' under pre-processor condition '" +
                             llvm::Twine(to_string(Cond)) + "'");
          } else {
            llvm::errs() << "Detected type-state error at " << llvmIRToString(I)
                         << " for object '" << llvmIRToShortString(Res)
                         << "' under pre-processor condition '"
                         << to_string(Cond) << "'\n";
          }
        }
      }
    }
  }
}

template <TypeStateAnalysisKind TSAKind>
auto getTypeNameOfInterest(const LLVMProjectIRDB &IRDB,
                           const stringstringmap_t &ForwardRenaming) {
  if constexpr (TSAKind == TypeStateAnalysisKind::Cipher) {
    return ForwardRenaming.empty()
               ? "evp_cipher_ctx_st"
               : extractDesugaredTypeNameOfInterestOrFail(
                     "EVP_CIPHER_CTX", IRDB, ForwardRenaming,
                     "error: could not retrieve desugared "
                     "typenameOfInterest for EVP_CIPHER_CTX");
  } else {
    return ForwardRenaming.empty() ? "evp_md_ctx_st"
                                   : extractDesugaredTypeNameOfInterestOrFail(
                                         "EVP_MD_CTX", IRDB, ForwardRenaming,
                                         "error: could not retrieve desugared "
                                         "typenameOfInterest for EVP_MD_CTX");
  }
}

template <TypeStateAnalysisKind TSAKind>
auto getTSADesc(llvm::StringRef TypeNameOfInterest,
                const stringstringmap_t &ForwardRenaming) {
  if constexpr (TSAKind == TypeStateAnalysisKind::Cipher) {
    return OpenSSLEVPCIPHERCTXDescription(
        ForwardRenaming.empty() ? nullptr : &ForwardRenaming,
        TypeNameOfInterest);
  } else {
    return OpenSSLEVPMDCTXDescription(
        ForwardRenaming.empty() ? nullptr : &ForwardRenaming,
        TypeNameOfInterest);
  }
}

template <TypeStateAnalysisKind TSAKind>
void doAnalysis(const LLVMProjectIRDB &IRDB, LLVMAliasInfoRef PT,
                const LLVMBasedICFG &ICF, llvm::StringRef CtorName,
                const stringstringmap_t &ForwardRenaming,
                const stringstringmap_t &BackwardRenaming,
                llvm::ArrayRef<std::string> AnalysisEntryPoints) {
  auto TypeNameOfInterest =
      getTypeNameOfInterest<TSAKind>(IRDB, ForwardRenaming);

  auto TSADesc = getTSADesc<TSAKind>(TypeNameOfInterest, ForwardRenaming);

  IDETypeStateAnalysis Problem(&IRDB, PT, &TSADesc, AnalysisEntryPoints);
  IDEVarTabulationProblem VarProblem(Problem, ICF, &BackwardRenaming);
  auto Results = solveIDEProblem(VarProblem, ICF);
  if (EmitRawResults) {
    Results.dumpResults(ICF);
  } else {
    emitTextReport(IRDB, Results, TSADesc);
  }
}

} // namespace

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

  std::string Main;
  if (auto It = ForwardRenaming.find("main"); It != ForwardRenaming.end()) {
    Main = It->second.str();
  } else {
    Main = "__ALL__";
  }
  auto AnalysisEntryPoints = std::vector<std::string>{{std::move(Main)}};

  // Note: Cannot use IncludeGlobals, because the generated entrypoints-selector
  // function can currently only call parameterless functions, except main.
  // Since main is renamed to __main_123 or similar, it refuses to create a call
  // to main.
  LLVMBasedICFG ICF(&IR, CallGraphAnalysisType::OTF, AnalysisEntryPoints, &TH,
                    &PT, Soundness::Soundy, /*IncludeGlobals=*/false);

  if (AnalysisKind == TypeStateAnalysisKind::Cipher) {
    doAnalysis<TypeStateAnalysisKind::Cipher>(
        IR, &PT, ICF, "EVP_CIPHER_CTX_new", ForwardRenaming, BackwardRenaming,
        AnalysisEntryPoints);
  } else if (AnalysisKind == TypeStateAnalysisKind::MessageDigest ||
             AnalysisKind == TypeStateAnalysisKind::MAC) {
    doAnalysis<TypeStateAnalysisKind::MessageDigest>(
        IR, &PT, ICF, "EVP_MD_CTX_new", ForwardRenaming, BackwardRenaming,
        AnalysisEntryPoints);
  }
  return 0;
}
