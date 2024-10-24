#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DefaultAnalysisPrinter.h"

#include "llvm/ADT/DenseMap.h"

#include "TestConfig.h"
#include "gtest/gtest.h"
using namespace psr;
using CallBackPairTy = std::pair<IDEExtendedTaintAnalysis<>::config_callback_t,
                                 IDEExtendedTaintAnalysis<>::config_callback_t>;

// Use template to variate between Typesate and Taint analysis
class GroundTruthCollector
    : public DefaultAnalysisPrinter<IDEExtendedTaintAnalysisDomain> {

  using n_t = IDEExtendedTaintAnalysisDomain::n_t;
  using d_t = IDEExtendedTaintAnalysisDomain::d_t;
  using l_t = IDEExtendedTaintAnalysisDomain::l_t;

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

private:
  void doOnResult(n_t Instr, d_t DfFact, l_t /*LatticeElement*/,
                  DataFlowAnalysisType /*AnalysisType*/) override {
    llvm::DenseMap<int, std::set<std::string>> FoundLeak;
    int SinkId = stoi(getMetaDataID(Instr));
    std::set<std::string> LeakedValueIds;
    LeakedValueIds.insert(getMetaDataID((DfFact)->base()));
    FoundLeak.try_emplace(SinkId, LeakedValueIds);
    findAndRemove(FoundLeak, GroundTruth);
  }

  void doOnFinalize() override { EXPECT_TRUE(GroundTruth.empty()); }

  llvm::DenseMap<int, std::set<std::string>> GroundTruth{};
};

class AnalysisPrinterTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles = PHASAR_BUILD_SUBFOLDER("xtaint/");
  const std::vector<std::string> EntryPoints = {"main"};

  void doAnalysisTest(
      llvm::StringRef IRFile, GroundTruthCollector &GTPrinter,
      std::variant<std::monostate, TaintConfigData *, CallBackPairTy> Config) {
    HelperAnalyses Helpers(PathToLlFiles + IRFile, EntryPoints);

    auto TConfig = std::visit(
        Overloaded{[&](std::monostate) {
                     return LLVMTaintConfig(Helpers.getProjectIRDB());
                   },
                   [&](TaintConfigData *JS) {
                     auto Ret = LLVMTaintConfig(Helpers.getProjectIRDB(), *JS);
                     return Ret;
                   },
                   [&](CallBackPairTy &&CB) {
                     return LLVMTaintConfig(std::move(CB.first),
                                            std::move(CB.second));
                   }},
        std::move(Config));

    auto TaintProblem = createAnalysisProblem<IDEExtendedTaintAnalysis<>>(
        Helpers, TConfig, EntryPoints);

    TaintProblem.setAnalysisPrinter(&GTPrinter);
    IDESolver Solver(TaintProblem, &Helpers.getICFG());
    Solver.solve();

    TaintProblem.emitTextReport(Solver.getSolverResults());
  }
};

/* ============== BASIC TESTS ============== */

TEST_F(AnalysisPrinterTest, HandleBasicTest_01) {
  llvm::DenseMap<int, std::set<std::string>> GroundTruth;
  GroundTruth[7] = {"0"};

  TaintConfigData Config;

  FunctionData FuncDataMain;
  FuncDataMain.Name = "main";
  FuncDataMain.SourceValues.push_back(0);

  FunctionData FuncDataPrint;
  FuncDataPrint.Name = "_Z5printi";
  FuncDataPrint.SinkValues.push_back(0);

  Config.Functions.push_back(std::move(FuncDataMain));
  Config.Functions.push_back(std::move(FuncDataPrint));

  GroundTruthCollector GroundTruthPrinter = {GroundTruth};
  doAnalysisTest("xtaint01_json_cpp_dbg.ll", GroundTruthPrinter, &Config);
}

TEST_F(AnalysisPrinterTest, XTaint01) {
  llvm::DenseMap<int, std::set<std::string>> GroundTruth;

  GroundTruth[13] = {"7"};
  GroundTruthCollector GroundTruthPrinter = {GroundTruth};
  doAnalysisTest("xtaint01_cpp.ll", GroundTruthPrinter, std::monostate{});
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
