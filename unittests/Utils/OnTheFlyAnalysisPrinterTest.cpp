#include "phasar/Utils/OnTheFlyAnalysisPrinter.h"

#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

namespace {

using namespace psr;
using namespace psr::unittest;

using GroundTruthTy =
    std::map<TestingSrcLocation, std::set<TestingSrcLocation>>;

class GroundTruthCollector
    : public OnTheFlyAnalysisPrinter<LLVMIFDSAnalysisDomainDefault> {
  using n_t = LLVMIFDSAnalysisDomainDefault::n_t;
  using d_t = LLVMIFDSAnalysisDomainDefault::d_t;
  using l_t = LLVMIFDSAnalysisDomainDefault::l_t;

public:
  // constructor init Groundtruth in each fixture
  GroundTruthCollector(GroundTruthTy GroundTruth)
      : GroundTruth(std::move(GroundTruth)) {};

  void convertGT(const LLVMProjectIRDB &IRDB) {
    LLVMGroundTruth = convertTestingLocationSetMapInIR(GroundTruth, IRDB);
  }

private:
  void doOnResult(n_t Instr, d_t DfFact, l_t /*LatticeElement*/,
                  DataFlowAnalysisType /*AnalysisType*/) override {
    assert(LLVMGroundTruth);
    auto It = LLVMGroundTruth->find(Instr);
    ASSERT_NE(It, LLVMGroundTruth->end())
        << "Found leak at unexpected location: " << llvmIRToString(Instr)
        << ": '" << llvmIRToString(DfFact) << "'";

    bool Erased = It->second.erase(DfFact);
    ASSERT_TRUE(Erased) << "Did not expect leak '" << llvmIRToString(DfFact)
                        << "' at instruction " << llvmIRToString(Instr);

    if (It->second.empty()) {
      LLVMGroundTruth->erase(It);
    }
  }

  void doOnFinalize() override {
    assert(LLVMGroundTruth);
    EXPECT_TRUE(LLVMGroundTruth->empty());
  }

  GroundTruthTy GroundTruth{};
  std::optional<
      std::map<const llvm::Instruction *, std::set<const llvm::Value *>>>
      LLVMGroundTruth;
};

class OnTheFlyAnalysisPrinterTest : public ::testing::Test {
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

  void doAnalysisTest(llvm::StringRef IRFile, GroundTruthCollector &GTPrinter) {
    initialize(IRFile);
    GTPrinter.convertGT(HA->getProjectIRDB());
    UnInitProblem->setAnalysisPrinter(&GTPrinter);
    IFDSSolver Solver(*UnInitProblem, &HA->getICFG());
    Solver.solve();
    GTPrinter.onFinalize();
  }
};

/* ============== BASIC TESTS ============== */

TEST_F(OnTheFlyAnalysisPrinterTest, UninitTest_01_LEAK) {

  GroundTruthTy GroundTruth;
  const auto Entry = LineColFun{2, 0, "main"};
  const auto EntryTwo = LineColFun{3, 11, "main"};
  const auto EntryThree = LineColFun{3, 13, "main"};
  GroundTruth.insert({EntryTwo, {Entry}});
  GroundTruth.insert({EntryThree, {EntryTwo}});

  GroundTruthCollector GroundTruthPrinter = {GroundTruth};
  doAnalysisTest("binop_uninit_cpp_dbg.ll", GroundTruthPrinter);
}

TEST_F(OnTheFlyAnalysisPrinterTest, UninitTest_02_NOLEAK) {
  GroundTruthTy GroundTruth;
  GroundTruthCollector GroundTruthPrinter = {GroundTruth};
  doAnalysisTest("ctor_default_cpp_dbg.ll", GroundTruthPrinter);
}
} // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
