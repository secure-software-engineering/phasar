/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/VTAResolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Utils/AlignNum.h"
#include "phasar/Utils/Timer.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <system_error>

namespace cl = llvm::cl;

static cl::OptionCategory CGCat("PhASAR CallGraph");

static cl::opt<bool> EmitCGAsDot(
    "emit-cg-as-dot",
    cl::desc("Output the computed call-graph as DOT graph that can be "
             "displayed with any graphviz viewer (default: true)"),
    cl::init(true), cl::cat(CGCat));

static cl::opt<bool> EmitCGAsJson(
    "emit-cg-as-json",
    cl::desc("Output the computed call-graph as JSON (default: false)"),
    cl::cat(CGCat));

static cl::opt<std::string>
    OutputFile("o",
               cl::desc("The file-path, where the output should be written to "
                        "(default: stdout)"),
               cl::init("-"), cl::cat(CGCat));

static cl::opt<psr::CallGraphAnalysisType>
    CGType("cg-type", cl::desc("The call-graph analysis type to use"),
           cl::ValuesClass{
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  clEnumValN(psr::CallGraphAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/ControlFlow/CallGraphAnalysisType.def"
           },
           cl::init(psr::CallGraphAnalysisType::OTF), cl::cat(CGCat));

static cl::opt<bool> BuildBaseCG(
    "build-base-cg",
    cl::desc("Whether to build-up an explicit base-call-graph to "
             "initialize the VTA algorithm. May take more time, but may reduce "
             "the size of the type-assignment graph"));

static cl::opt<psr::AliasAnalysisType>
    AAType("aa-type",
           cl::desc("The alias-analysis type for those call-graph "
                    "algorithms that require alias information"),
           cl::ValuesClass{
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                               \
  clEnumValN(psr::AliasAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/Pointer/AliasAnalysisType.def"
           },
           cl::init(psr::AliasAnalysisType::CFLAnders), cl::cat(CGCat));

static cl::opt<bool>
    EmitStats("S", cl::desc("Compute statistics on the computed call-graph"),
              cl::cat(CGCat));

static cl::opt<std::string> IRFile(cl::Positional, cl::Required,
                                   cl::desc("<The LLVM IR file to analyze>"),
                                   cl::cat(CGCat));

struct DiagTimer : psr::SimpleTimer { // NOLINT
  DiagTimer(llvm::StringRef Msg) noexcept : Message(Msg) {}
  ~DiagTimer() { llvm::errs() << Message << " (" << elapsed() << ")\n"; }

  llvm::StringRef Message;
};
static void computeCGStats(const psr::LLVMBasedCallGraph &CG,
                           llvm::raw_ostream &OS);

int main(int Argc, char *Argv[]) {
  cl::HideUnrelatedOptions(CGCat);
  cl::ParseCommandLineOptions(Argc, Argv);

  psr::SimpleTimer LoadingTm;
  auto IRDB = psr::LLVMProjectIRDB::loadOrExit(IRFile);
  auto VTP = psr::LLVMVFTableProvider(IRDB);
  auto TH = psr::DIBasedTypeHierarchy(IRDB);
  auto EntryPoints = psr::getDefaultEntryPoints(IRDB);
  llvm::errs() << "Loaded IR and computed helpers (" << LoadingTm.elapsed()
               << ")\n";

  if (BuildBaseCG && CGType != psr::CallGraphAnalysisType::VTA) {
    llvm::WithColor::warning() << "The option --build-base-cg only works for "
                                  "the cg-type 'vta'. It will be ignored for '"
                               << CGType << "'\n";
  }

  auto CG = [&] {
    DiagTimer Tm{"Created resolver"};

    switch (CGType) {
    case psr::CallGraphAnalysisType::NORESOLVE:
    case psr::CallGraphAnalysisType::CHA:
    case psr::CallGraphAnalysisType::RTA: {
      auto Res = psr::Resolver::create(CGType, &IRDB, &VTP, &TH);
      return psr::buildLLVMBasedCallGraph(IRDB, *Res, EntryPoints);
    }
    case psr::CallGraphAnalysisType::VTA: {
      auto BaseRes = psr::RTAResolver(&IRDB, &VTP, &TH);
      auto AA = psr::LLVMAliasSet(&IRDB, true, AAType);
      auto Res = [&] {
        if (BuildBaseCG) {
          auto BaseCG = std::make_unique<psr::LLVMBasedCallGraph>(
              psr::buildLLVMBasedCallGraph(IRDB, BaseRes, EntryPoints));
          return psr::VTAResolver(&IRDB, &VTP, &AA, std::move(BaseCG));
        }
        return psr::VTAResolver(&IRDB, &VTP, &AA, &BaseRes);
      }();
      return psr::buildLLVMBasedCallGraph(IRDB, Res, EntryPoints);
    }
    case psr::CallGraphAnalysisType::OTF: {
      auto AA = psr::LLVMAliasSet(&IRDB, true, AAType);
      auto Res = psr::OTFResolver(&IRDB, &VTP, &AA);
      return psr::buildLLVMBasedCallGraph(IRDB, Res, EntryPoints);
    }
    case psr::CallGraphAnalysisType::Invalid:
      llvm::report_fatal_error("Invalid call-graph analysis type");
    }
  }();

  std::optional<llvm::raw_fd_ostream> OS;
  const auto GetOS = [&OS]() -> llvm::raw_ostream & {
    if (!OS) {
      std::error_code EC;
      OS.emplace(OutputFile, EC);
      if (EC) {
        llvm::WithColor::error()
            << "Could not open output-file: " << EC.message() << '\n';
        std::exit(1);
      }
    }
    return *OS;
  };

  auto ICF = [&] {
    DiagTimer Tm{"Built call-graph"};
    return psr::LLVMBasedICFG(std::move(CG), &IRDB);
  }();

  if (EmitCGAsDot) {
    ICF.print(GetOS());
  }
  if (EmitCGAsJson) {
    ICF.printAsJson(GetOS());
  }
  if (EmitStats) {
    computeCGStats(ICF.getCallGraph(), GetOS());
  }
}

static constexpr unsigned Indent = 48;

template <typename T> struct Align : psr::AlignNum<T, Indent> {
  using psr::AlignNum<T, Indent>::AlignNum;
};
template <typename T> Align(llvm::StringRef, T) -> Align<T>;
Align(llvm::StringRef, size_t, size_t) -> Align<double>;

using AlignS = psr::AlignStr<Indent>;

static void computeCGStats(const psr::LLVMBasedCallGraph &CG,
                           llvm::raw_ostream &OS) {
  size_t NumVtxFuns = CG.getNumVertexFunctions();
  size_t NumVtxCS = CG.getNumVertexCallSites();

  size_t NumIndCalls = 0;
  size_t NumCallEdges = 0;
  size_t NumIndCallEdges = 0;

  size_t NumIndCSWith0Callees = 0;
  size_t NumIndCSWith1Callees = 0;
  size_t NumIndCSWith2Callees = 0;
  size_t NumIndCSWithGreater2Callees = 0;
  size_t NumIndCSWithGreater5Callees = 0;
  size_t NumIndCSWithGreater10Callees = 0;
  size_t NumIndCSWithGreater20Callees = 0;
  size_t NumIndCSWithGreater50Callees = 0;
  size_t NumIndCSWithGreater100Callees = 0;
  size_t LargestFanOut = 0;

  std::vector<uint32_t> NumCallEdgesPerCS;
  std::vector<uint32_t> NumCallEdgesPerIndCS;
  NumCallEdgesPerCS.reserve(NumVtxCS);
  NumCallEdgesPerIndCS.reserve(NumVtxCS);

  for (const auto *CS : CG.getAllVertexCallSites()) {
    bool IsIndCall =
        !llvm::isa<llvm::Function>(llvm::cast<llvm::CallBase>(CS)
                                       ->getCalledOperand()
                                       ->stripPointerCastsAndAliases());

    auto Callees = CG.getCalleesOfCallAt(CS);
    NumIndCalls += IsIndCall;
    NumCallEdges += Callees.size();
    NumIndCallEdges += Callees.size() * IsIndCall;
    NumCallEdgesPerCS.push_back(Callees.size());
    if (IsIndCall) {
      NumCallEdgesPerIndCS.push_back(Callees.size());
    }
    if (Callees.size() > LargestFanOut) {
      LargestFanOut = Callees.size();
    }

    NumIndCSWith0Callees += Callees.empty();
    NumIndCSWith1Callees += Callees.size() == 1 && IsIndCall;
    NumIndCSWith2Callees += Callees.size() == 2;
    NumIndCSWithGreater2Callees += Callees.size() > 2;
    NumIndCSWithGreater5Callees += Callees.size() > 5;
    NumIndCSWithGreater10Callees += Callees.size() > 10;
    NumIndCSWithGreater20Callees += Callees.size() > 20;
    NumIndCSWithGreater50Callees += Callees.size() > 50;
    NumIndCSWithGreater100Callees += Callees.size() > 100;
  }

  llvm::sort(NumCallEdgesPerCS);
  llvm::sort(NumCallEdgesPerIndCS);

  OS << "================== CallGraph Statistics ==================\n";

  OS << Align("Num vertex functions", NumVtxFuns);
  OS << Align("Num call-sites", NumVtxCS);
  OS << Align("Num call-edges", NumCallEdges);
  if (NumCallEdgesPerCS.empty()) {
    OS << AlignS("Avg num call-edges per call-site", "<none>");
    OS << AlignS("Med num call-edges per call-site", "<none>");
    OS << AlignS("90% num call-edges per call-site", "<none>");
  } else {
    OS << Align("Avg num call-edges per call-site",
                double(NumCallEdges) / double(NumVtxCS));
    OS << Align("Med num call-edges per call-site",
                NumCallEdgesPerCS[NumCallEdgesPerCS.size() / 2]);
    OS << Align(
        "90% num call-edges per call-site",
        NumCallEdgesPerCS[size_t(double(NumCallEdgesPerCS.size()) * 0.9)]);
  }
  OS << '\n';
  OS << Align("Num indirect call-sites", NumIndCalls);
  OS << Align("Num indirect call-edges", NumIndCallEdges);

  if (NumCallEdgesPerIndCS.empty()) {
    OS << AlignS("Avg num call-edges per indirect call-site", "<none>");
    OS << AlignS("Med num call-edges per indirect call-site", "<none>");
    OS << AlignS("90% num call-edges per indirect call-site", "<none>");
  } else {
    OS << Align("Avg num call-edges per indirect call-site",
                double(NumIndCallEdges) / double(NumIndCalls));
    OS << Align("Med num call-edges per indirect call-site",
                NumCallEdgesPerIndCS[NumCallEdgesPerIndCS.size() / 2]);
    OS << Align("90% num call-edges per indirect call-site",
                NumCallEdgesPerIndCS[size_t(
                    double(NumCallEdgesPerIndCS.size()) * 0.9)]);
  }
  OS << Align("Largest fanout (max num callees per call-site)", LargestFanOut);

  OS << '\n';
  OS << Align("Num indirect calls with 0 resolved callees",
              NumIndCSWith0Callees);
  OS << Align("Num indirect calls with 1 resolved callee",
              NumIndCSWith1Callees);
  OS << Align("Num indirect calls with 2 resolved callees",
              NumIndCSWith2Callees);
  OS << Align("Num indirect calls with >  2 resolved callees",
              NumIndCSWithGreater2Callees);
  OS << Align("Num indirect calls with >  5 resolved callees",
              NumIndCSWithGreater5Callees);
  OS << Align("Num indirect calls with > 10 resolved callees",
              NumIndCSWithGreater10Callees);
  OS << Align("Num indirect calls with > 20 resolved callees",
              NumIndCSWithGreater20Callees);
  OS << Align("Num indirect calls with > 50 resolved callees",
              NumIndCSWithGreater50Callees);
  OS << Align("Num indirect calls with >100 resolved callees",
              NumIndCSWithGreater100Callees);
}
