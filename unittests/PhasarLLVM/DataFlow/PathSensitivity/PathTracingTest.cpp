#include "phasar/DataFlow/IfdsIde/Solver/PathAwareIDESolver.h"
#include "phasar/DataFlow/PathSensitivity/FlowPath.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityManager.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/LLVMPathConstraints.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/Z3BasedPathSensitivityConfig.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/Z3BasedPathSensitvityManager.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/AdjacencyList.h"
#include "phasar/Utils/DFAMinimizer.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <cassert>
#include <fstream>
#include <memory>
#include <new>
#include <string>
#include <system_error>

namespace {
// ============== TEST FIXTURE ============== //
class PathTracingTest : public ::testing::Test {
public:
  static constexpr auto PathToLlFiles = PHASAR_BUILD_SUBFOLDER("path_tracing/");

  static std::pair<const llvm::Instruction *, const llvm::Value *>
  getInterestingInstFact(psr::LLVMProjectIRDB &IRDB) {
    auto *Main = IRDB.getFunctionDefinition("main");
    assert(Main);
    auto *LastInst = &Main->back().back();
    auto *InterestingFact = [&] {
      if (auto *RetInst = llvm::dyn_cast<llvm::ReturnInst>(LastInst);
          RetInst && RetInst->getReturnValue() &&
          !llvm::isa<llvm::Constant>(RetInst->getReturnValue())) {
        return RetInst->getReturnValue();
      }

      auto *Inst = LastInst->getPrevNode();
      while (Inst && !Inst->getType()->isIntegerTy()) {
        Inst = Inst->getPrevNode();
      }
      assert(Inst != nullptr);
      return static_cast<llvm::Value *>(Inst);
    }();

    return {LastInst, InterestingFact};
  }

protected:
  std::unique_ptr<psr::LLVMProjectIRDB> IRDB;
  psr::LLVMPathConstraints LPC;

  void SetUp() override { psr::ValueAnnotationPass::resetValueID(); }
  void TearDown() override { psr::Logger::disable(); }

  std::pair<const llvm::Instruction *, const llvm::Value *>
  getInterestingInstFact() {
    return getInterestingInstFact(*IRDB);
  }

  psr::FlowPathSequence<const llvm::Instruction *>
  doAnalysis(const std::string &LlvmFilePath, bool PrintDump = false) {
    IRDB = std::make_unique<psr::LLVMProjectIRDB>(PathToLlFiles + LlvmFilePath);
    psr::DIBasedTypeHierarchy TH(*IRDB);
    psr::LLVMAliasSet PT(IRDB.get());
    psr::LLVMBasedICFG ICFG(IRDB.get(), psr::CallGraphAnalysisType::OTF,
                            {"main"}, &TH, &PT, psr::Soundness::Soundy,
                            /*IncludeGlobals*/ false);
    psr::IDELinearConstantAnalysis LCAProblem(IRDB.get(), &ICFG, {"main"});
    psr::PathAwareIDESolver LCASolver(LCAProblem, &ICFG);
    LCASolver.solve();
    if (PrintDump) {
      // IRDB->print();
      // ICFG.print();
      // LCASolver.dumpResults();
      std::error_code EC;
      llvm::raw_fd_ostream ROS(LlvmFilePath + "_explicit_esg.dot", EC);
      assert(!EC);
      LCASolver.getExplicitESG().printAsDot(ROS);
    }
    auto [LastInst, InterestingFact] = getInterestingInstFact();
    llvm::outs() << "Target instruction: " << psr::llvmIRToString(LastInst);
    llvm::outs() << "\nTarget data-flow fact: "
                 << psr::llvmIRToString(InterestingFact) << '\n';

    psr::Z3BasedPathSensitivityManager<psr::IDELinearConstantAnalysisDomain>
        PSM(&LCASolver.getExplicitESG(), {}, &LPC);

    return PSM.pathsTo(LastInst, InterestingFact);
  }

  psr::FlowPathSequence<const llvm::Instruction *>
  doLambdaAnalysis(const std::string &LlvmFilePath,
                   size_t MaxDAGDepth = SIZE_MAX) {
    IRDB = std::make_unique<psr::LLVMProjectIRDB>(PathToLlFiles + LlvmFilePath);
    psr::DIBasedTypeHierarchy TH(*IRDB);
    psr::LLVMAliasSet PT(IRDB.get());
    psr::LLVMBasedICFG ICFG(IRDB.get(), psr::CallGraphAnalysisType::OTF,
                            {"main"}, &TH, &PT, psr::Soundness::Soundy,
                            /*IncludeGlobals*/ false);

    psr::LLVMTaintConfig Config(*IRDB);
    psr::IDEExtendedTaintAnalysis<3, false> Analysis(IRDB.get(), &ICFG, &PT,
                                                     Config, {"main"});
    psr::PathAwareIDESolver Solver(Analysis, &ICFG);
    Solver.solve();

    auto *Main = IRDB->getFunctionDefinition("main");
    assert(Main);
    auto *LastInst = &Main->back().back();
    llvm::outs() << "Target instruction: " << psr::llvmIRToString(LastInst)
                 << '\n';

    // std::error_code EC;
    // llvm::raw_fd_ostream ROS(LlvmFilePath + "_explicit_esg.dot", EC);
    // assert(!EC);
    // Solver.getExplicitESG().printAsDot(ROS);

    psr::Z3BasedPathSensitivityManager<psr::IDEExtendedTaintAnalysisDomain> PSM(
        &Solver.getExplicitESG(),
        psr::Z3BasedPathSensitivityConfig().withDAGDepthThreshold(MaxDAGDepth),
        &LPC);

    return PSM.pathsTo(LastInst, Analysis.getZeroValue());
  }

