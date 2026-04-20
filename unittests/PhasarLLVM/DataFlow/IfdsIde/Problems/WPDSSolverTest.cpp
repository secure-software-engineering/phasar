/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/WPDS/WPDSSolver.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/Utils/Printer.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/IntrinsicInst.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <string_view>

using namespace psr;
using namespace psr::wpds;
using namespace psr::unittest;

namespace {

// ─── Boolean semiring for IFDS-style reachability ────────────────────────────

struct LCAWeight {
  using l_t = typename IDELinearConstantAnalysisDomain::l_t;
  EdgeFunction<l_t> EF = AllTop<l_t>{};

  static LCAWeight zero() noexcept { return {.EF = AllTop<l_t>{}}; }
  static LCAWeight one() noexcept { return {.EF = EdgeIdentity<l_t>{}}; }
  [[nodiscard]] LCAWeight combine(LCAWeight O) const noexcept {
    return {.EF = EF.joinWith(O.EF)};
  }
  [[nodiscard]] LCAWeight extend(LCAWeight O) const noexcept {
    return {.EF = EF.composeWith(O.EF)};
  }
  friend bool operator==(LCAWeight L, LCAWeight R) noexcept = default;
};

// ─── Analysis domain ─────────────────────────────────────────────────────────

struct LCAReachabilityDomain {
  using n_t = const llvm::Instruction *;
  using d_t = const llvm::Value *;
  using w_t = LCAWeight;
  using i_t = LLVMBasedICFG;
};

// ─── WPDSProblem wrapping LCA's IFDS flow functions ──────────────────────────
//
// For returnFlowWeights, the union over all callers is used. This may
// over-approximate context-sensitive return flows, but yields a sound
// superset of the live facts computed by LCA.

class LCAReachabilityProblem {
  using n_t = LCAReachabilityDomain::n_t;
  using d_t = LCAReachabilityDomain::d_t;
  using f_t = const llvm::Function *;
  using Result = llvm::SmallVector<std::pair<d_t, LCAWeight>>;

public:
  LCAReachabilityProblem(IDELinearConstantAnalysis &LCA,
                         const LLVMBasedICFG &ICFG)
      : LCA(LCA), ICFG(ICFG), ZeroFact(LCA.createZeroValue()) {
    for (auto &[Node, Unused] : LCA.initialSeeds().getSeeds()) {
      EntryNodes.push_back(Node);
    }
  }

  Result intraFlowWeights(n_t Src, n_t Dst, d_t D) {
    auto FF = LCA.getNormalFlowFunction(Src, Dst);
    Result Out;
    for (d_t T : FF->computeTargets(D)) {
      Out.emplace_back(
          T, LCAWeight{.EF = LCA.getNormalEdgeFunction(Src, D, Dst, T)});
    }
    return Out;
  }

  Result callFlowWeights(n_t Call, n_t Entry, n_t /*RetSite*/, d_t D) {
    f_t Callee = ICFG.getFunctionOf(Entry);
    auto FF = LCA.getCallFlowFunction(Call, Callee);
    Result Out;
    for (d_t T : FF->computeTargets(D)) {
      Out.emplace_back(
          T, LCAWeight{.EF = LCA.getCallEdgeFunction(Call, D, Callee, T)});
    }
    return Out;
  }

  Result callToReturnFlowWeights(n_t Call, n_t RetSite, d_t D) {
    auto Callees = ICFG.getCalleesOfCallAt(Call);
    auto FF = LCA.getCallToRetFlowFunction(Call, RetSite, Callees);
    Result Out;
    for (d_t T : FF->computeTargets(D)) {
      Out.emplace_back(T, LCAWeight{.EF = LCA.getCallToRetEdgeFunction(
                                        Call, D, RetSite, T, Callees)});
    }
    return Out;
  }

  Result returnFlowWeights(n_t Exit, d_t D) {
    f_t Fun = ICFG.getFunctionOf(Exit);
    llvm::DenseSet<d_t> Seen;
    Result Out;
    for (n_t Caller : ICFG.getCallersOf(Fun)) {
      for (n_t RetSite : ICFG.getSuccsOf(Caller)) {
        auto FF = LCA.getRetFlowFunction(Caller, Fun, Exit, RetSite);
        for (d_t T : FF->computeTargets(D)) {
          if (Seen.insert(T).second) {
            Out.emplace_back(
                T, LCAWeight{.EF = LCA.getReturnEdgeFunction(Caller, Fun, Exit,
                                                             D, RetSite, T)});
          }
        }
      }
    }
    return Out;
  }

  d_t getZeroFact() const noexcept { return ZeroFact; }
  llvm::ArrayRef<n_t> getEntryPoints() const noexcept { return EntryNodes; }
  LCAWeight getInitialWeight(n_t) const noexcept { return LCAWeight::one(); }
  const LLVMBasedICFG &getICFG() const noexcept { return ICFG; }

private:
  IDELinearConstantAnalysis &LCA;
  const LLVMBasedICFG &ICFG;
  d_t ZeroFact;
  llvm::SmallVector<n_t> EntryNodes;
};

// ─── Test fixture
// ─────────────────────────────────────────────────────────────

class WPDSSolverTest : public ::testing::TestWithParam<std::string_view> {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("linear_constant/");

  using l_t = IDELinearConstantAnalysisDomain::l_t;

  void doAnalysis(const llvm::Twine &LlvmFilePath) {
    LLVMProjectIRDB IRDB(PathToLlFiles + LlvmFilePath);
    DIBasedTypeHierarchy TH(IRDB);
    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                       Soundness::Soundy, /*IncludeGlobals=*/true);

    IDELinearConstantAnalysis LCAProblem(&IRDB, &ICFG, {"main"});

