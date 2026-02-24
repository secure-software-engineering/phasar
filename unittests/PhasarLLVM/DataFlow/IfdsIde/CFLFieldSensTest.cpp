#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/CFLFieldSensIFDSProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAllocSitesAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/Twine.h"
#include "llvm/IR/Instruction.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <cstdint>
#include <optional>

namespace {

template <typename AliasInfoTy>
void populateWithMayAliases(const AliasInfoTy &AS,
                            std::set<const llvm::Value *> &Facts,
                            const llvm::Instruction *Context) {
  auto Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = AS.getAliasSet(Fact, Context);
    Tmp.insert(Aliases->begin(), Aliases->end());
  }

  Facts = std::move(Tmp);
}

class ExampleTaintAnalysis : public psr::DefaultAllocSitesAwareIFDSProblem {
public:
  explicit ExampleTaintAnalysis(const psr::LLVMProjectIRDB *IRDB,
                                psr::LLVMAliasInfoRef AS,
                                const psr::LLVMTaintConfig *Config,
                                std::vector<std::string> EntryPoints)
      : psr::DefaultAllocSitesAwareIFDSProblem(
            IRDB, AS, std::move(EntryPoints),
            psr::LLVMZeroValue::getInstance()),
        Config(&psr::assertNotNull(Config)) {
    this->disableStrongUpdateStore();
  }

  [[nodiscard]] psr::InitialSeeds<n_t, d_t, l_t> initialSeeds() override {

    psr::InitialSeeds<n_t, d_t, l_t> Seeds = Config->makeInitialSeeds();

    psr::LLVMBasedCFG CFG;

    addSeedsForStartingPoints(EntryPoints, IRDB, CFG, Seeds, getZeroValue(),
                              psr::BinaryDomain::BOTTOM);

    return Seeds;
  };

  [[nodiscard]] auto killsAt() const {
    return [this](n_t Curr, d_t CurrNode) -> std::optional<int32_t> {
      const auto *CS = llvm::dyn_cast<llvm::CallBase>(Curr);
      if (!CS) {
        return std::nullopt;
      }

      const auto *DestFun = CS->getCalledFunction();
      if (!DestFun) {
        return std::nullopt;
      }

      container_type Kill;
      psr::collectSanitizedFacts(Kill, *Config, CS, DestFun);

      const auto &DL = IRDB->getModule()->getDataLayout();

      for (const auto *KillFact : Kill) {
        auto [BasePtr, Offset] =
            psr::cfl_fieldsens::getBaseAndOffset(KillFact, DL);
        if (BasePtr == CurrNode) {
          return Offset;
        }
      }

      return std::nullopt;
    };
  }

  [[nodiscard]] FlowFunctionPtrType
  getSummaryFlowFunction(n_t CallSite, f_t DestFun) override {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

    container_type Gen;
    container_type Leak;
    container_type Kill;

    psr::collectGeneratedFacts(Gen, *Config, CS, DestFun);
    psr::collectLeakedFacts(Leak, *Config, CS, DestFun);
    psr::collectSanitizedFacts(Kill, *Config, CS, DestFun);

    if (Gen.empty() && Leak.empty() && Kill.empty()) {
      return DefaultAllocSitesAwareIFDSProblem::getSummaryFlowFunction(CS,
                                                                       DestFun);
    }

    populateWithMayAliases(getAliasInfo(), Gen, CallSite);
    populateWithMayAliases(getAliasInfo(), Leak, CallSite);

    return lambdaFlow([this, CS, Gen{std::move(Gen)}, Leak{std::move(Leak)},
                       Kill{std::move(Kill)}](d_t Source) -> container_type {
      if (isZeroValue(Source)) {
        return Gen;
      }

      if (Leak.contains(Source)) {
        Leaks[CS] = Source;
      }

      if (Kill.contains(Source)) {
        return {};
      }

      return {Source};
    });
  }

  llvm::DenseMap<n_t, d_t> Leaks{};

private:
  const psr::LLVMTaintConfig *Config{};
};

using namespace psr::unittest;

class CFLFieldSensTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("xtaint/");
  const std::vector<std::string> EntryPoints = {"main"};

  using TaintSetT = std::set<TestingSrcLocation>;

  void run(const llvm::Twine &IRFileName,
           const std::map<TestingSrcLocation, TaintSetT> &GroundTruth,
           bool ShouldDumpResults = false) {
    auto IRDB = psr::LLVMProjectIRDB::loadOrExit(IRFileName);

    auto GroundTruthEntries =
        convertTestingLocationSetMapInIR(GroundTruth, IRDB);

    psr::LLVMAliasSet BaseAS(&IRDB);
    psr::FilteredLLVMAliasSet AS(&BaseAS);
    psr::LLVMTaintConfig TC(IRDB);
    ExampleTaintAnalysis TaintProblem(&IRDB, &AS, &TC, {"main"});

    psr::CFLFieldSensIFDSProblem FsTaintProblem(&TaintProblem);

    psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"},
                            nullptr, &BaseAS);

    psr::IterativeIDESolver Solver(&FsTaintProblem, &ICFG);
    Solver.solve();
    auto Results = Solver.getSolverResults();

    // auto Results = psr::solveIDEProblem(FsTaintProblem, ICFG);
    // Results.dumpResults(ICFG);

    std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
        ComputedLeaks;

    for (auto IIt = TaintProblem.Leaks.begin(), End = TaintProblem.Leaks.end();
         IIt != End;) {
      auto It = IIt++;
      const auto &[LeakInst, LeakFact] = *It;

      const auto &Res = Results.resultAt(LeakInst, LeakFact);
      if (const auto *FieldStrings = Res.getValueOrNull()) {
        if (FieldStrings->Paths.empty()) {
          llvm::errs() << "> Erase leak at " << psr::llvmIRToString(LeakInst)
                       << "; because leaking fact "
                       << psr::llvmIRToShortString(LeakFact)
                       << " has empty set of access-paths: " << Res << '\n';
          TaintProblem.Leaks.erase(It);
          continue;
        }
      }
      ComputedLeaks[LeakInst].insert(LeakFact);
    }

    EXPECT_EQ(GroundTruthEntries, ComputedLeaks);
    if (ShouldDumpResults || HasFailure()) {
      Solver.dumpResults();
    }
  }
};

TEST_F(CFLFieldSensTest, Basic_01) {

  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{8, 3, "main"},
       {LineColFunOp{8, 9, "main", llvm::Instruction::Load}}},
  };

  run({PathToLLFiles + "xtaint01_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_02) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{9, 3, "main"},
       {LineColFunOp{9, 9, "main", llvm::Instruction::Load}}},
  };

  run({PathToLLFiles + "xtaint02_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_03) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{10, 3, "main"},
       {LineColFunOp{10, 9, "main", llvm::Instruction::Load}}},
  };

  run({PathToLLFiles + "xtaint03_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_04) {
  auto Call = LineColFun{6, 3, "_Z3barPi"};

  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {Call, {OperandOf{0, Call}}},
  };

  run({PathToLLFiles + "xtaint04_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_06) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      // no leaks expected
  };

  run({PathToLLFiles + "xtaint06_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_09_1) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{14, 3, "main"}, {LineColFun{14, 8, "main"}}},
  };

  run({PathToLLFiles + "xtaint09_1_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_09) {
  auto SinkCall = LineColFun{16, 3, "main"};
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {SinkCall, {OperandOf{0, SinkCall}}},
  };

  run({PathToLLFiles + "xtaint09_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_12) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{19, 3, "main"}, {LineColFun{19, 8, "main"}}},
  };

  // We sanitize an alias - since we don't have must-alias relations, we cannot
  // kill aliases at all

  run({PathToLLFiles + "xtaint12_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_13) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{17, 3, "main"}, {LineColFun{17, 8, "main"}}},
  };

  run({PathToLLFiles + "xtaint13_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_14) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{24, 3, "main"}, {LineColFun{24, 8, "main"}}},
  };

  run({PathToLLFiles + "xtaint14_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_16) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{13, 3, "main"}, {LineColFun{13, 8, "main"}}},
  };

  run({PathToLLFiles + "xtaint16_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_17) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{17, 3, "main"}, {LineColFun{17, 8, "main"}}},
  };

  run({PathToLLFiles + "xtaint17_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_18) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      // no leaks expected
  };

  run({PathToLLFiles + "xtaint18_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_20) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{12, 3, "main"}, {LineColFun{6, 7, "main"}}},
      {LineColFun{13, 3, "main"}, {LineColFun{13, 8, "main"}}},
  };

  run({PathToLLFiles + "xtaint20_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_22) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{9, 5, "main"}, {LineColFun{9, 11, "main"}}},
  };

  // psr::Logger::initializeStderrLogger(
  //     psr::SeverityLevel::DEBUG,
  //     psr::FieldSensAllocSitesAwareIFDSProblem::LogCategory.str());

  run({PathToLLFiles + "xtaint22_cpp_dbg.ll"}, GroundTruth);
}

TEST_F(CFLFieldSensTest, Basic_23) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{17, 5, "main"}, {LineColFun{17, 11, "main"}}},
  };

  run({PathToLLFiles + "xtaint23_cpp_dbg.ll"}, GroundTruth);
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