  void comparePaths(
      const psr::FlowPathSequence<const llvm::Instruction *> &AnalyzedPaths,
      const std::vector<std::vector<unsigned>> &GroundTruth) {
    std::set<size_t> MatchingIndices;
    auto Matches = [&AnalyzedPaths,
                    &MatchingIndices](const std::vector<unsigned> &GT) {
      size_t Idx = 0;
      for (const auto &Path : AnalyzedPaths) {
        psr::scope_exit IncIdx = [&Idx] { ++Idx; };
        if (Path.size() != GT.size()) {
          continue;
        }
        bool Match = true;
        for (size_t I = 0; I < Path.size(); ++I) {
          if (std::stoul(psr::getMetaDataID(Path[I])) != GT[I]) {
            Match = false;
            break;
          }
        }
        if (Match) {
          MatchingIndices.insert(Idx);
          return true;
        }
      }

      return false;
    };

    for (const auto &GT : GroundTruth) {
      EXPECT_TRUE(Matches(GT))
          << "No match found for " << psr::PrettyPrinter{GT}
          << "; MatchingIndices.size() = " << MatchingIndices.size()
          << "; AnalyzedPaths.size() = " << AnalyzedPaths.size();
    }

    EXPECT_EQ(MatchingIndices.size(), AnalyzedPaths.size());

    if (MatchingIndices.size() != AnalyzedPaths.size()) {
      for (size_t I = 0; I < AnalyzedPaths.size(); ++I) {
        if (MatchingIndices.count(I)) {
          continue;
        }

        llvm::errs() << "> PATH NOT IN GT: "
                     << psr::PrettyPrinter{llvm::map_range(
                            AnalyzedPaths[I],
                            [](const auto *Inst) {
                              return psr::getMetaDataID(Inst);
                            })}
                     << '\n';
      }
    }
  }
}; // Test Fixture

TEST_F(PathTracingTest, Handle_Inter_01) {
  auto PathsVec = doAnalysis("inter_01_cpp.ll");
  comparePaths(PathsVec, {{6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 5,
                           16, 17, 18}});
}

TEST_F(PathTracingTest, Lambda_Inter_01) {
  auto PathsVec = doLambdaAnalysis("inter_01_cpp.ll");
  comparePaths(PathsVec, {{6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5,
                           16, 17, 18}});
}

TEST_F(PathTracingTest, Handle_Inter_02) {
  auto PathsVec = doAnalysis("inter_02_cpp.ll");
  comparePaths(PathsVec,
               {{12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 6,  7,  8, 9,
                 10, 11, 22, 23, 24, 0,  1,  2,  3,  5,  25, 26, 27}});
}

TEST_F(PathTracingTest, Lambda_Inter_02) {
  auto PathsVec = doLambdaAnalysis("inter_02_cpp.ll");
  comparePaths(PathsVec,
               {{12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 6, 7,  8,  9,
                 10, 11, 22, 23, 24, 0,  1,  2,  3,  4,  5, 25, 26, 27}});
}

TEST_F(PathTracingTest, Handle_Inter_03) {
  //   psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                       "PathSensitivityManager");
  //   psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                       "CallStackPathFilter");
  auto PathsVec = doAnalysis("inter_03_cpp.ll", false);
  comparePaths(PathsVec, {{6, 7, 8,  9,  10, 11, 12, 13, 14, 15, 16, 0,  1, 2,
                           3, 5, 17, 18, 19, 0,  1,  2,  3,  5,  20, 21, 22}});
}

TEST_F(PathTracingTest, Lambda_Inter_03) {
  auto PathsVec = doLambdaAnalysis("inter_03_cpp.ll");
  comparePaths(PathsVec,
               {{6, 7, 8,  9,  10, 11, 12, 13, 14, 15, 16, 0,  1,  2, 3,
                 4, 5, 17, 18, 19, 0,  1,  2,  3,  4,  5,  20, 21, 22}});
}

TEST_F(PathTracingTest, Handle_Inter_04) {
  auto PathsVec = doAnalysis("inter_04_cpp.ll");
  comparePaths(PathsVec, {{22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 0,
                           1,  2,  3,  5,  33, 34, 35, 6,  8,  11, 16, 17,
                           0,  1,  2,  3,  5,  18, 19, 20, 21, 36, 37, 38}});
}

TEST_F(PathTracingTest, Lambda_Inter_04) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  auto PathsVec = doLambdaAnalysis("inter_04_cpp.ll");
  comparePaths(PathsVec,
               {
                   {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 0,  1,  2,
                    3,  4,  5,  33, 34, 35, 6,  7,  8,  9,  10, 11, 16, 17,
                    0,  1,  2,  3,  4,  5,  18, 19, 20, 21, 36, 37, 38},
                   {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 0,
                    1,  2,  3,  4,  5,  33, 34, 35, 6,  7,  8,  9,
                    10, 11, 12, 13, 14, 15, 20, 21, 36, 37, 38},
               });
}

