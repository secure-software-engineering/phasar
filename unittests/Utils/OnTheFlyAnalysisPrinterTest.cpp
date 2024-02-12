#include "phasar/Utils/OnTheFlyAnalysisPrinter.h"

#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

class GroundTruthCollector
    : public OnTheFlyAnalysisPrinter<LLVMIFDSAnalysisDomainDefault> {
  using n_t = LLVMIFDSAnalysisDomainDefault::n_t;
  using d_t = LLVMIFDSAnalysisDomainDefault::d_t;
  using l_t = LLVMIFDSAnalysisDomainDefault::l_t;

public:
  // constructor init Groundtruth in each fixture
  GroundTruthCollector(llvm::DenseMap<int, std::set<std::string>> &GroundTruth)
      : GroundTruth(GroundTruth){};

  void findAndRemove(llvm::DenseMap<int, std::set<std::string>> &Map1,
                     llvm::DenseMap<int, std::set<std::string>> &Map2) {
    for (auto Entry = Map1.begin(); Entry != Map1.end();) {
      auto Iter = Map2.find(Entry->first);
      if (Iter != Map2.end() && Iter->second == Entry->second) {
        Map2.erase(Iter);
      }
      ++Entry;
    }
  }

  void onResult(n_t Instr, d_t DfFact, l_t /*LatticeElement*/,
                DataFlowAnalysisType /*AnalysisType*/) override {
    llvm::DenseMap<int, std::set<std::string>> FoundLeak;
    int SinkId = stoi(getMetaDataID(Instr));
    std::set<std::string> LeakedValueIds;
    LeakedValueIds.insert(getMetaDataID((DfFact)));
    FoundLeak.try_emplace(SinkId, LeakedValueIds);
    findAndRemove(FoundLeak, GroundTruth);
  }

  void onFinalize() override { EXPECT_TRUE(GroundTruth.empty()); }

private:
  llvm::DenseMap<int, std::set<std::string>> GroundTruth{};
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
    UnInitProblem->setAnalysisPrinter(&GTPrinter);
    IFDSSolver Solver(*UnInitProblem, &HA->getICFG());
    Solver.solve();
    GTPrinter.onFinalize();
  }
};

/* ============== BASIC TESTS ============== */

TEST_F(OnTheFlyAnalysisPrinterTest, UninitTest_01_LEAK) {

  llvm::DenseMap<int, std::set<std::string>> GroundTruth;
  // %4 = load i32, i32* %2, ID: 6 ;  %2 is the uninitialized variable i
  GroundTruth[6] = {"1"};
  // %5 = add nsw i32 %4, 10 ;        %4 is undef, since it is loaded from
  // undefined alloca; not sure if it is necessary to report again
  GroundTruth[7] = {"6"};

  GroundTruthCollector GroundTruthPrinter = {GroundTruth};
  doAnalysisTest("binop_uninit_cpp_dbg.ll", GroundTruthPrinter);
}

TEST_F(OnTheFlyAnalysisPrinterTest, UninitTest_02_NOLEAK) {
  llvm::DenseMap<int, std::set<std::string>> GroundTruth;
  GroundTruthCollector GroundTruthPrinter = {GroundTruth};
  doAnalysisTest("ctor_default_cpp_dbg.ll", GroundTruthPrinter);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