    // Ground truth.
    IterativeIDESolver<IDELinearConstantAnalysis> IDESolver(&LCAProblem, &ICFG);
    IDESolver.solve();
    auto IDERes = IDESolver.getSolverResults();

    // WPDS reachability wrapping the same LCA flow functions.
    LCAReachabilityProblem WPDSProb(LCAProblem, ICFG);
    WPDSSolver<LCAReachabilityProblem, LCAReachabilityDomain> WPDSolver(
        WPDSProb);
    WPDSolver.solve();

    // Forward: every (fact, node) live in LCA must be reachable in WPDS.
    for (const auto &[Row, ColVal] : IDERes.getAllResultEntries()) {
      if (llvm::isa<llvm::DbgInfoIntrinsic>(Row)) {
        continue;
      }
      for (auto [Fact, Val] : ColVal) {

        std::optional<LocId> MFact = WPDSolver.factIdOr(Fact);
        std::optional<SymId> MSym = WPDSolver.symIdOr(Row);
        EXPECT_TRUE(MFact.has_value())
            << "WPDS never discovered a fact that LCA considers live";
        EXPECT_TRUE(MSym.has_value())
            << "WPDS never discovered a node that LCA considers live";
        if (MFact && MSym) {
          auto W = WPDSolver.getNodeValue(*MFact, *MSym);
          EXPECT_EQ(W.EF.computeTarget(LCAProblem.bottomElement()), Val)
              << "Value mismatch: " << W.EF << " VS " << Val << "\n> At "
              << NToString(Row) << "; for fact " << DToString(Fact);
        }
      }
    }

    // Reverse: every WPDS-reachable (fact, node) must be live in LCA.
    for (const auto &[Key, W] : WPDSolver.getNodeValueMap()) {
      auto [LId, SId] = Key;
      auto *Fact = WPDSolver.factAt(LId);
      auto *Node = WPDSolver.nodeAt(SId);
      if (LCAProblem.isZeroValue(Fact)) {
        continue; // zero fact is internal bookkeeping
      }
      if (llvm::isa<llvm::DbgInfoIntrinsic>(Node)) {
        continue;
      }
      EXPECT_NE(IDERes.resultAt(Node, Fact), LCAProblem.topElement())
          << "WPDS reports reachable but LCA says Top (not live): "
          << NToString(Node) << ", " << DToString(Fact);
    }
  }
};

// ─── Parameterized test
// ───────────────────────────────────────────────────────

TEST_P(WPDSSolverTest, LCAReachabilityMatchesIterativeIDESolver) {
  doAnalysis(GetParam());
}

static constexpr std::string_view LCATestFiles[] = {
    "basic_01_cpp_dbg.ll",
    "basic_02_cpp_dbg.ll",
    "basic_03_cpp_dbg.ll",
    "basic_04_cpp_dbg.ll",
    "basic_05_cpp_dbg.ll",
    "basic_06_cpp_dbg.ll",
    "basic_07_cpp_dbg.ll",
    "basic_08_cpp_dbg.ll",
    "basic_09_cpp_dbg.ll",
    "basic_10_cpp_dbg.ll",
    "basic_11_cpp_dbg.ll",
    "basic_12_cpp_dbg.ll",

    "branch_01_cpp_dbg.ll",
    "branch_02_cpp_dbg.ll",
    "branch_03_cpp_dbg.ll",
    "branch_04_cpp_dbg.ll",
    "branch_05_cpp_dbg.ll",
    "branch_06_cpp_dbg.ll",
    "branch_07_cpp_dbg.ll",

    "while_01_cpp_dbg.ll",
    "while_02_cpp_dbg.ll",
    "while_03_cpp_dbg.ll",
    "while_04_cpp_dbg.ll",
    "while_05_cpp_dbg.ll",
    "for_01_cpp_dbg.ll",

    "call_01_cpp_dbg.ll",
    "call_02_cpp_dbg.ll",
    "call_03_cpp_dbg.ll",
    "call_04_cpp_dbg.ll",
    "call_05_cpp_dbg.ll",
    "call_06_cpp_dbg.ll",
    "call_07_cpp_dbg.ll",
    "call_08_cpp_dbg.ll",
    "call_09_cpp_dbg.ll",
    "call_10_cpp_dbg.ll",
    "call_11_cpp_dbg.ll",

    "recursion_01_cpp_dbg.ll",
    "recursion_02_cpp_dbg.ll",
    "recursion_03_cpp_dbg.ll",

    "global_01_cpp_dbg.ll",
    "global_02_cpp_dbg.ll",
    "global_03_cpp_dbg.ll",
    "global_04_cpp_dbg.ll",
    "global_05_cpp_dbg.ll",
    "global_06_cpp_dbg.ll",
    "global_07_cpp_dbg.ll",
    "global_08_cpp_dbg.ll",
    "global_09_cpp_dbg.ll",
    "global_10_cpp_dbg.ll",
    "global_11_cpp_dbg.ll",
    "global_12_cpp_dbg.ll",
    "global_13_cpp_dbg.ll",
    "global_14_cpp_dbg.ll",
    "global_15_cpp_dbg.ll",
    "global_16_cpp_dbg.ll",

    "overflow_add_cpp_dbg.ll",
    "overflow_sub_cpp_dbg.ll",
    "overflow_mul_cpp_dbg.ll",
    "overflow_div_min_by_neg_one_cpp_dbg.ll",

    "ub_division_by_zero_cpp_dbg.ll",
    "ub_modulo_by_zero_cpp_dbg.ll",
    "external_fun_cpp.ll",
};

INSTANTIATE_TEST_SUITE_P(WPDSSolverTest, WPDSSolverTest,
                         ::testing::ValuesIn(LCATestFiles));

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