TEST_F(PathTracingTest, Handle_Inter_05) {
  /// NOTE: We are generating from zero a few times, so without AutoSkipZero we
  /// get a lot of paths here
  auto PathsVec = doAnalysis("inter_05_cpp.ll", false);
  comparePaths(
      PathsVec,
      {{22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        15, 16, 17, 18, 19, 21, 39, 40, 41, 44, 45, 46, 47, 0,  1,  2,  3,
        4,  5,  6,  7,  11, 12, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
       {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 15,
        16, 17, 18, 19, 21, 39, 40, 41, 42, 43, 44, 45, 46, 47, 0,  1,  2,  3,
        4,  5,  6,  7,  11, 12, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
       {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 15,
        16, 17, 18, 19, 20, 21, 39, 40, 41, 44, 45, 46, 47, 0,  1,  2,  3,  4,
        5,  6,  7,  8,  9,  10, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
       {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 15, 16, 17, 18, 19, 20, 21, 39, 40, 41, 42,
        43, 44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7,  8,
        9,  10, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
       {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 41, 44, 52, 55, 56},
       {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
        38, 15, 16, 17, 18, 19, 21, 39, 40, 41, 42, 43, 44, 52, 55, 56},
       {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        15, 16, 17, 18, 19, 20, 21, 39, 40, 41, 42, 43, 44, 52, 55, 56}});
}

TEST_F(PathTracingTest, Lambda_Inter_05) {
  /// We have 4 branches ==> 16 paths
  auto PathsVec = doLambdaAnalysis("inter_05_cpp.ll");
  comparePaths(
      PathsVec,
      {
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
           15, 16, 17, 18, 19, 21, 39, 40, 41, 44, 45, 46, 47, 0,  1,  2,  3,
           4,  5,  6,  7,  11, 12, 13, 14, 48, 49, 50, 51, 52, 55, 56},
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
           15, 16, 17, 18, 19, 21, 39, 40, 41, 42, 43, 44, 45, 46, 47, 0,  1,
           2,  3,  4,  5,  6,  7,  11, 12, 13, 14, 48, 49, 50, 51, 52, 55, 56},
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
           15, 16, 17, 18, 19, 21, 39, 40, 41, 44, 45, 46, 47, 0,  1,  2,  3,
           4,  5,  6,  7,  11, 12, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
           15, 16, 17, 18, 19, 20, 21, 39, 40, 41, 44, 45, 46, 47, 0,  1,  2,
           3,  4,  5,  6,  7,  8,  9,  10, 13, 14, 48, 49, 50, 51, 52, 55, 56},
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
           36, 37, 38, 15, 16, 17, 18, 19, 21, 39, 40, 41, 42, 43,
           44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7,  11, 12,
           13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
           36, 37, 38, 15, 16, 17, 18, 19, 20, 21, 39, 40, 41, 42,
           43, 44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7,  8,
           9,  10, 13, 14, 48, 49, 50, 51, 52, 55, 56},
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
           36, 37, 38, 15, 16, 17, 18, 19, 20, 21, 39, 40, 41, 44,
           45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
           13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
          {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
           36, 37, 38, 15, 16, 17, 18, 19, 20, 21, 39, 40, 41, 42,
           43, 44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7,  8,
           9,  10, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
      });
}

TEST_F(PathTracingTest, Lambda_Inter_Depth3_05) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  /// We have 4 branches ==> 16 paths
  auto PathsVec = doLambdaAnalysis("inter_05_cpp.ll", /*MaxDAGDepth*/ 3);
  comparePaths(PathsVec,
               {
                   {44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7,
                    8,  9,  10, 13, 14, 48, 49, 50, 51, 52, 55, 56},
                   {44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7,  8,
                    9,  10, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
                   {44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6, 7,
                    11, 12, 13, 14, 48, 49, 50, 51, 52, 55, 56},
                   {44, 45, 46, 47, 0,  1,  2,  3,  4,  5,  6,  7, 11,
                    12, 13, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56},
               });
}

TEST_F(PathTracingTest, Handle_Inter_06) {
  auto PathsVec = doAnalysis("inter_06_cpp.ll");
  comparePaths(PathsVec, {{8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                           20, 21, 25, 27, 0,  2,  4,  6,  7,  28, 29, 30},
                          {8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                           22, 26, 27, 0,  3,  5,  6,  7,  28, 29, 30}});
}

TEST_F(PathTracingTest, Lambda_Inter_06) {
  auto PathsVec = doLambdaAnalysis("inter_06_cpp.ll");
  comparePaths(PathsVec,
               {{8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                 24, 25, 26, 27, 0,  1,  2,  3,  4,  5,  6,  7,  28, 29, 30}});
}

TEST_F(PathTracingTest, Handle_Inter_07) {
  auto PathsVec = doAnalysis("inter_07_cpp.ll");
  comparePaths(PathsVec,
               {
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                    30, 34, 42, 44, 46, 8,  10, 12, 14, 15, 47, 48, 49, 50},
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 31,
                    34, 42, 45, 46, 8,  11, 13, 14, 15, 47, 48, 49, 50},
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                    30, 34, 42, 44, 46, 0,  2,  4,  6,  7,  47, 48, 49, 50},
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 31,
                    34, 42, 45, 46, 0,  3,  5,  6,  7,  47, 48, 49, 50},
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                    30, 34, 35, 37, 39, 8,  10, 12, 14, 15, 40, 41, 49, 50},
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 31,
                    34, 35, 38, 39, 8,  11, 13, 14, 15, 40, 41, 49, 50},
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                    30, 34, 35, 37, 39, 0,  2,  4,  6,  7,  40, 41, 49, 50},
                   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 31,
                    34, 35, 38, 39, 0,  3,  5,  6,  7,  40, 41, 49, 50},
               });
}

