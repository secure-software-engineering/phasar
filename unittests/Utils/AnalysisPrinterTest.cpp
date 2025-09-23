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

#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;
using namespace psr::unittest;

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
  GroundTruthCollector(
      const LLVMProjectIRDB &IRDB,
      const std::map<TestingSrcLocation, std::set<TestingSrcLocation>>
          &GroundTruth)
      : GroundTruth(convertTestingLocationSetMapInIR(GroundTruth, IRDB)) {};

private:
  void doOnResult(n_t Instr, d_t DfFact, l_t /*LatticeElement*/,
                  DataFlowAnalysisType /*AnalysisType*/) override {

    auto It = GroundTruth.find(Instr);
    EXPECT_TRUE(It != GroundTruth.end() && It->second.erase(DfFact->base()))
        << "Did not expect finding a leak of " << DToString(DfFact) << " at "
        << llvmIRToString(Instr) << '\n';
    if (It != GroundTruth.end() && It->second.empty()) {
      GroundTruth.erase(It);
    }
  }

  std::string printGroundTruth() {
    std::string Ret;
    llvm::raw_string_ostream OS(Ret);
    OS << "{\n";
    for (const auto &[Inst, Facts] : GroundTruth) {
      OS << "  " << llvmIRToString(Inst) << ": {\n";
      for (const auto *Val : Facts) {
        OS << "    " << llvmIRToString(Val) << '\n';
      }
    }
    OS << "}\n";

    return Ret;
  }

  void doOnFinalize(llvm::raw_ostream & /*OS*/) override {
    EXPECT_TRUE(GroundTruth.empty())
        << "Elements of GroundTruth not found: " << printGroundTruth();
  }

  std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
      GroundTruth{};
};

class AnalysisPrinterTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles = PHASAR_BUILD_SUBFOLDER("xtaint/");
  const std::vector<std::string> EntryPoints = {"main"};

  void doAnalysisTest(
      llvm::StringRef IRFile,
      const std::map<TestingSrcLocation, std::set<TestingSrcLocation>> &GT,
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

    GroundTruthCollector GTCollector(Helpers.getProjectIRDB(), GT);
    TaintProblem.setAnalysisPrinter(&GTCollector);
    IDESolver Solver(TaintProblem, &Helpers.getICFG());
    Solver.solve();

    TaintProblem.emitTextReport(Solver.getSolverResults());
  }
};

/* ============== BASIC TESTS ============== */

TEST_F(AnalysisPrinterTest, HandleBasicTest_01) {
  std::map<TestingSrcLocation, std::set<TestingSrcLocation>> GroundTruth;
  GroundTruth[LineColFun{8, 3, "main"}] = {
      LineColFun{4, 14, "main"},
  };

  TaintConfigData Config;

  FunctionData FuncDataMain;
  FuncDataMain.Name = "main";
  FuncDataMain.SourceValues.push_back(0);

  FunctionData FuncDataPrint;
  FuncDataPrint.Name = "_Z5printi";
  FuncDataPrint.SinkValues.push_back(0);

  Config.Functions.push_back(std::move(FuncDataMain));
  Config.Functions.push_back(std::move(FuncDataPrint));

  doAnalysisTest("xtaint01_json_cpp_dbg.ll", GroundTruth, &Config);
}

TEST_F(AnalysisPrinterTest, XTaint01) {
  std::map<TestingSrcLocation, std::set<TestingSrcLocation>> GroundTruth;

  GroundTruth[LineColFun{8, 3, "main"}] = {
      LineColFun{7, 48, "main"},
  };
  doAnalysisTest("xtaint01_cpp_dbg.ll", GroundTruth, std::monostate{});
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
