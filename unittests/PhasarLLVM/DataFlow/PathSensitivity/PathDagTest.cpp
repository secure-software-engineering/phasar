#include "phasar/DataFlow/IfdsIde/Solver/PathAwareIDESolver.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityConfig.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/DefaultPathSensitivityManager.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <cassert>
#include <memory>
#include <string>

namespace {

using namespace psr;

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
  std::unique_ptr<LLVMProjectIRDB> IRDB;

  void doLambdaAnalysis(const std::string &LlvmFilePath,
                        llvm::ArrayRef<std::vector<unsigned>> GroundTruth) {
    LLVMProjectIRDB IRDB(PathToLlFiles + LlvmFilePath);
    psr::DIBasedTypeHierarchy TH(IRDB);
    psr::LLVMAliasSet PT(&IRDB);
    psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"},
                            &TH, &PT, psr::Soundness::Soundy,
                            /*IncludeGlobals*/ false);

    psr::LLVMTaintConfig Config(IRDB);
    psr::IDEExtendedTaintAnalysis<3, false> Analysis(&IRDB, &ICFG, &PT, Config,
                                                     {"main"});
    psr::PathAwareIDESolver Solver(Analysis, &ICFG);
    Solver.solve();

    auto *Main = IRDB.getFunctionDefinition("main");
    assert(Main);
    auto *LastInst = &Main->back().back();
    // llvm::outs() << "Target instruction: " << psr::llvmIRToString(LastInst)
    //              << '\n';

    IRDB.emitPreprocessedIR(llvm::errs());

    llvm::LoopAnalysis LA;
    llvm::FunctionAnalysisManager FAM;

    llvm::PassBuilder PB;
    PB.registerFunctionAnalyses(FAM);

    auto Loops = LA.run(*Main, FAM);
    Loops.print(llvm::errs());

    // std::error_code EC;
    // llvm::raw_fd_ostream ROS(LlvmFilePath + "_explicit_esg.dot", EC);
    // assert(!EC);
    // Solver.getExplicitESG().printAsDot(ROS);

    psr::DefaultPathSensitivityManager<psr::IDEExtendedTaintAnalysisDomain> PSM(
        &Solver.getExplicitESG(), PathSensitivityConfig()
                                      .withPreventCycles(false)
                                      //.withMaxPathLength(50)
                                      .withMaxUnrollFactor(2));

    auto Graph =
        PSM.pathsGraphTo(LastInst, Analysis.getZeroValue(), PSM.getConfig());

    llvm::errs() << "built graph...\n";

    auto Paths = PSM.pathsTo(LastInst, std::move(Graph));

    comparePaths(Paths, GroundTruth);
  }

  void
  comparePaths(const psr::DefaultFlowPathSequence<const llvm::Instruction *>
                   &AnalyzedPaths,
               llvm::ArrayRef<std::vector<unsigned>> GroundTruth) {
    std::set<size_t> MatchingIndices;
    auto Matches = [&AnalyzedPaths,
                    &MatchingIndices](llvm::ArrayRef<unsigned> GT) {
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

TEST_F(PathTracingTest, Handle_Intra_09) {
  doLambdaAnalysis("intra_09_cpp.ll", {/*TODO*/});
}

TEST_F(PathTracingTest, Handle_Intra_10) {
  doLambdaAnalysis("intra_10_cpp.ll", {/*TODO*/});
}

TEST_F(PathTracingTest, Handle_Intra_11) {
  doLambdaAnalysis("intra_11_cpp.ll", {/*TODO*/});
}

} // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