TEST_F(PathTracingTest, Lambda_Inter_07) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  auto PathsVec = doLambdaAnalysis("inter_07_cpp.ll");
  comparePaths(PathsVec, {
                             {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                              28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                              0,  1,  2,  3,  4,  5,  6,  7,  40, 41, 49, 50},
                             {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                              28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                              8,  9,  10, 11, 12, 13, 14, 15, 40, 41, 49, 50},
                             {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                              28, 29, 30, 31, 32, 33, 34, 42, 43, 44, 45, 46,
                              0,  1,  2,  3,  4,  5,  6,  7,  47, 48, 49, 50},
                             {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                              28, 29, 30, 31, 32, 33, 34, 42, 43, 44, 45, 46,
                              8,  9,  10, 11, 12, 13, 14, 15, 47, 48, 49, 50},
                         });
}

TEST_F(PathTracingTest, Lambda_Inter_Depth3_07) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  auto PathsVec = doLambdaAnalysis("inter_07_cpp.ll", /*MaxDAGDepth*/ 3);
  comparePaths(PathsVec, {
                             {0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 49, 50},
                             {0, 1, 2, 3, 4, 5, 6, 7, 47, 48, 49, 50},
                             {8, 9, 10, 11, 12, 13, 14, 15, 40, 41, 49, 50},
                             {8, 9, 10, 11, 12, 13, 14, 15, 47, 48, 49, 50},
                         });
}

TEST_F(PathTracingTest, Handle_Inter_08) {
  auto PathsVec = doAnalysis("inter_08_cpp.ll", false);
  /// FIXME: Handle mutable z3::exprs; As of now, we reject all paths that go
  /// into a loop, because this requires the loop condiiton to hold, whereas
  /// leaving the loop requires the loop condition not to hold which is
  /// contradictory
  comparePaths(
      PathsVec,
      {
          {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
           55, 56, 57, 58, 59, 60, 61, 69, 70, 71, 72, 73,
           0,  1,  2,  7,  8,  10, 20, 21, 74, 75, 76, 77},
          // {54, 55, 56, 57, 61, 71, 73, 26, 29, 32,
          //  33, 35, 36, 40, 32, 41, 42, 74, 75, 76},
          // {54, 55, 58, 61, 72, 73, 27, 29, 32, 34, 35, 36, 40,
          //  32, 41, 42, 74, 75, 76, 77},
          {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
           55, 56, 57, 58, 59, 60, 61, 69, 70, 71, 72, 73,
           22, 23, 24, 29, 30, 32, 41, 42, 74, 75, 76, 77},
          // {54, 55, 56, 57, 61, 71, 73, 4,  7,  10, 11,
          //  13, 14, 15, 19, 10, 20, 21, 74, 75, 76, 77},
          // {54, 55, 58, 61, 72, 73, 5,  7,  10, 12,
          //  13, 14, 15, 19, 10, 20, 21, 74, 75, 76, 77},
          {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
           55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
           22, 23, 24, 29, 30, 32, 41, 42, 67, 68, 76, 77},
          // {54, 55, 56, 57, 61, 64, 66, 26, 29, 32,
          //  33, 35, 36, 40, 32, 41, 42, 67, 68, 76, 77},
          // {54, 55, 58, 61, 65, 66, 27, 29, 32, 34, 35, 36, 40, 32, 41, 42,
          //  67, 68, 76, 77},
          {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
           55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
           0,  1,  2,  7,  8,  10, 20, 21, 67, 68, 76, 77},
          // {54, 55, 56, 57, 61, 64, 66, 4,  7,  10, 11,
          //  13, 14, 15, 19, 10, 20, 21, 67, 68, 76, 77},
          // {54, 55, 58, 61, 65, 66, 5,  7,  10, 12,
          //  13, 14, 15, 19, 10, 20, 21, 67, 68, 76, 77},
      });
}

