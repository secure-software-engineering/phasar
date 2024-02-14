#include "phasar/PhasarLLVM/Utils/SourceMgrPrinter.h"

#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/AnalysisPrinterBase.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <set>

using namespace psr;

class GroundTruthCollector
    : public SourceMgrPrinter<LLVMIFDSAnalysisDomainDefault> {
  using n_t = LLVMIFDSAnalysisDomainDefault::n_t;
  using d_t = LLVMIFDSAnalysisDomainDefault::d_t;
  using l_t = LLVMIFDSAnalysisDomainDefault::l_t;

public:
  // constructor init Groundtruth in each fixture
  GroundTruthCollector(std::set<std::string> &GroundTruth)
      : SourceMgrPrinter<LLVMIFDSAnalysisDomainDefault>(
            [](DataFlowAnalysisType) { return ""; }, OS),
        GroundTruth(GroundTruth), OS(Str){};

private:
  void doOnFinalize() override {
    for (auto It = GroundTruth.begin(); It != GroundTruth.end();) {
      if (OS.str().find(*It)) {
        GroundTruth.erase(It++);
      } else {
        ++It;
      }
    }
    EXPECT_TRUE(GroundTruth.empty());
  }

  std::set<std::string> GroundTruth{};
  std::string Str;
  llvm::raw_string_ostream OS;
};

class SourceMgrPrinterTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("uninitialized_variables/");

  const std::vector<std::string> EntryPoints = {"main"};
  std::optional<IFDSUninitializedVariables> UnInitProblem;
  std::optional<HelperAnalyses> HA;

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(PathToLlFiles + IRFile, EntryPoints);
    UnInitProblem =
        createAnalysisProblem<IFDSUninitializedVariables>(*HA, EntryPoints);
  }

  void doAnalysisTest(
      llvm::StringRef IRFile,
      AnalysisPrinterBase<LLVMIFDSAnalysisDomainDefault> &GTPrinter) {
    initialize(IRFile);
    UnInitProblem->setAnalysisPrinter(&GTPrinter);
    IFDSSolver Solver(*UnInitProblem, &HA->getICFG());
    Solver.solve();
    GTPrinter.onFinalize();
  }
};

/* ============== BASIC TESTS ============== */

TEST_F(SourceMgrPrinterTest, UninitTest_01_LEAK) {

  std::set<std::string> GroundTruth;

  GroundTruth = {"binop_uninit.cpp:3:11", "binop_uninit.cpp:3:13"};

  GroundTruthCollector GroundTruthPrinter = {GroundTruth};

  doAnalysisTest("binop_uninit_cpp_dbg.ll", GroundTruthPrinter);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
