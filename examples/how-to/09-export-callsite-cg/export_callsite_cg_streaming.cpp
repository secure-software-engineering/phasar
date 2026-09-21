// export_callsite_cg_streaming.cpp
//
// export_callsite_cg.cpp (the first version) calls PhASAR's
// exportICFGAsJson(), which builds ONE nlohmann::json object holding
// every edge in memory, and only writes to disk at the very end. On
// FFmpeg's real edge count (~1.2M+, per the CHA/RTA baseline), this grew
// unbounded -- confirmed against a real container run: 11GB resident,
// then 2.8GB of swap and climbing, zero bytes written the entire time,
// no realistic path to finishing.
//
// This version uses the same underlying PhASAR primitives
// (getCallsFromWithin, getCalleesOfCallAt, getSrcCodeInfoFromIR) that
// exportICFGAsJson itself is built on, but writes each edge to disk as
// CSV the moment it's produced, then discards it. Memory stays roughly
// constant regardless of total edge count -- the file also grows
// visibly during the run, so progress is actually observable instead of
// a black box until either completion or an OOM kill.
//
// Usage:
//   export_callsite_cg_streaming <bitcode.bc> <entry-point> [cha|rta|vta|otf] <out.csv>

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/HelperAnalysisConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace psr;

// Deliberately bypasses PhASAR's getSrcCodeInfoFromIR: it composes File
// from getFilePathFromIR/getDIFileFromIR and Line/Column from
// getLineFromIR/getDILocation -- two functions with DIFFERENT fallback
// branches for instructions lacking direct !dbg metadata (inlined or
// macro-expanded calls). This can silently pair a File from one
// resolution path with a Line from another, unrelated one -- confirmed
// against real FFmpeg data: a "task_wrapper" callsite reported line 2776
// in a 63-line header.
//
// This resolves ONE DILocation per instruction, reads File/Line/Column
// off that SAME object, and explicitly marks (rather than silently
// guesses at) cases with no direct location at all.
struct ResolvedLoc {
  std::string File;
  unsigned Line = 0;
  unsigned Column = 0;
  bool Approximate = false; // true = fell back to enclosing function's
                            // declaration site, not the real call site
};

static ResolvedLoc resolveLocation(const llvm::Instruction *I) {
  if (const llvm::DebugLoc &DL = I->getDebugLoc()) {
    const llvm::DILocation *Loc = DL.get();
    return {Loc->getFilename().str(), Loc->getLine(), Loc->getColumn(),
            false};
  }
  // No direct location on this instruction -- fall back to the
  // enclosing function's declared file, but SAY SO explicitly rather
  // than silently blending it with an unrelated line number the way
  // PhASAR's composed helper does.
  if (const llvm::DISubprogram *SP = I->getFunction()->getSubprogram()) {
    return {SP->getFilename().str(), SP->getLine(), 0, true};
  }
  return {"<no-debug-info>", 0, 0, true};
}

static ResolvedLoc resolveLocation(const llvm::Function *F) {
  if (const llvm::DISubprogram *SP = F->getSubprogram()) {
    return {SP->getFilename().str(), SP->getLine(), 0, false};
  }
  return {"<no-debug-info>", 0, 0, true};
}

static CallGraphAnalysisType parseCGType(const std::string &S) {
  if (S == "cha") return CallGraphAnalysisType::CHA;
  if (S == "rta") return CallGraphAnalysisType::RTA;
  if (S == "vta") return CallGraphAnalysisType::VTA;
  if (S == "otf") return CallGraphAnalysisType::OTF;
  llvm::errs() << "Unknown call-graph analysis type '" << S
               << "', defaulting to OTF\n";
  return CallGraphAnalysisType::OTF;
}

// Minimal CSV field escaping -- source lines can contain commas/quotes.
static std::string csvField(const std::string &S) {
  bool needsQuote = S.find(',') != std::string::npos ||
                    S.find('"') != std::string::npos ||
                    S.find('\n') != std::string::npos;
  if (!needsQuote)
    return S;
  std::string Out = "\"";
  for (char C : S) {
    if (C == '"')
      Out += "\"\"";
    else
      Out += C;
  }
  Out += "\"";
  return Out;
}

int main(int argc, char **argv) {
  if (argc < 5) {
    llvm::errs() << "usage: " << argv[0]
                 << " <bitcode.bc> <entry-point> [cha|rta|vta|otf] <out.csv>\n";
    return 1;
  }

  std::string BitcodeFile = argv[1];
  std::vector<std::string> EntryPoints = {argv[2]};
  CallGraphAnalysisType CGTy = parseCGType(argv[3]);
  std::string OutPath = argv[4];

  FILE *Out = std::fopen(OutPath.c_str(), "w");
  if (!Out) {
    llvm::errs() << "could not open " << OutPath << " for writing\n";
    return 1;
  }
  std::fprintf(Out, "caller_function,caller_file,caller_line,caller_column,"
                     "callee_function,callee_file,callee_line,callee_column,"
                     "caller_loc_approximate\n");
  std::fflush(Out);

  HelperAnalyses HA(BitcodeFile, EntryPoints,
                    HelperAnalysisConfig{}.withCGType(CGTy));
  LLVMBasedICFG &ICF = HA.getICFG();

  uint64_t EdgeCount = 0;
  uint64_t FnCount = 0;

  // Walk every function PhASAR knows about, every call instruction
  // inside it, and every resolved callee at that specific call site --
  // exactly the same traversal exportICFGAsJson does internally, just
  // writing (and forgetting) each result immediately instead of
  // accumulating all of them.
  for (const llvm::Function *Fun : ICF.getAllFunctions()) {
    if (Fun->isDeclaration())
      continue;
    ++FnCount;

    for (const llvm::Instruction *CS : ICF.getCallsFromWithin(Fun)) {
      ResolvedLoc CallerLoc = resolveLocation(CS);

      for (const llvm::Function *Callee : ICF.getCalleesOfCallAt(CS)) {
        if (!Callee)
          continue;
        ResolvedLoc CalleeLoc = resolveLocation(Callee);

        std::fprintf(
            Out, "%s,%s,%u,%u,%s,%s,%u,%u,%s\n",
            csvField(Fun->getName().str()).c_str(),
            csvField(CallerLoc.File).c_str(),
            CallerLoc.Line, CallerLoc.Column,
            csvField(Callee->getName().str()).c_str(),
            csvField(CalleeLoc.File).c_str(),
            CalleeLoc.Line, CalleeLoc.Column,
            CallerLoc.Approximate ? "true" : "false");

        ++EdgeCount;
        if (EdgeCount % 50000 == 0) {
          std::fflush(Out); // periodic flush -- makes progress visible on
                             // disk instead of buffered invisibly
          llvm::errs() << "[export_callsite_cg_streaming] " << EdgeCount
                       << " edges written, " << FnCount
                       << " functions processed so far\n";
        }
      }
    }
  }

  std::fflush(Out);
  std::fclose(Out);

  llvm::errs() << "[export_callsite_cg_streaming] DONE: " << EdgeCount
               << " total edges across " << FnCount << " functions -> "
               << OutPath << "\n";
  return 0;
}