TEST_F(PathTracingTest, Lambda_Inter_08) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  auto PathsVec = doLambdaAnalysis("inter_08_cpp.ll");
  /// FIXME: Handle mutable z3::exprs; As of now, we reject all paths that go
  /// into a loop, because this requires the loop condiiton to hold, whereas
  /// leaving the loop requires the loop condition not to hold which is
  /// contradictory
  comparePaths(PathsVec,
               {
                   {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                    57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 0,  1,  2,  3,
                    4,  5,  6,  7,  8,  9,  10, 20, 21, 67, 68, 76, 77},
                   {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                    57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 22, 23, 24, 25,
                    26, 27, 28, 29, 30, 31, 32, 41, 42, 67, 68, 76, 77},
                   {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                    57, 58, 59, 60, 61, 69, 70, 71, 72, 73, 0,  1,  2,  3,
                    4,  5,  6,  7,  8,  9,  10, 20, 21, 74, 75, 76, 77},
                   {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                    57, 58, 59, 60, 61, 69, 70, 71, 72, 73, 22, 23, 24, 25,
                    26, 27, 28, 29, 30, 31, 32, 41, 42, 74, 75, 76, 77},
                   // {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                   // 57, 58, 59,
                   //  60, 61, 62, 63, 64, 65, 66, 22, 23, 24, 25, 26, 27, 28,
                   //  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 30, 31,
                   //  32, 41, 42, 67, 68, 76, 77},
                   // {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                   // 57, 58, 59,
                   //  60, 61, 69, 70, 71, 72, 73, 22, 23, 24, 25, 26, 27, 28,
                   //  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 30, 31,
                   //  32, 41, 42, 74, 75, 76, 77},
                   // {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
                   //  56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 0,  1,
                   //  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
                   //  15, 16, 17, 18, 19, 8,  9,  10, 20, 21, 67, 68, 76, 77},
                   // {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
                   //  56, 57, 58, 59, 60, 61, 69, 70, 71, 72, 73, 0,  1,
                   //  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
                   //  15, 16, 17, 18, 19, 8,  9,  10, 20, 21, 74, 75, 76, 77},

               });
}

TEST_F(PathTracingTest, Handle_Inter_09) {
  auto PathsVec = doAnalysis("inter_09_cpp.ll", false);
  /// FIXME: Same reason as Handle_Inter_08
  comparePaths(
      PathsVec,
      {
          // {22, 2, 5, 6, 7, 8, 2, 5, 11, 12, 13, 14, 15, 9, 10, 14, 15},
          {16, 17, 18, 19, 20, 21, 22, 0, 2, 5, 11, 12, 13, 14, 15, 23, 24},
      });
}

TEST_F(PathTracingTest, Lambda_Inter_09) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  auto PathsVec = doLambdaAnalysis("inter_09_cpp.ll");
  /// FIXME: Same reason as Lambda_Inter_08
  comparePaths(PathsVec, {
                             {16, 17, 18, 19, 20, 21, 22, 0,  1,  2,
                              3,  4,  5,  11, 12, 13, 14, 15, 23, 24},
                             //  {16, 17, 18, 19, 20, 21, 22, 0,  1,  2, 3,
                             //   4,  5,  6,  7,  8,  0,  1,  2,  3,  4, 5,
                             //   11, 12, 13, 14, 15, 9,  10, 14, 15, 23, 24},
                         });
}

TEST_F(PathTracingTest, Handle_Inter_10) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  GTEST_SKIP() << "Need globals support";
  auto PathsVec = doAnalysis("inter_10_cpp.ll", false);
  /// TODO: GT
}

TEST_F(PathTracingTest, Handle_Inter_11) {
  auto PathsVec = doAnalysis("inter_11_cpp.ll");
  // Note: The alias analysis is strong enough to see that Three::assignValue
  // can never be called
  comparePaths(PathsVec, {{8,  9,  10, 11, 12, 13, 14, 15, 16, 24, 25,
                           26, 27, 30, 31, 32, 33, 34, 28, 29, 17, 18,
                           19, 20, 21, 22, 35, 38, 40, 41, 42, 23}});
}

TEST_F(PathTracingTest, Lambda_Inter_11) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  auto PathsVec = doLambdaAnalysis("inter_11_cpp.ll");
  // Note: The alias analysis is strong enough to see that Three::assignValue
  // can never be called
  comparePaths(PathsVec, {{8,  9,  10, 11, 12, 13, 14, 15, 16, 24, 25, 26,
                           27, 30, 31, 32, 33, 34, 28, 29, 17, 18, 19, 20,
                           21, 22, 35, 36, 37, 38, 39, 40, 41, 42, 23}});
}

