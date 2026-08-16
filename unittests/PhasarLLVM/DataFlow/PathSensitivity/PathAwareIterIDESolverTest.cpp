#include "phasar/DataFlow/IfdsIde/Solver/PathAwareIterIDESolver.h"

#include "phasar/DataFlow/IfdsIde/Solver/PathAwareIDESolver.h"
#include "phasar/DataFlow/PathSensitivity/FlowPath.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/LLVMPathConstraints.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/Z3BasedPathSensitivityConfig.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/Z3BasedPathSensitvityManager.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "llvm/ADT/StringRef.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

namespace {
struct PathAwareIterIDESolverTest
    : public ::testing::TestWithParam<std::string_view> {

  static constexpr auto PathToLlFiles = PHASAR_BUILD_SUBFOLDER("path_tracing/");
};

using n_t = const llvm::Instruction *;

static void comparePaths(const psr::FlowPathSequence<n_t> &Old,
                         const psr::FlowPathSequence<n_t> &New) {
  std::set<size_t> MatchingIndices;
  auto Matches = [&New, &MatchingIndices](const psr::FlowPath<n_t> &GT) {
    size_t Idx = 0;
    for (const auto &Path : New) {
      psr::scope_exit IncIdx = [&Idx] { ++Idx; };
      if (Path.size() != GT.size()) {
        continue;
      }
      bool Match = llvm::equal(Path, GT);

      if (Match) {
        MatchingIndices.insert(Idx);
        return true;
      }
    }

    return false;
  };

  for (const auto &GT : Old) {
    EXPECT_TRUE(Matches(GT))
        << "No match found for " << psr::PrettyPrinter{GT}
        << "; MatchingIndices.size() = " << MatchingIndices.size()
        << "; NewPaths.size() = " << New.size();
  }

  EXPECT_EQ(MatchingIndices.size(), New.size());

  if (MatchingIndices.size() != New.size()) {
    for (size_t I = 0; I < New.size(); ++I) {
      if (MatchingIndices.contains(I)) {
        continue;
      }

      llvm::errs() << "> PATH NOT IN GT: "
                   << psr::PrettyPrinter{llvm::map_range(
                          New[I],
                          [](const auto *Inst) {
                            return psr::getMetaDataID(Inst);
                          })}
                   << '\n';
    }
  }
}

TEST_P(PathAwareIterIDESolverTest, RegressionAgainstIDESolver) {
  auto IRDB = psr::LLVMProjectIRDB::loadOrExit(PathToLlFiles + GetParam());

  auto *Main = IRDB.getFunctionDefinition("main");
  ASSERT_NE(Main, nullptr);
  auto *LastInst = &Main->back().back();
  llvm::outs() << "Target instruction: " << psr::llvmIRToString(LastInst)
               << '\n';

  psr::DIBasedTypeHierarchy TH(IRDB);
  psr::LLVMAliasSet PT(&IRDB);
  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"}, &TH,
                          &PT, psr::Soundness::Soundy,
                          /*IncludeGlobals*/ false);

  psr::LLVMTaintConfig Config(IRDB);
  psr::IDEExtendedTaintAnalysis<3, false> Analysis(&IRDB, &ICFG, &PT, Config,
                                                   {"main"});
  psr::PathAwareIDESolver OldSolver(&Analysis, &ICFG);
  OldSolver.solve();

  psr::PathAwareIterIDESolver NewSolver(&Analysis, &ICFG);
  NewSolver.solve();

  const auto &OldESG = OldSolver.getExplicitESG();
  const auto &NewESG = NewSolver.getExplicitESG();

  psr::LLVMPathConstraints LPC;

  psr::Z3BasedPathSensitivityManager<psr::IDEExtendedTaintAnalysisDomain>
      OldPSM(&OldESG, psr::Z3BasedPathSensitivityConfig(), &LPC);
  psr::Z3BasedPathSensitivityManager<psr::IDEExtendedTaintAnalysisDomain>
      NewPSM(&NewESG, psr::Z3BasedPathSensitivityConfig(), &LPC);

  auto OldPaths = OldPSM.pathsTo(LastInst, Analysis.getZeroValue());
  auto NewPaths = NewPSM.pathsTo(LastInst, Analysis.getZeroValue());

  comparePaths(OldPaths, NewPaths);
}

constexpr std::string_view IRFiles[] = {
    "inter_01_cpp.ll", "inter_02_cpp.ll", "inter_03_cpp.ll", "inter_04_cpp.ll",
    "inter_05_cpp.ll", "inter_06_cpp.ll", "inter_07_cpp.ll", "inter_08_cpp.ll",
    "inter_09_cpp.ll", "inter_10_cpp.ll", "inter_11_cpp.ll", "inter_12_cpp.ll",
};

INSTANTIATE_TEST_SUITE_P(PathSensitivity, PathAwareIterIDESolverTest,
                         ::testing::ValuesIn(IRFiles));

} // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
