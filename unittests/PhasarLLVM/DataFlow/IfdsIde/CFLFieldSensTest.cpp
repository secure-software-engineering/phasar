#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAllocSitesAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FieldSensAllocSitesAwareIFDSProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Instruction.h"

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
            psr::FieldSensAllocSitesAwareIFDSProblemBase::getBaseAndOffset(
                KillFact, DL);
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

      if (Leak.count(Source)) {
        Leaks[CS] = Source;
      }

      if (Kill.count(Source)) {
        return {};
      }

      return {Source};
    });
  }

  llvm::DenseMap<n_t, d_t> Leaks{};

private:
  const psr::LLVMTaintConfig *Config{};
};

class CFLFieldSensTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("xtaint/");
  const std::vector<std::string> EntryPoints = {"main"};

  void run(const llvm::Twine &IRFileName,
           const std::map<int, std::set<std::string>> &GroundTruth) {
    psr::LLVMProjectIRDB IRDB(IRFileName);
    ASSERT_TRUE(IRDB);

    psr::LLVMAliasSet BaseAS(&IRDB);
    psr::FilteredLLVMAliasSet AS(&BaseAS);
    psr::LLVMTaintConfig TC(IRDB);
    ExampleTaintAnalysis TaintProblem(&IRDB, &AS, &TC, {"main"});

    psr::FieldSensAllocSitesAwareIFDSProblem FsTaintProblem(
        &TaintProblem, &AS,
        {
            .KillsAt = TaintProblem.killsAt(),
        });

    psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"},
                            nullptr, &BaseAS);

    psr::IterativeIDESolver Solver(&FsTaintProblem, &ICFG);
    Solver.solve();
    auto Results = Solver.getSolverResults();

    Solver.dumpResults();

    // auto Results = psr::solveIDEProblem(FsTaintProblem, ICFG);
    // Results.dumpResults(ICFG);

    std::map<int, std::set<std::string>> ComputedLeaks;

    for (auto IIt = TaintProblem.Leaks.begin(), End = TaintProblem.Leaks.end();
         IIt != End;) {
      auto It = IIt++;
      const auto &[LeakInst, LeakFact] = *It;

      const auto &Res = Results.resultAt(LeakInst, LeakFact);
      if (const auto *FieldStrings = Res.getValueOrNull()) {
        if (llvm::all_of(FieldStrings->Paths,
                         [](const auto &F) { return !F.empty(); })) {
          llvm::errs() << "> Erase leak at " << psr::llvmIRToString(LeakInst)
                       << "; because leaking fact "
                       << psr::llvmIRToShortString(LeakFact)
                       << " has non-empty field-string: " << Res << '\n';
          TaintProblem.Leaks.erase(It);
        } else {
          ComputedLeaks[stoi(psr::getMetaDataID(LeakInst))].insert(
              psr::getMetaDataID(LeakFact));
        }
      }
    }

    EXPECT_EQ(GroundTruth, ComputedLeaks);
  }
};

TEST_F(CFLFieldSensTest, Basic_01) {
  std::map<int, std::set<std::string>> Gt;

  Gt[13] = {"12"};

  run({PathToLLFiles + "xtaint01_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_02) {
  // GTEST_SKIP() << "Need field-sensitive alias information!";

  std::map<int, std::set<std::string>> Gt;

  Gt[18] = {"17"};

  run({PathToLLFiles + "xtaint02_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_03) {
  std::map<int, std::set<std::string>> Gt;

  Gt[21] = {"20"};

  run({PathToLLFiles + "xtaint03_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_04) {
  std::map<int, std::set<std::string>> Gt;

  Gt[16] = {"15"};

  run({PathToLLFiles + "xtaint04_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_06) {
  std::map<int, std::set<std::string>> Gt;

  // no leaks expected

  run({PathToLLFiles + "xtaint06_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_09_1) {
  std::map<int, std::set<std::string>> Gt;

  Gt[25] = {"24"};

  run({PathToLLFiles + "xtaint09_1_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_09) {
  std::map<int, std::set<std::string>> Gt;

  Gt[24] = {"23"};

  run({PathToLLFiles + "xtaint09_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_12) {
  std::map<int, std::set<std::string>> Gt;

  // We sanitize an alias - since we don't have must-alias relations, we cannot
  // kill aliases at all
  Gt[28] = {"27"};

  run({PathToLLFiles + "xtaint12_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_13) {
  std::map<int, std::set<std::string>> Gt;

  Gt[30] = {"29"};

  run({PathToLLFiles + "xtaint13_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_14) {
  std::map<int, std::set<std::string>> Gt;

  Gt[33] = {"32"};

  run({PathToLLFiles + "xtaint14_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_16) {
  std::map<int, std::set<std::string>> Gt;

  Gt[24] = {"23"};

  run({PathToLLFiles + "xtaint16_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_17) {
  std::map<int, std::set<std::string>> Gt;

  Gt[27] = {"26"};

  run({PathToLLFiles + "xtaint17_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_18) {
  std::map<int, std::set<std::string>> Gt;

  // no leaks expected

  run({PathToLLFiles + "xtaint18_cpp.ll"}, Gt);
}

TEST_F(CFLFieldSensTest, Basic_20) {
  std::map<int, std::set<std::string>> Gt;

  Gt[22] = {"14"};
  // Gt[24] = {"23"}; // no leak here, because above we define the semantics to
  // exclude deep taints!

  run({PathToLLFiles + "xtaint20_cpp.ll"}, Gt);
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