TEST_F(PathTracingTest, Handle_Inter_12) {
  auto PathsVec = doAnalysis("inter_12_cpp.ll");
  comparePaths(
      PathsVec,
      {{11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52, 53,
        54, 42, 43, 20, 21, 22, 23, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 66, 69, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41,  50,  51, 52, 53,
        54, 42, 43, 20, 21, 22, 23, 29, 30, 44, 45, 46, 47,  50,  51, 52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 94, 97, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51,
        52, 53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 74, 75,
        76, 77, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53, 54, 48,
        49, 31, 32, 33, 34, 35, 36, 66, 69, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39,  40,  41, 50, 51,
        52, 53, 54, 42, 43, 20, 21, 22, 23, 24, 25,  26,  27, 74, 75,
        76, 77, 28, 29, 30, 44, 45, 46, 47, 50, 51,  52,  53, 54, 48,
        49, 31, 32, 33, 34, 35, 36, 94, 97, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52,
        53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 83, 84, 85, 86,
        74, 75, 76, 77, 87, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 66, 69, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41,  50,  51, 52,
        53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 83,  84,  85, 86,
        74, 75, 76, 77, 87, 28, 29, 30, 44, 45, 46, 47, 50,  51,  52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 94, 97, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52,
        53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 55, 56, 57, 58,
        74, 75, 76, 77, 59, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 66, 69, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41,  50,  51, 52,
        53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 55,  56,  57, 58,
        74, 75, 76, 77, 59, 28, 29, 30, 44, 45, 46, 47, 50,  51,  52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 94, 97, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52, 53, 54,
        42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 88, 89, 90, 91, 83, 84, 85, 86,
        74, 75, 76, 77, 87, 92, 93, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 66, 69, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39,  40,  41, 50,
        51, 52, 53, 54, 42, 43, 20, 21, 22, 23, 24,  25,  26, 27,
        88, 89, 90, 91, 83, 84, 85, 86, 74, 75, 76,  77,  87, 92,
        93, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52,  53,  54, 48,
        49, 31, 32, 33, 34, 35, 36, 94, 97, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52, 53, 54,
        42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 60, 61, 62, 63, 55, 56, 57, 58,
        74, 75, 76, 77, 59, 64, 65, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 66, 69, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39,  40,  41, 50,
        51, 52, 53, 54, 42, 43, 20, 21, 22, 23, 24,  25,  26, 27,
        60, 61, 62, 63, 55, 56, 57, 58, 74, 75, 76,  77,  59, 64,
        65, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52,  53,  54, 48,
        49, 31, 32, 33, 34, 35, 36, 94, 97, 99, 100, 101, 37}});
}

TEST_F(PathTracingTest, Lambda_Inter_12) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  // "PathSensitivityManager");
  auto PathsVec = doLambdaAnalysis("inter_12_cpp.ll");
  comparePaths(
      PathsVec,
      {{11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39,  40,  41,
        50, 51, 52, 53, 54, 42, 43, 20, 21, 22, 23,  29,  30,
        44, 45, 46, 47, 50, 51, 52, 53, 54, 48, 49,  31,  32,
        33, 34, 35, 36, 94, 95, 96, 97, 98, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52, 53, 54,
        42, 43, 20, 21, 22, 23, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53, 54, 48,
        49, 31, 32, 33, 34, 35, 36, 66, 67, 68, 69, 70, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39,  40,  41, 50, 51, 52,
        53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26,  27,  74, 75, 76, 77,
        28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53,  54,  48, 49, 31, 32,
        33, 34, 35, 36, 94, 95, 96, 97, 98, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52,
        53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 74, 75, 76, 77,
        28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53, 54, 48, 49, 31, 32,
        33, 34, 35, 36, 66, 67, 68, 69, 70, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41,  50,  51, 52, 53,
        54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 55, 56,  57,  58, 74, 75,
        76, 77, 59, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52,  53,  54, 48, 49,
        31, 32, 33, 34, 35, 36, 94, 95, 96, 97, 98, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52, 53,
        54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 55, 56, 57, 58, 74, 75,
        76, 77, 59, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53, 54, 48, 49,
        31, 32, 33, 34, 35, 36, 66, 67, 68, 69, 70, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41,  50,  51, 52, 53,
        54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 83, 84,  85,  86, 74, 75,
        76, 77, 87, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52,  53,  54, 48, 49,
        31, 32, 33, 34, 35, 36, 94, 95, 96, 97, 98, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52, 53,
        54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 83, 84, 85, 86, 74, 75,
        76, 77, 87, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53, 54, 48, 49,
        31, 32, 33, 34, 35, 36, 66, 67, 68, 69, 70, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38,  39,  40, 41, 50, 51,
        52, 53, 54, 42, 43, 20, 21, 22, 23, 24,  25,  26, 27, 88, 89,
        90, 91, 83, 84, 85, 86, 74, 75, 76, 77,  87,  92, 93, 28, 29,
        30, 44, 45, 46, 47, 50, 51, 52, 53, 54,  48,  49, 31, 32, 33,
        34, 35, 36, 94, 95, 96, 97, 98, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51, 52, 53, 54,
        42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 88, 89, 90, 91, 83, 84, 85, 86,
        74, 75, 76, 77, 87, 92, 93, 28, 29, 30, 44, 45, 46, 47, 50, 51, 52, 53,
        54, 48, 49, 31, 32, 33, 34, 35, 36, 66, 67, 68, 69, 70, 71, 72, 73, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38,  39,  40, 41, 50, 51,
        52, 53, 54, 42, 43, 20, 21, 22, 23, 24,  25,  26, 27, 60, 61,
        62, 63, 55, 56, 57, 58, 74, 75, 76, 77,  59,  64, 65, 28, 29,
        30, 44, 45, 46, 47, 50, 51, 52, 53, 54,  48,  49, 31, 32, 33,
        34, 35, 36, 94, 95, 96, 97, 98, 99, 100, 101, 37},
       {11, 12, 13, 14, 15, 16, 17, 18, 19, 38, 39, 40, 41, 50, 51,
        52, 53, 54, 42, 43, 20, 21, 22, 23, 24, 25, 26, 27, 60, 61,
        62, 63, 55, 56, 57, 58, 74, 75, 76, 77, 59, 64, 65, 28, 29,
        30, 44, 45, 46, 47, 50, 51, 52, 53, 54, 48, 49, 31, 32, 33,
        34, 35, 36, 66, 67, 68, 69, 70, 71, 72, 73, 37}});
}

