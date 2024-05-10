#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstrTypes.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

namespace {

static LLVMTaintConfig getDefaultConfig() {
  auto SourceCB = [](const llvm::Instruction *Inst) {
    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        Call->getCalledFunction()->getName() == "_Z6sourcev") {
      Ret.insert(Call);
    }
    return Ret;
  };
  auto SinkCB = [](const llvm::Instruction *Inst) {
    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        Call->getCalledFunction()->getName() == "_Z4sinki") {
      assert(Call->arg_size() > 0);
      Ret.insert(Call->getArgOperand(0));
    }
    return Ret;
  };
  return LLVMTaintConfig(std::move(SourceCB), std::move(SinkCB));
}

static LLVMTaintConfig getDoubleFreeConfig() {
  auto SourceCB = [](const llvm::Instruction *Inst) {
    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        Call->getCalledFunction()->getName() == "free") {
      Ret.insert(Call->getArgOperand(0));
    }
    return Ret;
  };

  return LLVMTaintConfig(SourceCB, SourceCB);
}

class TaintAnalysis : public ::testing::TestWithParam<std::string_view> {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("taint_analysis/");
  const std::vector<std::string> EntryPoints = {"main"};

}; // Test Fixture

TEST_P(TaintAnalysis, LeaksWithAndWithoutAliasFilteringEqual) {

  LLVMProjectIRDB IRDB(PathToLlFiles + GetParam());
  LLVMAliasSet AS(&IRDB, false);
  FilteredLLVMAliasSet FAS(&AS);
  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, EntryPoints, nullptr,
                    &AS);

  auto TSF = llvm::StringRef(GetParam()).startswith("double_free")
                 ? getDoubleFreeConfig()
                 : getDefaultConfig();

  IFDSTaintAnalysis TaintProblem(&IRDB, &AS, &TSF, EntryPoints);
  IFDSTaintAnalysis FilterTaintProblem(&IRDB, &FAS, &TSF, EntryPoints);

  solveIFDSProblem(TaintProblem, ICF).dumpResults(ICF);
  solveIFDSProblem(FilterTaintProblem, ICF).dumpResults(ICF);

  EXPECT_EQ(TaintProblem.Leaks.size(), FilterTaintProblem.Leaks.size());

  for (const auto &[LeakInst, LeakFacts] : TaintProblem.Leaks) {
    const auto It = FilterTaintProblem.Leaks.find(LeakInst);

    EXPECT_NE(It, FilterTaintProblem.Leaks.end())
        << "Expected to find leak at " + llvmIRToString(LeakInst);

    if (It == FilterTaintProblem.Leaks.end()) {
      continue;
    }

    for (const auto *LeakFact : LeakFacts) {
      EXPECT_TRUE(It->second.count(LeakFact))
          << "Expected to find leak-fact " + llvmIRToShortString(LeakFact) +
                 " at " + llvmIRToString(LeakInst);
    }
  }
}

static constexpr std::string_view TaintTestFiles[] = {
    // -- dummy-source-sink
    "dummy_source_sink/taint_01_cpp_dbg.ll",
    "dummy_source_sink/taint_01_cpp_m2r_dbg.ll",
    "dummy_source_sink/taint_02_cpp_dbg.ll",
    "dummy_source_sink/taint_03_cpp_dbg.ll",
    "dummy_source_sink/taint_04_cpp_dbg.ll",
    "dummy_source_sink/taint_05_cpp_dbg.ll",
    "dummy_source_sink/taint_06_cpp_m2r_dbg.ll",
    "dummy_source_sink/taint_exception_01_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_01_cpp_m2r_dbg.ll",
    "dummy_source_sink/taint_exception_02_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_03_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_04_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_05_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_06_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_07_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_08_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_09_cpp_dbg.ll",
    "dummy_source_sink/taint_exception_10_cpp_dbg.ll",
    // -- double-free
    "double_free_01_c.ll",
    "double_free_02_c.ll",
};

INSTANTIATE_TEST_SUITE_P(InteractiveIDESolverTest, TaintAnalysis,
                         ::testing::ValuesIn(TaintTestFiles));

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
