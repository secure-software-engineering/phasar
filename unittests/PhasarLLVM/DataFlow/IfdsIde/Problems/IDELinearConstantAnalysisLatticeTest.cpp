#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/Utils/JoinLatticeVerifier.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <functional>

using l_t = psr::IDELinearConstantAnalysisDomain::l_t;

TEST(IDELinearConstantAnalysisLatticeTest, Lattice) {
  std::vector<l_t> Vals{
      1,  2,  3,  4, 5,  6,  7,  8,  9,   10,         -1,
      -2, -3, -4, 5, -6, -7, -8, -9, -10, psr::Top{}, psr::Bottom{}};

  struct Hash {
    size_t operator()(const l_t &Val) const noexcept {
      if (Val.isBottom()) {
        return INT64_MAX;
      }
      if (Val.isTop()) {
        return INT64_MIN;
      }

      return std::hash<int64_t>{}(Val.assertGetValue());
    }
  };

  EXPECT_TRUE(psr::validateBoundedJoinLattice<l_t>(Vals, llvm::errs(), Hash{}));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