TEST_F(PathTracingTest, Handle_Intra_01) {
  auto PathsVec = doAnalysis("intra_01_cpp.ll");
  comparePaths(PathsVec, {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}});
}

TEST_F(PathTracingTest, Handle_Intra_02) {
  auto PathsVec = doAnalysis("intra_02_cpp.ll");
  comparePaths(PathsVec, {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15},
                          {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15}});
}

TEST_F(PathTracingTest, Handle_Intra_03) {
  auto PathsVec = doAnalysis("intra_03_cpp.ll");
  comparePaths(PathsVec, {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                           16, 19, 20}});
}

TEST_F(PathTracingTest, Handle_Intra_04) {
  //   psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                       "PathSensitivityManager");
  auto PathsVec = doAnalysis("intra_04_cpp.ll");
  /// FIXME: Same reason as Handle_Inter_08
  comparePaths(PathsVec,
               {
                   // {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                   // 18, 21, 22, 23, 24, 25, 29, 21, 30, 33, 34},
                   {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                    12, 13, 14, 15, 16, 18, 19, 21, 30, 33, 34},
               });
}

TEST_F(PathTracingTest, Handle_Intra_05) {
  //   psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                       "PathSensitivityManager");
  auto PathsVec = doAnalysis("intra_05_cpp.ll");
  comparePaths(
      PathsVec,
      {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 19, 20},
       {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20}});
}

TEST_F(PathTracingTest, Handle_Intra_06) {
  auto PathsVec = doAnalysis("intra_06_cpp.ll");
  comparePaths(PathsVec, {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16},
                          {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16}});
}

TEST_F(PathTracingTest, Handle_Intra_07) {
  //   psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                       "PathSensitivityManager");
  auto PathsVec = doAnalysis("intra_07_cpp.ll");
  /// FIXME: Same reason as Handle_Inter_08
  comparePaths(
      PathsVec,
      {
          //  {16, 17, 19, 22, 31, 34, 19, 22, 23, 24, 25, 26, 30, 22, 31,
          //   34, 35, 38, 39},
          // {16, 17, 18, 19, 20, 21, 22, 31, 32, 33, 34,
          //  18, 19, 20, 21, 22, 31, 32, 33, 34, 35, 38, 39},
          //  {16, 17, 19, 22, 23, 24, 25, 26, 30, 22, 31, 34, 35, 38, 39},
          //  {16, 17, 19, 22, 31, 34, 19, 22, 31, 34, 35, 38, 39},
          {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
           14, 15, 16, 17, 18, 19, 20, 22, 31, 32, 34, 35, 38, 39},
      });
}

TEST_F(PathTracingTest, Handle_Intra_08) {
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                     "PathSensitivityManager");
  auto PathsVec = doAnalysis("intra_08_cpp.ll", false);
  // for (const auto &Path : PathsVec) {
  //   for (const auto *Inst : Path) {
  //     llvm::errs() << " " << psr::getMetaDataID(Inst);
  //   }
  //   llvm::errs() << '\n';
  // }

  /// NOTE: we have a fallthrough from case 4 to default; Therefore, we only
  /// have 3 paths
  /// UPDATE: Despite the fallthrough, clang-14 now generates 4 distinct
  /// switch-cases, where the ID13 one just branches into the default case. So,
  /// we now have 4 cases!
  comparePaths(PathsVec, {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 18},
                          {0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 17, 18},
                          {0, 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 17, 18},
                          {0, 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15, 16, 17, 18}});
}

TEST_F(PathTracingTest, Handle_Intra_09) {
  //   psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                       "PathSensitivityManager");
  auto PathsVec = doAnalysis("intra_09_cpp.ll");
  /// FIXME: Same reason as Handle_Inter_08
  comparePaths(PathsVec,
               {
                   //  {0, 1, 2, 5, 11, 14, 16, 17, 18, 22, 14, 23, 24},
                   //  {0, 1, 2, 4, 11, 14, 15, 17, 18, 22, 14, 23, 24},
                   {0, 1, 2, 3, 11, 12, 14, 23, 24},
               });
}

TEST_F(PathTracingTest, Handle_Other_01) {
  auto PathsVec = doAnalysis("other_01_cpp.ll");
  comparePaths(PathsVec, {{0, 1, 6, 7, 8, 9, 10, 12, 13}});
}

