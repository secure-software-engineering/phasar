#include "phasar/PhasarLLVM/DataFlow/MonoIfds/Problems/MonoIFDSTaintAnalysis.h"

#include "phasar/ControlFlow/CGSCCs.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/DataFlow/MonoIfds/MonoIFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/FunctionCompressor.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/CachedLLVMAliasIterator.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasIterator.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/UsedGlobals.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/Printer.h"

#include "llvm/ADT/Twine.h"

#include "SrcCodeLocationEntry.h"
#include "TaintTest.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <optional>
#include <source_location>
#include <string>

namespace {
using namespace psr;
using namespace psr::unittest;

class GroundTruthCollector
    : public AnalysisPrinterBase<
          monoifds::TaintAnalysis::ProblemAnalysisDomain> {
public:
  using base_t =
      AnalysisPrinterBase<monoifds::TaintAnalysis::ProblemAnalysisDomain>;

  using typename base_t::d_t;
  using typename base_t::n_t;

  GroundTruthCollector(std::map<n_t, std::set<d_t>> GroundTruth,
                       std::source_location Loc)
      : GroundTruth(std::move(GroundTruth)), Loc(Loc) {}

  void doOnResult(n_t Inst, d_t Fact, l_t /*Value*/,
                  DataFlowAnalysisType /*TAType*/) override {
    auto *Inner = getOrNull(GroundTruth, Inst);
    ASSERT_TRUE(Inner && Inner->erase(Fact))
        << "Unexpected fact " << DToString(Fact) << " found at "
        << NToString(Inst) << ";\nCalled from " << loc();
    if (Inner && Inner->empty()) {
      GroundTruth.erase(Inst);
    }
  }

  void doOnFinalize() override {
    ASSERT_TRUE(GroundTruth.empty())
        << "Expected facts not found: " << PrettyPrinter{GroundTruth}
        << ";\nCalled from " << loc();
  }

  [[nodiscard]] std::string loc() const {
    return std::string(Loc.file_name()) + ":" + std::to_string(Loc.line()) +
           ":" + std::to_string(Loc.column());
  }

  std::map<n_t, std::set<d_t>> GroundTruth;
  std::source_location Loc;
};

class MonoIFDSTaintAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("taint_analysis/");
  static inline const std::vector<std::string> EntryPoints = {"main"};

  using GroundTruthTy =
      std::map<TestingSrcLocation, std::set<TestingSrcLocation>>;

  // void SetUp() override {
  //   psr::Logger::initializeStderrLogger(
  //       SeverityLevel::DEBUG,
  //       monoifds::MonoIFDFSSolverBase::LogCategory.str());
  // }

