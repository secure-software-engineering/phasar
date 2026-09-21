#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/HelperAnalysisConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Pointer/AliasAnalysisType.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdio>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace cl = llvm::cl;

static cl::OptionCategory Cat("CallGraphCSV");

static cl::opt<std::string> IRFile(cl::Positional, cl::Required,
                                   cl::desc("The LLVM IR file to analyze"),
                                   cl::cat(Cat));

static cl::opt<psr::CallGraphAnalysisType>
    CGTy("call-graph-analysis", cl::init(psr::CallGraphAnalysisType::VTA),
         cl::cat(Cat),
         cl::ValuesClass{
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  clEnumValN(psr::CallGraphAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/ControlFlow/CallGraphAnalysisType.def"
         });

static cl::opt<psr::AliasAnalysisType> AATy(
    "alias-analysis", cl::init(psr::AliasAnalysisType::AndersenOTF),
    cl::cat(Cat),
    cl::desc("The alias analysis to be used by VTA or OTF call-graph analysis. "
             "Note that CFLAnders/CFLSteens should only be used with "
             "call-graph-analysis=otf"),
    cl::ValuesClass{
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                               \
  clEnumValN(psr::AliasAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/Pointer/AliasAnalysisType.def"
    });

static cl::list<std::string> EntryPointsOpt(
    "entry-points", cl::OneOrMore, cl::cat(Cat),
    cl::desc("The functions from which the analysis should start. "
             "For executables, usually 'main'; use '__ALL__' for all "
             "externally visible functions"));

static cl::opt<std::string>
    OutFile("o", cl::init("-"), cl::cat(Cat),
            cl::desc("The CSV output file path. Stdout by default"));

struct ResolvedLoc {
  uint32_t Line{};
  uint32_t Column{};
  llvm::StringRef FileName{};
  bool Approximate{}; // true = fell back to enclosing function's declaration
                      // site, not the real call site

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const ResolvedLoc &Loc) {
    OS.write_escaped(Loc.FileName) << ',';
    return OS << Loc.Line << ',' << Loc.Column;
  }
};

static constexpr llvm::StringLiteral NoDebugInfo = "<no-debug-info>";

static ResolvedLoc resolveLocation(const llvm::Instruction *I) {
  if (auto Loc = psr::getDebugLocation(I)) {
    return {
        .Line = Loc->Line,
        .Column = Loc->Column,
        .FileName = Loc->File->getFilename(),
        .Approximate = false,
    };
  }

  // No direct location on this instruction -- fall back to the
  // enclosing function's declared file
  if (const auto *SP = I->getFunction()->getSubprogram()) {
    return {
        .Line = SP->getLine(),
        .Column = 0,
        .FileName = SP->getFile()->getFilename(),
        .Approximate = true,
    };
  }
  return {.FileName = NoDebugInfo, .Approximate = true};
}

static ResolvedLoc resolveLocation(const llvm::Function *F) {
  if (const auto *SP = F->getSubprogram()) {
    return {
        .Line = SP->getLine(),
        .Column = 0,
        .FileName = SP->getFile()->getFilename(),
        .Approximate = false,
    };
  }
  return {.FileName = NoDebugInfo, .Approximate = true};
}

int main(int Argc, char **Argv) {
  cl::HideUnrelatedOptions(Cat);
  cl::ParseCommandLineOptions(
      Argc, Argv,
      "Simple CLI tool to build a PhASAR-based call-graph and print it as CSV. "
      "Uses on-the-fly printing, so you get results even when aborting the "
      "process.");

  std::error_code EC;
  llvm::raw_fd_ostream OS(OutFile, EC);
  if (EC) {
    llvm::WithColor::error()
        << "While opening output file '" << OutFile << "':\n";
    llvm::WithColor::error() << EC.message() << '\n';
    return 1;
  }

  // CSV header:
  OS << "caller_function,caller_file,caller_line,caller_column,"
        "callee_function,callee_file,callee_line,callee_column,"
        "caller_loc_approximate\n";
  OS.flush();

  psr::HelperAnalyses HA{
      std::make_unique<psr::LLVMProjectIRDB>(
          psr::LLVMProjectIRDB::loadOrExit(IRFile)),
      EntryPointsOpt,
      psr::HelperAnalysisConfig{.PTATy = AATy, .CGTy = CGTy},
  };

  auto &ICF = HA.getICFG();

  size_t EdgeCount = 0;
  size_t FnCount = 0;

  // Walk every function PhASAR's call-graph knows about, every call
  // instruction inside it, and every resolved callee at that specific call
  // site, writing (and forgetting) each result immediately instead of
  // accumulating all of them.
  for (const llvm::Function *Fun : ICF.getAllVertexFunctions()) {
    auto FunName = Fun->getName();
    ++FnCount;

    ResolvedLoc CalleeLoc = resolveLocation(Fun);
    for (const auto *CS : ICF.getCallersOf(Fun)) {
      ResolvedLoc CallerLoc = resolveLocation(CS);

      OS.write_escaped(CS->getFunction()->getName()) << ',' << CallerLoc << ',';
      OS.write_escaped(FunName) << ',' << CalleeLoc << ',';
      OS << (CallerLoc.Approximate ? "true" : "false") << '\n';

      ++EdgeCount;
      if (EdgeCount % 50000 == 0) {
        OS.flush(); // periodic flush -- makes progress visible on
                    // disk instead of buffered invisibly
        llvm::WithColor::note()
            << "[export_callsite_cg_streaming] " << EdgeCount
            << " edges written, " << FnCount << '/'
            << ICF.getNumVertexFunctions() << " functions processed so far\n";
      }
    }
  }

  OS.close();

  llvm::WithColor::note() << "[export_callsite_cg_streaming] DONE: "
                          << EdgeCount << " total edges across " << FnCount
                          << " functions -> "
                          << (OutFile == "-" ? llvm::StringRef("stdout")
                                             : OutFile)
                          << "\n";
  return 0;
}