TEST(PathsDAGTest, ForwardMinimizeDAGTest) {
  psr::AdjacencyList<int> Graph;
  using traits_t = psr::GraphTraits<decltype(Graph)>;
  auto One = traits_t::addNode(Graph, 1);
  auto Two = traits_t::addNode(Graph, 2);
  auto Three = traits_t::addNode(Graph, 2);
  auto Four = traits_t::addNode(Graph, 2);

  traits_t::addRoot(Graph, One);
  traits_t::addEdge(Graph, One, Two);
  traits_t::addEdge(Graph, One, Three);
  traits_t::addEdge(Graph, One, Four);

  auto Eq = psr::minimizeGraph(Graph);
  Graph = psr::createEquivalentGraphFrom(std::move(Graph), Eq);
  // psr::printGraph(Graph, llvm::outs());
  // llvm::outs() << '\n';

  EXPECT_EQ(traits_t::size(Graph), 2);
  ASSERT_EQ(traits_t::roots_size(Graph), 1);
  auto Rt = traits_t::roots(Graph)[0];
  EXPECT_EQ(traits_t::outDegree(Graph, Rt), 1);
  EXPECT_NE(traits_t::target(traits_t::outEdges(Graph, Rt)[0]), Rt);
  EXPECT_EQ(traits_t::outDegree(
                Graph, traits_t::target(traits_t::outEdges(Graph, Rt)[0])),
            0);
}

template <typename GraphTy>
std::vector<std::vector<std::string>> getPaths(const GraphTy &G) {
  std::vector<std::vector<std::string>> Ret;
  std::vector<std::string> Curr;
  using traits_t = psr::GraphTraits<GraphTy>;
  auto doGetPaths = [&G, &Curr, &Ret](const auto &doGetPaths,
                                      auto Vtx) -> void {
    size_t Sz = Curr.size();
    for (const auto *N : traits_t::node(G, Vtx)) {
      Curr.push_back(psr::getMetaDataID(N));
    }
    bool HasSucc = false;
    for (auto Edge : traits_t::outEdges(G, Vtx)) {
      HasSucc = true;
      doGetPaths(doGetPaths, traits_t::target(Edge));
    }
    if (!HasSucc) {
      Ret.push_back(Curr);
    }
    Curr.resize(Sz);
  };

  for (auto Rt : traits_t::roots(G)) {
    doGetPaths(doGetPaths, Rt);
  }

  return Ret;
}

TEST(PathsDAGTest, InLLVMSSA) {
  psr::LLVMProjectIRDB IRDB(PathTracingTest::PathToLlFiles + "inter_01_cpp.ll");
  psr::DIBasedTypeHierarchy TH(IRDB);
  psr::LLVMAliasSet PT(&IRDB);
  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"}, &TH,
                          &PT, psr::Soundness::Soundy,
                          /*IncludeGlobals*/ false);
  psr::IDELinearConstantAnalysis LCAProblem(&IRDB, &ICFG, {"main"});
  psr::PathAwareIDESolver LCASolver(LCAProblem, &ICFG);
  LCASolver.solve();
  // if (PrintDump) {
  //   // IRDB->print();
  //   // ICFG.print();
  //   // LCASolver.dumpResults();
  //   std::error_code EC;
  //   llvm::raw_fd_ostream ROS(LlvmFilePath + "_explicit_esg.dot", EC);
  //   assert(!EC);
  //   LCASolver.getExplicitESG().printAsDot(ROS);
  // }
  auto [LastInst, InterestingFact] =
      PathTracingTest::getInterestingInstFact(IRDB);
  // llvm::outs() << "Target instruction: " << psr::llvmIRToString(LastInst);
  // llvm::outs() << "\nTarget data-flow fact: "
  //              << psr::llvmIRToString(InterestingFact) << '\n';

  const auto *InterestingFactInst =
      llvm::dyn_cast<llvm::Instruction>(InterestingFact);
  ASSERT_FALSE(InterestingFact->getType()->isVoidTy());
  ASSERT_NE(nullptr, InterestingFactInst);

  psr::PathSensitivityManager<psr::IDELinearConstantAnalysisDomain> PSM(
      &LCASolver.getExplicitESG());

  auto Dag = PSM.pathsDagToInLLVMSSA(InterestingFactInst, InterestingFact,
                                     psr::PathSensitivityConfig{});

  // psr::printGraph(Dag, llvm::outs(), "", [](const auto &Node) {
  //   std::string Str;
  //   llvm::raw_string_ostream ROS(Str);
  //   ROS << '[';
  //   llvm::interleaveComma(Node, ROS, [&ROS](const auto *Inst) {
  //     ROS << psr::getMetaDataID(Inst);
  //   });
  //   ROS << ']';

  //   return Str;
  // });
  auto Paths = getPaths(Dag);
  ASSERT_EQ(1, Paths.size());

  // TODO: Should the "18" be removed?
  std::vector<std::string> Gt = {"17", "16", "5",  "3",  "2",  "1",
                                 "0",  "15", "14", "13", "12", "11",
                                 "10", "9",  "8",  "7",  "6",  "18"};
  EXPECT_EQ(Gt, Paths[0]);
}

} // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
