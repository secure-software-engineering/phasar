#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/SrcCodeLocationEntry.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include "SourceMapping.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <cstdint>
#include <memory>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSTaintAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("taint_analysis/");
  static inline const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;

  std::optional<IFDSTaintAnalysis> TaintProblem;
  std::optional<LLVMTaintConfig> TSF;

  IFDSTaintAnalysisTest() = default;
  ~IFDSTaintAnalysisTest() override = default;

  static LLVMTaintConfig getDefaultConfig() {
    auto SourceCB = [](const llvm::Instruction *Inst) {
      std::set<const llvm::Value *> Ret;
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
          Call && Call->getCalledFunction() &&
          Call->getCalledFunction()->getName() == "_Z6sourcev") {
        Ret.insert(Call);
      }
      return Ret;
    };
    auto SinkCB = [](const llvm::Instruction *Inst) {
      std::set<const llvm::Value *> Ret;
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
          Call && Call->getCalledFunction() &&
          Call->getCalledFunction()->getName() == "_Z4sinki") {
        assert(Call->arg_size() > 0);
        Ret.insert(Call->getArgOperand(0));
      }
      return Ret;
    };
    return LLVMTaintConfig(std::move(SourceCB), std::move(SinkCB));
  }

  static LLVMTaintConfig getDoubleFreeConfig() {
    auto SourceCB = [](const llvm::Instruction *Inst) {
      std::set<const llvm::Value *> Ret;
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
          Call && Call->getCalledFunction() &&
          Call->getCalledFunction()->getName() == "free") {
        Ret.insert(Call->getArgOperand(0));
      }
      return Ret;
    };

    return LLVMTaintConfig(SourceCB, SourceCB);
  }

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(IRFile, EntryPoints);

    if (!TSF) {
      TSF = getDefaultConfig();
    }
    TaintProblem =
        createAnalysisProblem<IFDSTaintAnalysis>(*HA, &*TSF, EntryPoints);
  }
  static void doAnalysis(const llvm::Twine &IRFile,
                         const LLVMTaintConfig &Config,
                         const std::set<SrcCodeLocationEntry> &GroundTruth) {
    HelperAnalyses HA(PathToLlFiles + IRFile, EntryPoints);

    auto TaintProblem =
        createAnalysisProblem<IFDSTaintAnalysis>(HA, &Config, EntryPoints);

    IFDSSolver TaintSolver(TaintProblem, &HA.getICFG());
    TaintSolver.solve();

    TaintSolver.dumpResults();

    compare(TaintProblem.Leaks, GroundTruth);
  }

  static void doAnalysis(const llvm::Twine &IRFile,
                         const std::set<SrcCodeLocationEntry> &GroundTruth) {
    doAnalysis(IRFile, getDefaultConfig(), GroundTruth);
  }

  template <typename LeaksTy>
  static void compare(const LeaksTy &Leaks,
                      const std::set<SrcCodeLocationEntry> &GroundTruth) {
    // TODO: zu std::set ändern
    std::set<SrcCodeLocationEntry> FoundLeaks;
    for (const auto &Leak : Leaks) {
      uint32_t SinkId;
      if (Leak.first) {
        if (const auto *SinkInst =
                llvm::dyn_cast_or_null<llvm::Instruction>(Leak.first)) {
          SinkId = SinkInst->getDebugLoc()->getLine();
          llvm::outs() << "SinkId: " << SinkId << "\n";
        }
      }
      std::set<uint32_t> LeakedValueIds;
      for (const auto *LV : Leak.second) {
        llvm::outs() << "*LV: " << *LV << "\n";
        if (LV) {
          if (const auto *LVInst =
                  llvm::dyn_cast_or_null<llvm::Instruction>(LV)) {
            LeakedValueIds.insert(LVInst->getDebugLoc()->getColumn());
            llvm::outs() << "getCol: " << LVInst->getDebugLoc()->getColumn()
                         << "\n";
          }
        }
        // LeakedValueIds.insert(getMetaDataID(LV));
      }
      FoundLeaks.insert({SinkId, LeakedValueIds});
    }
    EXPECT_EQ(FoundLeaks, GroundTruth);
  }

  void
  compareResults(const std::set<SrcCodeLocationEntry> &GroundTruth) noexcept {
    compare(TaintProblem->Leaks, GroundTruth);
  }
}; // Test Fixture

TEST_F(IFDSTaintAnalysisTest, TaintTest_01) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_01_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  SrcCodeLocationEntry Entry(6, {8});
  std::set<SrcCodeLocationEntry> GroundTruth = {Entry};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_01_m2r) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_01_cpp_m2r_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  SrcCodeLocationEntry Entry(6, {11});
  std::set<SrcCodeLocationEntry> GroundTruth = {Entry};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_02) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_02_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  SrcCodeLocationEntry Entry(5, {8});
  std::set<SrcCodeLocationEntry> GroundTruth = {Entry};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_03) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_03_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  SrcCodeLocationEntry Entry(6, {8});
  std::set<SrcCodeLocationEntry> GroundTruth = {Entry};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_04) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_04_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  SrcCodeLocationEntry Entry(6, {8});
  SrcCodeLocationEntry EntryTwo(8, {8});
  std::set<SrcCodeLocationEntry> GroundTruth = {Entry, EntryTwo};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_05) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_05_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  SrcCodeLocationEntry Entry(6, {8});
  std::set<SrcCodeLocationEntry> GroundTruth = {Entry};
  compareResults(GroundTruth);
}

// TODO: Fabian fragen, wie man mit Ground Truth von "main.0" hier umgehen soll.
#if false

TEST_F(IFDSTaintAnalysisTest, TaintTest_06) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_06_cpp_m2r_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[5] = set<string>{"main.0"};
  compareResults(GroundTruth);
}

#endif

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_01_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{12, {8}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01_m2r) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_01_cpp_m2r_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{12, {14}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_02) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_02_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{11, {8}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_03) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_03_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  // This test weirdly fails, when I just insert {{}, {}} into compareResults.
  std::set<SrcCodeLocationEntry> GroundTruth;
  GroundTruth.insert(SrcCodeLocationEntry{11, {8}});
  GroundTruth.insert(SrcCodeLocationEntry{14, {8}});
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_04) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_04_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{16, {8}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_05) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_05_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{16, {8}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_06) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_06_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{13, {10}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_07) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_07_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{14, {10}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_08) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_08_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{19, {8}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_09) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_09_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{20, {8}}});
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_10) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_10_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  compareResults({{19, {10}}});
}

// TODO: fix seg fault
#if false

TEST_F(IFDSTaintAnalysisTest, TaintTest_DoubleFree_01) {
  doAnalysis("double_free_01_c.ll", getDoubleFreeConfig(),
             {
                 {6, {5}},
             });
}


TEST_F(IFDSTaintAnalysisTest, TaintTest_DoubleFree_02) {
  doAnalysis("double_free_02_c.ll", getDoubleFreeConfig(),
             {
                 {11, {"10"}},
             });
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_LibSummary_01) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_lib_sum_01_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[20] = {"19"};
  compareResults(GroundTruth);
}

#endif

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
