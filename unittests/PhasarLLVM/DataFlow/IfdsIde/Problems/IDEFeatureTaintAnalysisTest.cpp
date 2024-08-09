#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEFeatureTaintAnalysis.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMSolverResults.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

namespace {

using namespace psr;

static std::string printSet(const std::set<std::string> &EdgeFact) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);
  llvm::interleaveComma(EdgeFact, ROS << '<');
  ROS << '>';
  return Ret;
}

/* ============== TEST FIXTURE ============== */
class IDEInstInteractionAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("inst_interaction/");

  // Function - Line Nr - Variable - Values
  using IIACompactResult_t =
      std::tuple<std::string, std::size_t, std::string, std::set<std::string>>;

  std::optional<HelperAnalyses> HA;
  LLVMProjectIRDB *IRDB{};

  void initializeIR(const llvm::Twine &LlvmFilePath,
                    const std::vector<std::string> &EntryPoints = {"main"}) {
    HA.emplace(PathToLlFiles + LlvmFilePath, EntryPoints,
               HelperAnalysisConfig{}.withCGType(CallGraphAnalysisType::CHA));
    IRDB = &HA->getProjectIRDB();

    for (const auto &Glob : IRDB->getModule()->globals()) {
      BitVectorSet<std::string, llvm::SmallBitVector> BV;
      BV.insert(getMetaDataID(&Glob));
    }

    // Initialze IDs
    for (const auto *Inst : IRDB->getAllInstructions()) {
      BitVectorSet<std::string, llvm::SmallBitVector> BV;
      BV.insert(getMetaDataID(Inst));
    }
  }

  void
  doAnalysisAndCompareResults(const std::string &LlvmFilePath,
                              const std::vector<std::string> &EntryPoints,
                              const std::set<IIACompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    initializeIR(LlvmFilePath, EntryPoints);

    // IDEInstInteractionAnalysisT<std::string, true> IIAProblem(IRDB, &ICFG,
    // &PT,
    //                                                           EntryPoints);

    // use Phasar's instruction ids as testing labels
    auto Generator =
        [](std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>
               Current) -> std::set<std::string> {
      return std::visit(
          [](const auto *InstOrGlob) -> std::set<std::string> {
            std::set<std::string> Labels;
            if (InstOrGlob->hasMetadata()) {
              std::string Label =
                  llvm::cast<llvm::MDString>(
                      InstOrGlob->getMetadata(PhasarConfig::MetaDataKind())
                          ->getOperand(0))
                      ->getString()
                      .str();
              Labels.insert(Label);
            }
            return Labels;
          },
          Current);
    };
    assert(HA);
    auto IIAProblem = createAnalysisProblem<IDEFeatureTaintAnalysis>(
        *HA, EntryPoints, Generator);

    // if (PrintDump) {
    //   psr::Logger::initializeStderrLogger(SeverityLevel::DEBUG);
    // }

    IDESolver IIASolver(IIAProblem, &HA->getICFG());
    IIASolver.solve();
    if (PrintDump) {
      // IRDB->emitPreprocessedIR(llvm::outs());
      IIASolver.dumpResults();
      llvm::outs()
          << "\n======================================================\n";
      printDump(HA->getProjectIRDB(), IIASolver.getSolverResults());
    }
    // do the comparison
    for (const auto &[FunName, SrcLine, VarName, LatticeVal] : GroundTruth) {
      const auto *Fun = IRDB->getFunctionDefinition(FunName);
      const auto *IRLine = getNthInstruction(Fun, SrcLine);
      auto ResultMap = IIASolver.resultsAt(IRLine);
      assert(IRLine && "Could not retrieve IR line!");
      bool FactFound = false;
      for (auto &[Fact, Value] : ResultMap) {
        std::string FactStr;
        llvm::raw_string_ostream RSO(FactStr);
        RSO << *Fact;
        llvm::StringRef FactRef(FactStr);
        if (FactRef.ltrim().startswith("%" + VarName + " ") ||
            FactRef.ltrim().startswith("@" + VarName + " ")) {
          PHASAR_LOG_LEVEL(DFADEBUG, "Checking variable: " << FactStr);
          EXPECT_EQ(LatticeVal, Value.toSet<std::string>())
              << "Value do not match for Variable '" << VarName
              << "': Expected " << printSet(LatticeVal)
              << "; got: " << LToString(Value.toBVSet<std::string>());
          FactFound = true;
        }
      }

      EXPECT_TRUE(FactFound) << "Variable '" << VarName << "' missing at '"
                             << llvmIRToString(IRLine) << "'.";
    }
  }

  void TearDown() override {}

  // See vara::PhasarTaintAnalysis::taintsForInst
  [[nodiscard]] inline std::set<std::string>
  taintsForInst(const llvm::Instruction *Inst,
                SolverResults<const llvm::Instruction *, const llvm::Value *,
                              IDEFeatureTaintEdgeFact>
                    SR) {

    if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(Inst)) {
      if (Ret->getNumOperands() == 0) {
        return {};
      }
    } else if (llvm::isa<llvm::UnreachableInst>(Inst)) {
      return {};
    }

    std::set<std::string> AggregatedTaints;

    if (Inst->getType()->isVoidTy()) { // For void types, we need to look what
                                       // taints flow into the inst

      // auto Results = SR.resultsAt(Inst);
      assert(Inst->getNumOperands() >= 1 &&
             "Found case without first operand.");
      AggregatedTaints =
          SR.resultAt(Inst, Inst->getOperand(0)).toSet<std::string>();

    } else {
      auto Results = SR.resultsAtInLLVMSSA(Inst);
      auto SearchPosTaints = Results.find(Inst);
      if (SearchPosTaints != Results.end()) {
        AggregatedTaints = SearchPosTaints->second.toSet<std::string>();
      }
    }

    // additionalStaticTaints
    AggregatedTaints.insert(getMetaDataID(Inst));

    return AggregatedTaints;
  }

  void printDump(const LLVMProjectIRDB &IRDB,
                 SolverResults<const llvm::Instruction *, const llvm::Value *,
                               IDEFeatureTaintEdgeFact>
                     SR) {
    const llvm::Function *CurrFun = nullptr;
    for (const auto *Inst : IRDB.getAllInstructions()) {
      if (CurrFun != Inst->getFunction()) {
        CurrFun = Inst->getFunction();
        llvm::outs() << "\n=================== '" << CurrFun->getName()
                     << "' ===================\n";
      }
      llvm::outs() << "  N: " << llvmIRToString(Inst) << '\n';
      llvm::outs() << "  D: " << printSet(taintsForInst(Inst, SR)) << "\n\n";
    }
  }

}; // Test Fixture

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 9, "i", std::set<std::string>{"4"});
  GroundTruth.emplace("main", 9, "j",
                      std::set<std::string>{"4", "5", "6", "7"});
  GroundTruth.emplace("main", 9, "retval", std::set<std::string>{"3"});
  doAnalysisAndCompareResults("basic_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 24, "retval", std::set<std::string>{"6"});
  GroundTruth.emplace("main", 24, "argc.addr", std::set<std::string>{"7"});
  GroundTruth.emplace("main", 24, "argv.addr", std::set<std::string>{"8"});
  GroundTruth.emplace("main", 24, "i", std::set<std::string>{"16", "18"});
  GroundTruth.emplace("main", 24, "j",
                      std::set<std::string>{"9", "10", "11", "12"});
  GroundTruth.emplace("main", 24, "k",
                      std::set<std::string>{"21", "16", "18", "20"});
  doAnalysisAndCompareResults("basic_02_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 20, "retval", std::set<std::string>{"3"});
  GroundTruth.emplace("main", 20, "i",
                      std::set<std::string>{"4", "10", "11", "12"});
  GroundTruth.emplace("main", 20, "x",
                      std::set<std::string>{"5", "14", "15", "16"});
  doAnalysisAndCompareResults("basic_03_cpp.ll", {"main"}, GroundTruth, false);
}

PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_04) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 23, "retval", std::set<std::string>{"13"});
  GroundTruth.emplace("main", 23, "argc.addr", std::set<std::string>{"14"});
  GroundTruth.emplace("main", 23, "argv.addr", std::set<std::string>{"15"});
  GroundTruth.emplace("main", 23, "i", std::set<std::string>{"16"});
  GroundTruth.emplace("main", 23, "j",
                      std::set<std::string>{"16", "17", "19", "18"});
  GroundTruth.emplace(
      "main", 23, "k",
      std::set<std::string>{"16", "17", "18", "19", "20", "24", "25"});
  doAnalysisAndCompareResults("basic_04_cpp.ll", {"main"}, GroundTruth, false);
})

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 11, "i", std::set<std::string>{"5", "7"});
  GroundTruth.emplace("main", 11, "retval", std::set<std::string>{"2"});
  doAnalysisAndCompareResults("basic_05_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 19, "retval", std::set<std::string>{"5"});
  GroundTruth.emplace("main", 19, "i", std::set<std::string>{"15", "6", "13"});
  GroundTruth.emplace("main", 19, "j", std::set<std::string>{"15", "6", "13"});
  GroundTruth.emplace("main", 19, "k", std::set<std::string>{"6"});
  GroundTruth.emplace("main", 19, "p",
                      std::set<std::string>{"1", "2", "9", "11"});
  doAnalysisAndCompareResults("basic_06_cpp.ll", {"main"}, GroundTruth, true);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 15, "retval", std::set<std::string>{"5"});
  GroundTruth.emplace("main", 15, "argc.addr", std::set<std::string>{"6"});
  GroundTruth.emplace("main", 15, "argv.addr", std::set<std::string>{"7"});
  GroundTruth.emplace("main", 15, "i", std::set<std::string>{"12"});
  GroundTruth.emplace("main", 15, "j",
                      std::set<std::string>{"8", "9", "10", "11"});
  doAnalysisAndCompareResults("basic_07_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_08) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 12, "retval", std::set<std::string>{"2"});
  GroundTruth.emplace("main", 12, "i", std::set<std::string>{"9"});
  doAnalysisAndCompareResults("basic_08_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_09) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 10, "i", std::set<std::string>{"4"});
  GroundTruth.emplace("main", 10, "j", std::set<std::string>{"4", "6", "7"});
  GroundTruth.emplace("main", 10, "retval", std::set<std::string>{"3"});
  doAnalysisAndCompareResults("basic_09_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_10) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "i", std::set<std::string>{"3"});
  GroundTruth.emplace("main", 6, "retval", std::set<std::string>{"2"});
  doAnalysisAndCompareResults("basic_10_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_11) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 20, "FeatureSelector",
                      std::set<std::string>{"5", "7", "8"});
  GroundTruth.emplace("main", 20, "retval", std::set<std::string>{"11", "16"});
  doAnalysisAndCompareResults("basic_11_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 14, "retval", std::set<std::string>{"8"});
  GroundTruth.emplace("main", 14, "i", std::set<std::string>{"9"});
  GroundTruth.emplace("main", 14, "j",
                      std::set<std::string>{"12", "9", "10", "11"});
  GroundTruth.emplace(
      "main", 14, "k",
      std::set<std::string>{"15", "1", "2", "13", "12", "9", "10", "11"});
  doAnalysisAndCompareResults("call_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 13, "retval", std::set<std::string>{"12"});
  GroundTruth.emplace("main", 13, "i", std::set<std::string>{"13"});
  GroundTruth.emplace("main", 13, "j", std::set<std::string>{"14"});
  GroundTruth.emplace("main", 13, "k",
                      std::set<std::string>{"4", "5", "15", "6", "3", "14", "2",
                                            "13", "16", "18"});
  doAnalysisAndCompareResults("call_02_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 10, "retval", std::set<std::string>{"20"});
  GroundTruth.emplace("main", 10, "i", std::set<std::string>{"21"});
  GroundTruth.emplace("main", 10, "j",
                      std::set<std::string>{"22", "15", "6", "21", "2", "13",
                                            "8", "9", "12", "10", "24"});
  doAnalysisAndCompareResults("call_03_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 20, "retval", std::set<std::string>{"33"});
  GroundTruth.emplace("main", 20, "i", std::set<std::string>{"34"});
  GroundTruth.emplace("main", 20, "j",
                      std::set<std::string>{"15", "6", "2", "13", "8", "9",
                                            "12", "10", "35", "34", "37"});
  GroundTruth.emplace("main", 20, "k",
                      std::set<std::string>{
                          "41", "19", "15", "6",  "44", "2",  "13", "8",  "45",
                          "18", "9",  "12", "10", "46", "24", "25", "35", "27",
                          "23", "26", "38", "34", "37", "42", "40"});
  doAnalysisAndCompareResults("call_04_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 10, "retval", std::set<std::string>{"8"});
  GroundTruth.emplace("main", 10, "i", std::set<std::string>{"3", "11", "9"});
  GroundTruth.emplace("main", 10, "j", std::set<std::string>{"3", "10", "12"});
  doAnalysisAndCompareResults("call_05_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_06) {
  // NOTE: Here we are suffering from IntraProceduralAliasesOnly
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 24, "retval", std::set<std::string>{"11"});
  GroundTruth.emplace("main", 24, "i",
                      std::set<std::string>{"3", "1", "2", "16", "18", "12"});
  GroundTruth.emplace("main", 24, "j",
                      std::set<std::string>{"19", "21", "3", "1", "2", "13"});
  GroundTruth.emplace("main", 24, "k",
                      std::set<std::string>{"22", "3", "14", "1", "2", "24"});
  GroundTruth.emplace("main", 24, "l",
                      std::set<std::string>{"15", "3", "1", "2", "25", "27"});
  doAnalysisAndCompareResults("call_06_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "retval", std::set<std::string>{"7"});
  GroundTruth.emplace("main", 6, "VarIR", std::set<std::string>{"6", "3", "8"});
  doAnalysisAndCompareResults("call_07_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 9, "retval", std::set<std::string>{"3"});
  GroundTruth.emplace("main", 9, "i", std::set<std::string>{"7"});
  GroundTruth.emplace("main", 9, "j", std::set<std::string>{"0", "5", "6"});
  doAnalysisAndCompareResults("global_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z5initBv", 2, "a", std::set<std::string>{"0"});
  GroundTruth.emplace("_Z5initBv", 2, "b", std::set<std::string>{"2"});
  GroundTruth.emplace("main", 12, "a", std::set<std::string>{"0"});
  GroundTruth.emplace("main", 12, "b", std::set<std::string>{"2"});
  GroundTruth.emplace("main", 12, "retval", std::set<std::string>{"6"});
  GroundTruth.emplace("main", 12, "c", std::set<std::string>{"1", "8", "7"});
  doAnalysisAndCompareResults("global_02_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 1, "GlobalFeature", std::set<std::string>{"0"});
  GroundTruth.emplace("main", 2, "GlobalFeature", std::set<std::string>{"0"});
  GroundTruth.emplace("main", 17, "GlobalFeature", std::set<std::string>{"0"});
  doAnalysisAndCompareResults("global_03_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 1, "GlobalFeature", std::set<std::string>{"0"});
  GroundTruth.emplace("main", 2, "GlobalFeature", std::set<std::string>{"0"});
  GroundTruth.emplace("main", 17, "GlobalFeature", std::set<std::string>{"0"});
  GroundTruth.emplace("_Z7doStuffi", 1, "GlobalFeature",
                      std::set<std::string>{"0"});
  GroundTruth.emplace("_Z7doStuffi", 2, "GlobalFeature",
                      std::set<std::string>{"0"});
  doAnalysisAndCompareResults("global_04_cpp.ll", {"main", "_Z7doStuffi"},
                              GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 12, "retval", std::set<std::string>{"4"});
  GroundTruth.emplace("main", 12, "i", std::set<std::string>{"5"});
  GroundTruth.emplace("main", 12, "j", std::set<std::string>{"10"});
  GroundTruth.emplace("main", 12, "k", std::set<std::string>{"9", "8", "5"});
  doAnalysisAndCompareResults("KillTest_01_cpp.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 12, "retval", std::set<std::string>{"6"});
  GroundTruth.emplace("main", 12, "A", std::set<std::string>{"0"});
  GroundTruth.emplace("main", 12, "B", std::set<std::string>{"2"});
  GroundTruth.emplace("main", 12, "C", std::set<std::string>{"1", "7", "8"});
  doAnalysisAndCompareResults("KillTest_02_cpp.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleReturnTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "retval", std::set<std::string>{"3"});
  GroundTruth.emplace("main", 6, "localVar", std::set<std::string>{"4"});
  GroundTruth.emplace("main", 6, "call", std::set<std::string>{"0"});
  GroundTruth.emplace("main", 8, "localVar", std::set<std::string>{"0", "6"});
  GroundTruth.emplace("main", 8, "call", std::set<std::string>{"0"});
  doAnalysisAndCompareResults("return_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleHeapTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 19, "retval", std::set<std::string>{"3"});
  GroundTruth.emplace("main", 19, "i", std::set<std::string>{"6", "7"});
  GroundTruth.emplace("main", 19, "j",
                      std::set<std::string>{"6", "7", "8", "10", "9"});
  doAnalysisAndCompareResults("heap_01_cpp.ll", {"main"}, GroundTruth, false);
}

PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleRVOTest_01) {
  GTEST_SKIP() << "This test heavily depends on the used stdlib version. TODO: "
                  "add a better one";

  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 16, "retval", std::set<std::string>{"75", "76"});
  GroundTruth.emplace("main", 16, "str",
                      std::set<std::string>{"70", "65", "72", "74", "77"});
  GroundTruth.emplace("main", 16, "ref.tmp",
                      std::set<std::string>{"66", "9", "72", "73", "71"});
  doAnalysisAndCompareResults("rvo_01_cpp.ll", {"main"}, GroundTruth, false);
})

PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleRVOTest_02) {
  // GTEST_SKIP() << "This test heavily depends on the used stdlib version.
  // TODO: "
  //                 "add a better one";

  std::set<IIACompactResult_t> GroundTruth;
  // GroundTruth.emplace("main", 16, "retval", std::set<std::string>{"75",
  // "76"}); GroundTruth.emplace("main", 16, "str",
  //                     std::set<std::string>{"70", "65", "72", "74", "77"});
  // GroundTruth.emplace("main", 16, "ref.tmp",
  //                     std::set<std::string>{"66", "9", "72", "73", "71"});
  doAnalysisAndCompareResults("rvo_02_cpp.ll", {"main"}, GroundTruth, true);

  ASSERT_FALSE(true) << "Add GroundTruth!";
})

TEST_F(IDEInstInteractionAnalysisTest, HandleRVOTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  // GroundTruth.emplace("main", 16, "retval", std::set<std::string>{"75",
  // "76"}); GroundTruth.emplace("main", 16, "str",
  //                     std::set<std::string>{"70", "65", "72", "74", "77"});
  // GroundTruth.emplace("main", 16, "ref.tmp",
  //                     std::set<std::string>{"66", "9", "72", "73", "71"});
  doAnalysisAndCompareResults("rvo_03_cpp.ll", {"main"}, GroundTruth, true);

  ASSERT_FALSE(true) << "Add GroundTruth!";
}

TEST_F(IDEInstInteractionAnalysisTest, HandleRVOTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  // GroundTruth.emplace("main", 16, "retval", std::set<std::string>{"75",
  // "76"}); GroundTruth.emplace("main", 16, "str",
  //                     std::set<std::string>{"70", "65", "72", "74", "77"});
  // GroundTruth.emplace("main", 16, "ref.tmp",
  //                     std::set<std::string>{"66", "9", "72", "73", "71"});
  doAnalysisAndCompareResults("rvo_04_cpp.ll", {"main"}, GroundTruth, true);

  ASSERT_FALSE(true) << "Add GroundTruth!";
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
