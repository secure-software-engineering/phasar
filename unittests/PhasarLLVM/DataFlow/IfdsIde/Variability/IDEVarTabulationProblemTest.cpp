/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/IDEVarTabulationProblem.h"

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/StringRef.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVarTabulationProblemTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER(
      "variability/linear_constant/manually_transformed/");
  const std::vector<std::string> EntryPoints = {"main"};

  using ResultSetTy = std::set<std::pair<std::string, LatticeDomain<int64_t>>>;
  // Function - Line Nr - Variable - Z3Constraint x Value
  using LCAVarCompactResults_t =
      std::tuple<std::string, std::size_t, std::string, ResultSetTy>;

  // IDELinearConstantAnalysis::lca_restults_t
  void
  doAnalysisAndCompareResults(const std::string &IRFilePath,
                              std::set<LCAVarCompactResults_t> &GroundTruth,
                              bool PrintDump = false) {
    LLVMProjectIRDB IRDB(PathToLLFiles + IRFilePath);
    if (PrintDump) {
      IRDB.emitPreprocessedIR(llvm::outs());
    }

    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, EntryPoints);
    IDELinearConstantAnalysis LCAProblem(&IRDB, &ICFG, EntryPoints);
    IDEVarTabulationProblem VARAProblem(LCAProblem, ICFG);
    IDESolver LCASolver(&VARAProblem, &ICFG);

    LCASolver.solve();
    if (PrintDump) {
      LCASolver.dumpResults();
    }

    for (const auto &[TruthFnName, TruthInstId, TruthVarName, TruthResultSet] :
         GroundTruth) {
      const auto *Fun = IRDB.getFunctionDefinition(TruthFnName);
      const auto *Inst = getNthInstruction(Fun, TruthInstId);
      auto Results = LCASolver.resultsAt(Inst);
      for (auto &[Fact, Value] : Results) {
        if (llvm::StringRef(llvmIRToString(Fact)).startswith(TruthVarName)) {
          for (auto &[Constraint, IntegerValue] : Value) {
            bool Found = false;
            for (const auto &[TrueConstaint, TrueIntegerValue] :
                 TruthResultSet) {

              if (Constraint.to_string() == TrueConstaint) {
                EXPECT_EQ(IntegerValue, TrueIntegerValue);
                Found = true;
                break;
              }
            }
            if (!Found) {
              FAIL() << "Could not find constraint: '" << Constraint.to_string()
                     << "' in ground truth!";
            }
          }
        }
      }
    }
  }

}; // Test Fixture

// TEST_F(IDEVarTabulationProblemTest,
// HandleBasic_TwoVariablesDesugared) {
//   auto Results = doAnalysis("twovariables_desugared_c.ll", true);
//   // std::set<LCAVarCompactResults_t
// > GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

TEST_F(IDEVarTabulationProblemTest, HandleBasic_01) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 11, "x",
                      ResultSetTy{{"A_defined", 42}, {"(not A_defined)", 13}});
  GroundTruth.emplace("main", 11, "retval", ResultSetTy{{"true", 0}});
  doAnalysisAndCompareResults("basic_01_c.ll", GroundTruth, true);
}

TEST_F(IDEVarTabulationProblemTest, HandleBasic_02) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 6, "x", ResultSetTy{{"true", 150}});
  GroundTruth.emplace("main", 6, "%0", ResultSetTy{{"true", 150}});
  GroundTruth.emplace("main", 6, "retval", ResultSetTy{{"true", 0}});
  doAnalysisAndCompareResults("basic_02_c.ll", GroundTruth, false);
}

TEST_F(IDEVarTabulationProblemTest, HandleBasic_03) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 14, "x",
                      ResultSetTy{{"A_defined", 42}, {"(not A_defined)", 13}});
  GroundTruth.emplace("main", 14, "y", ResultSetTy{{"true", 210}});
  GroundTruth.emplace("main", 14, "retval", ResultSetTy{{"true", 0}});
  doAnalysisAndCompareResults("basic_03_c.ll", GroundTruth, false);
}

TEST_F(IDEVarTabulationProblemTest, HandleBasic_04) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 9, "x", ResultSetTy{{"true", 301}});
  GroundTruth.emplace("main", 9, "retval", ResultSetTy{{"true", 0}});
  doAnalysisAndCompareResults("basic_03_c.ll", GroundTruth, false);
}

TEST_F(IDEVarTabulationProblemTest, HandleLoops_01) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 24, "x",
                      ResultSetTy{{"true", Top{}},
                                  {"A_defined", Bottom{}},
                                  {"(not A_defined)", 0}});
  GroundTruth.emplace("main", 24, "i",
                      ResultSetTy{{"true", Top{}},
                                  {"A_defined", Bottom{}},
                                  {"(not A_defined)", Bottom{}}});
  GroundTruth.emplace(
      "main", 24, "retval",
      ResultSetTy{{"true", Top{}}, {"A_defined", 0}, {"(not A_defined)", 0}});
  doAnalysisAndCompareResults("loop_01_c.ll", GroundTruth, false);
}

TEST_F(IDEVarTabulationProblemTest, HandleCalls_01) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 13, "x",
                      ResultSetTy{{"true", Top{}},
                                  {"A_defined", 100},
                                  {"(not A_defined)", 99}});
  GroundTruth.emplace(
      "main", 13, "retval",
      ResultSetTy{{"true", Top{}}, {"A_defined", 0}, {"(not A_defined)", 0}});
  doAnalysisAndCompareResults("call_01_c.ll", GroundTruth, false);
}

TEST_F(IDEVarTabulationProblemTest, HandleRecursion_01) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 13, "x",
                      ResultSetTy{{"true", Top{}},
                                  {"A_defined", Bottom{}},
                                  {"(not A_defined)", 5}});
  GroundTruth.emplace(
      "main", 13, "retval",
      ResultSetTy{{"true", Top{}}, {"A_defined", 0}, {"(not A_defined)", 0}});
  doAnalysisAndCompareResults("recursion_01_c.ll", GroundTruth, false);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
