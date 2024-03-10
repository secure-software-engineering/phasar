#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionVerifier.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEInstInteractionAnalysis.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/JoinLatticeVerifier.h"

#include "llvm/Support/ErrorHandling.h"

#include "gtest/gtest.h"

#include <cstdint>

using e_t = int;
using l_t = psr::IDEInstInteractionAnalysisDomain<e_t>::l_t;
using set_t = psr::BitVectorSet<e_t>;
using IIAAAddLabelsEF = psr::IDEInstInteractionAnalysisT<e_t>::IIAAAddLabelsEF;
using IIAAKillOrReplaceEF =
    psr::IDEInstInteractionAnalysisT<e_t>::IIAAKillOrReplaceEF;

TEST(IDEInstInteractionAnalysisLatticeTest, ValueLattice) {
  std::vector<l_t> Vals{set_t{1}, set_t{2}, set_t{3},   set_t{4},
                        set_t{5}, set_t{6}, psr::Top{}, psr::Bottom{}};

  struct Hash {
    size_t operator()(const l_t &Val) const noexcept {
      if (Val.isBottom()) {
        return INT64_MAX;
      }
      if (Val.isTop()) {
        return INT64_MIN;
      }

      return hash_value(Val.assertGetValue());
    }
  };

  EXPECT_TRUE(psr::validateBoundedJoinLattice<l_t>(Vals, llvm::errs(), Hash{}));
}

TEST(IDEInstInteractionAnalysisLatticeTest, EdgeFunctionLattice) {
  psr::DefaultEdgeFunctionSingletonCache<IIAAAddLabelsEF> IIAAAddLabelsEFCache;
  psr::DefaultEdgeFunctionSingletonCache<IIAAKillOrReplaceEF>
      IIAAKillOrReplaceEFCache;

  std::vector<l_t> Vals{set_t{1}, set_t{2}, set_t{3},   set_t{4},
                        set_t{5}, set_t{6}, psr::Top{}, psr::Bottom{}};
  std::vector<psr::EdgeFunction<l_t>> EFs;
  EFs.reserve(2 * Vals.size());
  EFs.emplace_back(psr::AllTop<l_t>{});
  EFs.emplace_back(psr::AllBottom<l_t>{});
  EFs.emplace_back(psr::EdgeIdentity<l_t>{});

  for (const auto &Val : Vals) {
    if (Val.isBottom() || Val.isTop()) {
      continue;
    }

    EFs.push_back(IIAAAddLabelsEFCache.createEdgeFunction(Val));
    EFs.push_back(IIAAKillOrReplaceEFCache.createEdgeFunction(Val));
  }

  struct Hash {
    size_t operator()(const psr::EdgeFunction<l_t> &Val) const noexcept {
      if (const auto *AddLab = llvm::dyn_cast<IIAAAddLabelsEF>(Val)) {
        return 97 * hash_value(AddLab->Data);
      }
      if (const auto *KillOrRep = llvm::dyn_cast<IIAAKillOrReplaceEF>(Val)) {
        return 51 * hash_value(KillOrRep->Replacement);
      }

      if (llvm::isa<psr::AllBottom<l_t>>(Val)) {
        return INT64_MAX;
      }
      if (llvm::isa<psr::AllTop<l_t>>(Val)) {
        return INT64_MIN;
      }
      if (llvm::isa<psr::EdgeIdentity<l_t>>(Val)) {
        return 0;
      }

      llvm::report_fatal_error("All EFs should be handled already (excluding "
                               "ComposeEF, that is not generated (yet)): " +
                               llvm::Twine(to_string(Val)));
    }
  };

  EXPECT_TRUE(
      psr::validateEdgeFunctionLattice<l_t>(EFs, Vals, llvm::errs(), Hash{}));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
