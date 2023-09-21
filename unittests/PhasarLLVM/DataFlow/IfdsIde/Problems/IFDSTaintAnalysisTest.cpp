#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

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

#include "TestConfig.h"
#include "gtest/gtest.h"

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
                         const map<int, set<string>> &GroundTruth) {
    HelperAnalyses HA(PathToLlFiles + IRFile, EntryPoints);

    auto TaintProblem =
        createAnalysisProblem<IFDSTaintAnalysis>(HA, &Config, EntryPoints);

    IFDSSolver TaintSolver(TaintProblem, &HA.getICFG());
    TaintSolver.solve();

    TaintSolver.dumpResults();

    compare(TaintProblem.Leaks, GroundTruth);
  }

  static void doAnalysis(const llvm::Twine &IRFile,
                         const map<int, set<string>> &GroundTruth) {
    doAnalysis(IRFile, getDefaultConfig(), GroundTruth);
  }

  template <typename LeaksTy>
  static void compare(const LeaksTy &Leaks,
                      const map<int, set<string>> &GroundTruth) {
    map<int, set<string>> FoundLeaks;
    for (const auto &Leak : Leaks) {
      int SinkId = stoi(getMetaDataID(Leak.first));
      set<string> LeakedValueIds;
      for (const auto *LV : Leak.second) {
        LeakedValueIds.insert(getMetaDataID(LV));
      }
      FoundLeaks.insert(make_pair(SinkId, LeakedValueIds));
    }
    EXPECT_EQ(FoundLeaks, GroundTruth);
  }

  void compareResults(const map<int, set<string>> &GroundTruth) noexcept {
    compare(TaintProblem->Leaks, GroundTruth);
  }
}; // Test Fixture

TEST_F(IFDSTaintAnalysisTest, TaintTest_01) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_01_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[13] = set<string>{"12"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_01_m2r) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_01_cpp_m2r_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[4] = set<string>{"2"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_02) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_02_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[9] = set<string>{"8"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_03) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_03_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[18] = set<string>{"17"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_04) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_04_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[19] = set<string>{"18"};
  GroundTruth[24] = set<string>{"23"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_05) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_05_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[22] = set<string>{"21"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_06) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_06_cpp_m2r_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[5] = set<string>{"main.0"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_01_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[15] = set<string>{"14"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01_m2r) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_01_cpp_m2r_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[6] = set<string>{"0"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_02) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_02_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[17] = set<string>{"16"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_03) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_03_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[11] = set<string>{"10"};
  GroundTruth[21] = set<string>{"20"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_04) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_04_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_05) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_05_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();

  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_06) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_06_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[15] = set<string>{"14"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_07) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_07_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[31] = set<string>{"30"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_08) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_08_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_09) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_09_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[64] = set<string>{"63"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_10) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_10_cpp_dbg.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[62] = set<string>{"61"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_DoubleFree_01) {
  doAnalysis("double_free_01_c.ll", getDoubleFreeConfig(),
             {
                 {6, {"5"}},
             });
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_DoubleFree_02) {
  doAnalysis("double_free_02_c.ll", getDoubleFreeConfig(),
             {
                 {11, {"10"}},
             });
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