  void doAnalysisAndCompareResults(
      const llvm::Twine &IRFile, const GroundTruthTy &GroundTruth,
      LLVMTaintConfig *CustomConfig = nullptr,
      std::source_location Loc = std::source_location::current()) {
    std::optional<LLVMTaintConfig> ConfigBuf;
    const auto &Config =
        CustomConfig ? *CustomConfig : ConfigBuf.emplace(getDefaultConfig());

    HelperAnalyses HA(PathToLlFiles + IRFile, EntryPoints,
                      {.CGTy = CallGraphAnalysisType::VTA});
    ASSERT_TRUE(HA.getProjectIRDB().isValid());

    auto &IRDB = HA.getProjectIRDB();
    auto &ICF = HA.getICFG();

    const auto &CG = ICF.getCallGraph();
    auto FC = compressFunctions(CG, psr::getEntryFunctions(IRDB, EntryPoints));

    auto SCCs = computeCGSCCs(CG, ICF, FC);
    auto SCCCallers = computeCGSCCCallers(CG, ICF, FC, SCCs);

    auto UsedGlobals = computeUsedGlobals(IRDB, FC, SCCs, SCCCallers);

    auto AI = HA.getAliasInfo();
    FilteredLLVMAliasIterator FAI(AI);
    CachedLLVMAliasIterator CAI(&FAI);
    GroundTruthCollector GT(convertTestingLocationSetMapInIR(GroundTruth, IRDB),
                            Loc);
    monoifds::TaintAnalysis TA(&Config, &UsedGlobals, &CAI);
    TA.setAnalysisPrinter(&GT);
    monoifds::MonoIFDSSolver Solver(&TA, &ICF);
    Solver.setCGSCCs(&SCCs).setFunctionCompressor(&FC);
    GT.onInitialize();
    Solver.solve();

    GT.onFinalize();
  }
};

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_01) {
  auto Entry = LineColFun{.Line = 6, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 6, .Col = 8, .InFunction = "main"};
  doAnalysisAndCompareResults("dummy_source_sink/taint_01_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_01_m2r) {
  auto Entry = LineColFun{.Line = 6, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 5, .Col = 11, .InFunction = "main"};
  doAnalysisAndCompareResults("dummy_source_sink/taint_01_cpp_m2r_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_02) {
  // source() is not called, so no leak
  doAnalysisAndCompareResults("dummy_source_sink/taint_02_cpp_dbg.ll", {});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_03) {
  auto Entry = LineColFun{.Line = 6, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 6, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_03_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_04) {
  auto Entry = LineColFun{.Line = 6, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 6, .Col = 8, .InFunction = "main"};
  auto EntryThree = LineColFun{.Line = 8, .Col = 3, .InFunction = "main"};
  auto EntryFour = LineColFun{.Line = 8, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_04_cpp_dbg.ll",
                              {
                                  {Entry, {EntryTwo}},
                                  {EntryThree, {EntryFour}},
                              });
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_05) {
  auto Entry = LineColFun{.Line = 6, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 6, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_05_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_06) {
  // source() is not called, so no leak
  doAnalysisAndCompareResults("dummy_source_sink/taint_06_cpp_m2r_dbg.ll", {});
}

TEST_F(MonoIFDSTaintAnalysisTest, SRetTest_01) {
  auto SinkCall = LineColFun{.Line = 21, .Col = 3, .InFunction = "main"};
  auto BsdataAt0 = LineColFunOp{
      .Line = 21,
      .Col = 8,
      .InFunction = "main",
      .OpCode = llvm::Instruction::Load,
  };

  doAnalysisAndCompareResults("dummy_source_sink/sret_c_dbg.ll",
                              {{SinkCall, {BsdataAt0}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01) {
  auto Entry = LineColFun{.Line = 12, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 12, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_01_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01_m2r) {
  auto Entry = LineColFun{.Line = 12, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 10, .Col = 14, .InFunction = "main"};

  doAnalysisAndCompareResults(
      "dummy_source_sink/taint_exception_01_cpp_m2r_dbg.ll",
      {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_02) {
  // source() is not called, so no leak
  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_02_cpp_dbg.ll",
                              {});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_03) {
  auto Entry = LineColFun{.Line = 11, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 11, .Col = 8, .InFunction = "main"};
  auto EntryThree = LineColFun{.Line = 14, .Col = 3, .InFunction = "main"};
  auto EntryFour = LineColFun{.Line = 14, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_03_cpp_dbg.ll",
                              {
                                  {Entry, {EntryTwo}},
                                  {EntryThree, {EntryFour}},
                              });
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_04) {
  auto Entry = LineColFun{.Line = 16, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 16, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_04_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_05) {
  auto Entry = LineColFun{.Line = 16, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 16, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_05_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_06) {
  auto Entry = LineColFun{.Line = 13, .Col = 5, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 13, .Col = 10, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_06_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_07) {
  auto Entry = LineColFun{.Line = 14, .Col = 5, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 14, .Col = 10, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_07_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_08) {
  auto Entry = LineColFun{.Line = 19, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 19, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_08_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_09) {
  auto Entry = LineColFun{.Line = 20, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 20, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_09_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_ExceptionHandling_10) {
  auto Entry = LineColFun{.Line = 19, .Col = 5, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 19, .Col = 10, .InFunction = "main"};

  doAnalysisAndCompareResults("dummy_source_sink/taint_exception_10_cpp_dbg.ll",
                              {{Entry, {EntryTwo}}});
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_DoubleFree_01) {
  auto DoubleFreeConf = getDoubleFreeConfig();
  auto Entry = LineColFun{.Line = 6, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 6, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("double_free_01_c_dbg.ll", {{Entry, {EntryTwo}}},
                              &DoubleFreeConf);
}

TEST_F(MonoIFDSTaintAnalysisTest, TaintTest_DoubleFree_02) {
  auto DoubleFreeConf = getDoubleFreeConfig();
  auto Entry = LineColFun{.Line = 8, .Col = 3, .InFunction = "main"};
  auto EntryTwo = LineColFun{.Line = 8, .Col = 8, .InFunction = "main"};

  doAnalysisAndCompareResults("double_free_02_c_dbg.ll", {{Entry, {EntryTwo}}},
                              &DoubleFreeConf);
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
