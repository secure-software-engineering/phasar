#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEFeatureTaintAnalysis.h"

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMSolverResults.h" // for resultsAtInLLVMSSA
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/Fn.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <set>

namespace {

using namespace psr;
using namespace psr::unittest;

using TaintSetT = std::set<TestingSrcLocation>;

static std::string printSet(const TaintSetT &EdgeFact) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);
  llvm::interleaveComma(EdgeFact, ROS << '<');
  ROS << '>';
  return Ret;
}

/* ============== TEST FIXTURE ============== */
class IDEFeatureTaintAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("inst_interaction/");

  using VarNameT = std::variant<std::string, unittest::RetVal>;
  // Function - Line Nr - Variable - Values
  using IIACompactResult_t =
      std::tuple<TestingSrcLocation, VarNameT, std::set<TestingSrcLocation>>;

  std::optional<HelperAnalyses> HA;
  LLVMProjectIRDB *IRDB{};

  void initializeIR(const llvm::Twine &LlvmFilePath,
                    const std::vector<std::string> &EntryPoints = {"main"}) {
    HA.emplace(PathToLlFiles + LlvmFilePath, EntryPoints,
               HelperAnalysisConfig{}.withCGType(CallGraphAnalysisType::CHA));
    IRDB = &HA->getProjectIRDB();
  }

  [[nodiscard]] bool matchesVar(const llvm::Value *Fact,
                                const VarNameT &VarName) {
    return std::visit(
        psr::Overloaded{
            [&](const std::string &Name) {
              if (!llvm::isa<llvm::AllocaInst>(Fact) &&
                  !llvm::isa<llvm::GlobalVariable>(Fact)) {
                return false;
              }
              auto FactName = psr::getVarNameFromIR(Fact);
              return FactName == Name;
            },
            [&](RetVal R) {
              return llvm::any_of(Fact->users(), [R](const auto *V) {
                const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(V);
                return Ret && Ret->getFunction()->getName() == R.InFunction;
              });
            },
        },
        VarName);
  }
  [[nodiscard]] std::string printVar(const VarNameT &VarName) {
    return std::visit(psr::Overloaded{
                          [](const std::string &Name) { return Name; },
                          [](RetVal R) { return R.str(); },
                      },
                      VarName);
  }

  static TaintSetT generateTaintsAtInst(const llvm::Instruction *Inst) {
    TaintSetT Labels;
    auto [Line, Col] = getLineAndColFromIR(Inst);
    if (Col == 0 && llvm::isa<llvm::StoreInst>(Inst)) {
      std::tie(Line, Col) = getLineAndColFromIR(
          Inst->getOperand(llvm::StoreInst::getPointerOperandIndex()));
    }
    if (Line != 0) {
      Labels.insert(LineColFun{
          Line,
          Col,
          Inst->getFunction()->getName(),
      });
    }
    return Labels;
  }

  void
  doAnalysisAndCompareResults(const std::string &LlvmFilePath,
                              const std::vector<std::string> &EntryPoints,
                              const std::set<IIACompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    initializeIR(LlvmFilePath, EntryPoints);

    // use Phasar's instruction ids as testing labels
    auto Generator =
        [](std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>
               Current) -> TaintSetT {
      return std::visit(
          psr::Overloaded{
              [](const llvm::GlobalVariable *Glob) {
                return TaintSetT{GlobalVar{Glob->getName()}};
              },
              fn<&generateTaintsAtInst>,
          },
          Current);
    };
    assert(HA);
    auto IIAProblem = createAnalysisProblem<IDEFeatureTaintAnalysis>(
        *HA, EntryPoints, Generator);

    IDESolver IIASolver(IIAProblem, &HA->getICFG());
    // IterativeIDESolver IIASolver(&IIAProblem, &HA->getICFG());
    auto Results = IIASolver.solve();

    // do the comparison
    for (const auto &[InstLoc, VarName, ExpectedVal] : GroundTruth) {
      const auto *IRLoc = testingLocInIR(InstLoc, *IRDB);
      ASSERT_TRUE(IRLoc) << "Could not retrieve IR Loc: " << InstLoc.str();
      ASSERT_TRUE(llvm::isa<llvm::Instruction>(IRLoc));
      auto ResultMap =
          IIASolver.resultsAt(llvm::cast<llvm::Instruction>(IRLoc));
      bool FactFound = false;
      for (auto &[Fact, ComputedVal] : ResultMap) {
        if (matchesVar(Fact, VarName)) {
          EXPECT_EQ(ExpectedVal, ComputedVal.toSet<TestingSrcLocation>())
              << "Unexpected taint-set at " << InstLoc << " for variable '"
              << printVar(VarName) << "' (" << llvmIRToString(Fact) << ")";
          FactFound = true;
        }
      }

      EXPECT_TRUE(FactFound)
          << "Variable '" << printVar(VarName) << "' missing at '"
          << llvmIRToString(IRLoc) << "'.";
    }

    if (PrintDump || HasFailure()) {
      IIASolver.dumpResults(llvm::errs());
      llvm::errs()
          << "\n======================================================\n";
      printDump(HA->getProjectIRDB(), Results);
    }
  }

  void TearDown() override {
    BitVectorSet<TestingSrcLocation, llvm::SmallBitVector>::clearPosition();
  }

  // See vara::PhasarTaintAnalysis::taintsForInst
  [[nodiscard]] inline TaintSetT taintsForInst(
      const llvm::Instruction *Inst,
      GenericSolverResults<const llvm::Instruction *, const llvm::Value *,
                           IDEFeatureTaintEdgeFact> SR) {

    if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(Inst)) {
      if (Ret->getNumOperands() == 0) {
        return {};
      }
    } else if (llvm::isa<llvm::UnreachableInst>(Inst)) {
      return {};
    }

    TaintSetT AggregatedTaints;

    if (Inst->getType()->isVoidTy()) {
      // For void types, we need to look what
      // taints flow into the inst

      assert(Inst->getNumOperands() >= 1 &&
             "Found case without first operand.");
      AggregatedTaints =
          SR.resultAt(Inst, Inst->getOperand(0)).toSet<TestingSrcLocation>();

    } else {
      auto Results = SR.resultsAtInLLVMSSA(Inst);
      auto SearchPosTaints = Results.find(Inst);
      if (SearchPosTaints != Results.end()) {
        AggregatedTaints = SearchPosTaints->second.toSet<TestingSrcLocation>();
      }
    }

    // additionalStaticTaints
    auto AdditionalTaints = generateTaintsAtInst(Inst);
    AggregatedTaints.insert(AdditionalTaints.begin(), AdditionalTaints.end());

    return AggregatedTaints;
  }

  void
  printDump(const LLVMProjectIRDB &IRDB,
            GenericSolverResults<const llvm::Instruction *, const llvm::Value *,
                                 IDEFeatureTaintEdgeFact>
                SR) {
    const llvm::Function *CurrFun = nullptr;
    for (const auto *Inst : IRDB.getAllInstructions()) {
      if (CurrFun != Inst->getFunction()) {
        CurrFun = Inst->getFunction();
        llvm::errs() << "\n=================== '" << CurrFun->getName()
                     << "' ===================\n";
      }
      llvm::errs() << "  N: " << llvmIRToString(Inst) << '\n';
      llvm::errs() << "  D: " << printSet(taintsForInst(Inst, SR)) << "\n\n";
    }
  }

}; // Test Fixture

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main9 = LineColFun{4, 3, "main"};
  GroundTruth.emplace(Main9, "i", TaintSetT{LineColFun{2, 7, "main"}});
  GroundTruth.emplace(Main9, "j",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{3, 11, "main"},
                          LineColFun{3, 13, "main"},
                          LineColFun{3, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main24 = LineColFun{10, 3, "main"};

  GroundTruth.emplace(Main24, "argc", TaintSetT{LineColFun{1, 14, "main"}});
  GroundTruth.emplace(Main24, "argv", TaintSetT{LineColFun{1, 27, "main"}});
  GroundTruth.emplace(Main24, "i",
                      TaintSetT{
                          LineColFun{5, 7, "main"},
                          LineColFun{7, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "j",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{3, 11, "main"},
                          LineColFun{3, 13, "main"},
                          LineColFun{3, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "k",
                      TaintSetT{
                          LineColFun{5, 7, "main"},
                          LineColFun{7, 7, "main"},
                          LineColFun{9, 11, "main"},
                          LineColFun{9, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_03) {
  std::set<IIACompactResult_t> GroundTruth;

  auto Main20 = LineColFun{6, 3, "main"};

  GroundTruth.emplace(Main20, "i",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{4, 5, "main"},
                      });
  GroundTruth.emplace(Main20, "x",
                      TaintSetT{
                          LineColFun{3, 12, "main"},
                          LineColFun{3, 28, "main"},
                      });

  doAnalysisAndCompareResults("basic_03_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_04) {

  LIBCPP_GTEST_SKIP;

  std::set<IIACompactResult_t> GroundTruth;
  auto Main23 = LineColFun{11, 3, "main"};

  GroundTruth.emplace(Main23, "argc",
                      TaintSetT{
                          LineColFun{3, 14, "main"},
                      });
  GroundTruth.emplace(Main23, "argv",
                      TaintSetT{
                          LineColFun{3, 27, "main"},
                      });
  GroundTruth.emplace(Main23, "i",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                      });
  GroundTruth.emplace(Main23, "j",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main23, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{6, 7, "main"},
                          LineColFun{8, 9, "main"},
                          LineColFun{8, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_04_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main11 = LineColFun{10, 3, "main"};
  GroundTruth.emplace(Main11, "i",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                          LineColFun{8, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_05_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main19 = LineColFun{14, 3, "main"};

  GroundTruth.emplace(Main19, "i",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                          LineColFun{13, 8, "main"},
                          LineColFun{13, 6, "main"},
                      });
  GroundTruth.emplace(Main19, "j",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                          LineColFun{13, 8, "main"},
                          LineColFun{13, 6, "main"},
                      });
  GroundTruth.emplace(Main19, "k",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                      });
  GroundTruth.emplace(Main19, "p",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{9, 7, "main"},
                          LineColFun{11, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_06_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main15 = LineColFun{5, 3, "main"};

  GroundTruth.emplace(Main15, "argc",
                      TaintSetT{
                          LineColFun{1, 14, "main"},
                      });
  GroundTruth.emplace(Main15, "argv",
                      TaintSetT{
                          LineColFun{1, 27, "main"},
                      });
  // strong update on i
  GroundTruth.emplace(Main15, "i",
                      TaintSetT{
                          LineColFun{4, 5, "main"},
                      });
  GroundTruth.emplace(Main15, "j",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{3, 11, "main"},
                          LineColFun{3, 13, "main"},
                          LineColFun{3, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_07_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_08) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main12 = LineColFun{11, 3, "main"};

  // strong update on i
  GroundTruth.emplace(Main12, "i",
                      TaintSetT{
                          LineColFun{10, 5, "main"},
                      });

  doAnalysisAndCompareResults("basic_08_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_09) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = LineColFun{6, 3, "main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{3, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{3, 7, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{5, 5, "main"},
                      });

  doAnalysisAndCompareResults("basic_09_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_10) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main6 = LineColFun{4, 3, "main"};
  GroundTruth.emplace(Main6, "i",
                      TaintSetT{
                          LineColFun{3, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_10_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleBasicTest_11) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main20 = RetStmt{"main"};

  GroundTruth.emplace(Main20, "FeatureSelector",
                      TaintSetT{
                          LineColFun{3, 14, "main"},
                          LineColFun{4, 25, "main"},
                          LineColFun{4, 7, "main"},
                      });

  GroundTruth.emplace(Main20, RetVal{"main"},
                      TaintSetT{
                          LineColFun{7, 5, "main"},
                          LineColFun{15, 3, "main"},
                          LineColFun{16, 1, "main"},
                      });

  doAnalysisAndCompareResults("basic_11_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleCallTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main14 = RetStmt{"main"};

  GroundTruth.emplace(Main14, "i",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                      });
  GroundTruth.emplace(Main14, "j",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main14, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{6, 7, "main"},
                          LineColFun{6, 11, "main"},
                          LineColFun{6, 14, "main"},
                          LineColFun{1, 12, "_Z2idi"},
                          LineColFun{1, 24, "_Z2idi"},
                      });

  doAnalysisAndCompareResults("call_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleCallTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main13 = RetStmt{"main"};

  GroundTruth.emplace(Main13, "i",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                      });
  GroundTruth.emplace(Main13, "j",
                      TaintSetT{
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main13, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{6, 15, "main"},
                          LineColFun{6, 18, "main"},
                          LineColFun{6, 11, "main"},
                          LineColFun{6, 7, "main"},
                          LineColFun{1, 13, "_Z3sumii"},
                          LineColFun{1, 20, "_Z3sumii"},
                          LineColFun{1, 32, "_Z3sumii"},
                          LineColFun{1, 36, "_Z3sumii"},
                          LineColFun{1, 34, "_Z3sumii"},
                      });

  doAnalysisAndCompareResults("call_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleCallTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = RetStmt{"main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{9, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{9, 7, "main"},
                          LineColFun{10, 21, "main"},
                          LineColFun{6, 1, "_Z9factorialj"},
                          LineColFun{3, 5, "_Z9factorialj"},
                          LineColFun{9, 7, "main"},
                          LineColFun{1, 29, "_Z9factorialj"},
                          LineColFun{5, 3, "_Z9factorialj"},
                          LineColFun{5, 10, "_Z9factorialj"},
                          LineColFun{5, 14, "_Z9factorialj"},
                          LineColFun{5, 24, "_Z9factorialj"},
                          LineColFun{5, 12, "_Z9factorialj"},
                          LineColFun{5, 26, "_Z9factorialj"},
                          LineColFun{10, 11, "main"},
                          LineColFun{10, 7, "main"},
                      });

  doAnalysisAndCompareResults("call_03_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleCallTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = RetStmt{"main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{13, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{6, 1, "_Z9factorialj"},
                          LineColFun{3, 5, "_Z9factorialj"},
                          LineColFun{1, 29, "_Z9factorialj"},
                          LineColFun{5, 3, "_Z9factorialj"},
                          LineColFun{5, 10, "_Z9factorialj"},
                          LineColFun{5, 14, "_Z9factorialj"},
                          LineColFun{5, 24, "_Z9factorialj"},
                          LineColFun{5, 12, "_Z9factorialj"},
                          LineColFun{5, 26, "_Z9factorialj"},
                          LineColFun{14, 21, "main"},
                          LineColFun{13, 7, "main"},
                          LineColFun{14, 7, "main"},
                          LineColFun{14, 11, "main"},
                      });
  GroundTruth.emplace(Main10, "k",
                      TaintSetT{
                          LineColFun{16, 12, "main"},
                          LineColFun{8, 24, "_Z2idi"},
                          LineColFun{6, 1, "_Z9factorialj"},
                          LineColFun{3, 5, "_Z9factorialj"},
                          LineColFun{16, 5, "main"},
                          LineColFun{1, 29, "_Z9factorialj"},
                          LineColFun{5, 3, "_Z9factorialj"},
                          LineColFun{5, 10, "_Z9factorialj"},
                          LineColFun{5, 14, "_Z9factorialj"},
                          LineColFun{16, 5, "main"},
                          LineColFun{8, 12, "_Z2idi"},
                          LineColFun{5, 24, "_Z9factorialj"},
                          LineColFun{5, 12, "_Z9factorialj"},
                          LineColFun{5, 26, "_Z9factorialj"},
                          LineColFun{16, 12, "main"},
                          LineColFun{10, 20, "_Z3sumii"},
                          LineColFun{10, 32, "_Z3sumii"},
                          LineColFun{14, 11, "main"},
                          LineColFun{14, 21, "main"},
                          LineColFun{10, 34, "_Z3sumii"},
                          LineColFun{10, 36, "_Z3sumii"},
                          LineColFun{10, 13, "_Z3sumii"},
                          LineColFun{15, 11, "main"},
                          LineColFun{15, 14, "main"},
                          LineColFun{13, 7, "main"},
                          LineColFun{14, 7, "main"},
                          LineColFun{16, 8, "main"},
                          LineColFun{16, 15, "main"},
                          LineColFun{15, 7, "main"},
                      });

  doAnalysisAndCompareResults("call_04_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleCallTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = RetStmt{"main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{2, 38, "_Z18setValueToFortyTwoPi"},
                          LineColFun{7, 3, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{2, 38, "_Z18setValueToFortyTwoPi"},
                          LineColFun{6, 7, "main"},
                          LineColFun{8, 3, "main"},
                      });

  doAnalysisAndCompareResults("call_05_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleCallTest_06) {
  // NOTE: Here we are suffering from IntraProceduralAliasesOnly
  std::set<IIACompactResult_t> GroundTruth;
  auto Main24 = RetStmt{"main"};

  GroundTruth.emplace(Main24, "i",
                      TaintSetT{
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{9, 17, "main"},
                          LineColFun{9, 5, "main"},
                          LineColFun{9, 7, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "j",
                      TaintSetT{
                          LineColFun{10, 7, "main"},
                          LineColFun{10, 17, "main"},
                          LineColFun{10, 5, "main"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{6, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "k",
                      TaintSetT{
                          LineColFun{11, 17, "main"},
                          LineColFun{11, 7, "main"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{7, 7, "main"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{11, 5, "main"},
                      });
  GroundTruth.emplace(Main24, "l",
                      TaintSetT{
                          LineColFun{8, 7, "main"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{12, 7, "main"},
                          LineColFun{12, 17, "main"},
                          LineColFun{12, 5, "main"},
                      });

  doAnalysisAndCompareResults("call_06_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleCallTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main6 = RetStmt{"main"};

  GroundTruth.emplace(Main6, "VarIR",
                      TaintSetT{
                          LineColFun{7, 7, "main"},
                          LineColFun{3, 6, "_Z13inputRefParamRi"},
                          LineColFun{8, 3, "main"},
                      });
  doAnalysisAndCompareResults("call_07_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleGlobalTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main9 = RetStmt{"main"};

  GroundTruth.emplace(Main9, "i",
                      TaintSetT{
                          LineColFun{6, 5, "main"},
                      });
  GroundTruth.emplace(Main9, "j",
                      TaintSetT{
                          GlobalVar{"i"},
                          LineColFun{5, 7, "main"},
                          LineColFun{5, 5, "main"},
                      });

  doAnalysisAndCompareResults("global_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleGlobalTest_02) {
  std::set<IIACompactResult_t> GroundTruth;

  auto Main12 = RetStmt{"main"};
  auto Init2 = RetStmt{"_Z5initBv"};

  GroundTruth.emplace(Init2, "a",
                      TaintSetT{
                          GlobalVar{"a"},
                      });
  GroundTruth.emplace(Init2, "b",
                      TaintSetT{
                          LineColFun{4, 18, "_Z5initBv"},
                      });

  GroundTruth.emplace(Main12, "a",
                      TaintSetT{
                          GlobalVar{"a"},
                      });
  GroundTruth.emplace(Main12, "b",
                      TaintSetT{
                          LineColFun{4, 18, "_Z5initBv"},
                      });
  GroundTruth.emplace(Main12, "c",
                      TaintSetT{
                          GlobalVar{"b"},
                          LineColFun{7, 7, "main"},
                          LineColFun{7, 11, "main"},
                      });

  doAnalysisAndCompareResults("global_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleGlobalTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(LineColFun{6, 11, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{6, 25, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(RetStmt{"main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });

  doAnalysisAndCompareResults("global_03_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleGlobalTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(LineColFun{8, 11, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{8, 25, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(RetStmt{"main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{3, 31, "_Z7doStuffi"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{3, 22, "_Z7doStuffi"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });

  doAnalysisAndCompareResults("global_04_cpp_dbg.ll", {"main", "_Z7doStuffi"},
                              GroundTruth, false);
}

TEST_F(IDEFeatureTaintAnalysisTest, KillTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main12 = RetStmt{"main"};

  GroundTruth.emplace(Main12, "i",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                      });
  GroundTruth.emplace(Main12, "j",
                      TaintSetT{
                          LineColFun{5, 5, "main"},
                      });
  GroundTruth.emplace(Main12, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{4, 11, "main"},
                          LineColFun{2, 7, "main"},
                      });

  doAnalysisAndCompareResults("KillTest_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, KillTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main12 = RetStmt{"main"};

  GroundTruth.emplace(Main12, "A",
                      TaintSetT{
                          GlobalVar{"A"},
                      });
  GroundTruth.emplace(Main12, "B",
                      TaintSetT{
                          LineColFun{4, 18, "_Z5initBv"},
                      });
  GroundTruth.emplace(Main12, "C",
                      TaintSetT{
                          GlobalVar{"B"},
                          LineColFun{7, 11, "main"},
                          LineColFun{7, 7, "main"},
                      });

  doAnalysisAndCompareResults("KillTest_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleReturnTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main6 = LineColFun{7, 12, "main"};
  auto Main8 = RetStmt{"main"};

  GroundTruth.emplace(Main6, "localVar",
                      TaintSetT{
                          LineColFun{6, 12, "main"},
                      });
  GroundTruth.emplace(Main8, "localVar",
                      TaintSetT{
                          LineColFun{2, 30, "_Z20returnIntegerLiteralv"},
                          LineColFun{7, 14, "main"},
                          LineColFun{7, 12, "main"},
                      });

  doAnalysisAndCompareResults("return_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleHeapTest_01) {
  std::set<IIACompactResult_t> GroundTruth;

  auto Main17 = RetStmt{"main"};
  GroundTruth.emplace(Main17, "i",
                      TaintSetT{
                          LineColFun{3, 12, "main"},
                          LineColFun{3, 8, "main"},
                      });
  GroundTruth.emplace(Main17, "j",
                      TaintSetT{
                          LineColFun{3, 12, "main"},
                          LineColFun{3, 8, "main"},
                          LineColFun{4, 12, "main"},
                          LineColFun{4, 11, "main"},
                          LineColFun{4, 7, "main"},
                      });

  doAnalysisAndCompareResults("heap_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

// PHASAR_SKIP_TEST(TEST_F(IDEFeatureTaintAnalysisTest, HandleRVOTest_02) {
//   GTEST_SKIP() << "This test heavily depends on the used stdlib version.
//   TODO: "
//                   "add a better one";

//   std::set<IIACompactResult_t> GroundTruth;
//   GroundTruth.emplace("main", 18, "retval", std::set<std::string>{"75",
//   "76"}); GroundTruth.emplace("main", 18, "str",
//                       std::set<std::string>{"70", "65", "72", "74", "77"});
//   GroundTruth.emplace("main", 18, "ref.tmp",
//                       std::set<std::string>{"66", "9", "72", "73", "71"});
//   doAnalysisAndCompareResults("rvo_02_cpp.ll", {"main"}, GroundTruth, true);
// })

TEST_F(IDEFeatureTaintAnalysisTest, HandleRVOTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main19 = RetStmt{"main"};

  GroundTruth.emplace(Main19, "Str",
                      TaintSetT{
                          LineColFun{49, 10, "main"},
                          LineColFun{49, 10, "main"},
                          LineColFun{51, 7, "main"},
                          LineColFun{52, 7, "main"},
                          LineColFun{53, 14, "main"},
                          LineColFun{54, 1, "main"},
                          LineColFun{41, 10, "_ZN6StringC2Ev"},
                      });

  doAnalysisAndCompareResults("rvo_03_cpp_dbg.ll", {"main"}, GroundTruth, true);
}

TEST_F(IDEFeatureTaintAnalysisTest, HandleRVOTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = RetStmt{"main"};

  GroundTruth.emplace(Main10, "F",
                      TaintSetT{
                          LineColFun{17, 7, "main"},
                          LineColFun{17, 7, "main"},
                          LineColFun{18, 7, "main"},
                          LineColFun{18, 5, "main"},
                          LineColFun{14, 26, "_Z9createFoov"},
                          LineColFun{14, 26, "_Z9createFoov"},
                          LineColFun{7, 7, "_ZN3FooC2Ev"},
                      });

  doAnalysisAndCompareResults("rvo_04_cpp_dbg.ll", {"main"}, GroundTruth, true);
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
