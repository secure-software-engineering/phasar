
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/SVF/SVFPointsToSet.h"
#include "phasar/Pointer/AliasAnalysisType.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

TEST(SVFAliasSetTest, Alias_01) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/basic_01_cpp_dbg.ll");

  LLVMAliasSet AS(&IRDB, false, AliasAnalysisType::SVFVFS);

  const auto *V = IRDB.getInstruction(5);
  ASSERT_TRUE(V && V->getType()->isPointerTy());

  const auto *Alias = IRDB.getInstruction(0);

  auto ASet = AS.getAliasSet(V);
  LLVMAliasSet::AliasSetTy GroundTruth = {V, Alias};
  EXPECT_EQ(GroundTruth, *ASet);
}

TEST(SVFAliasSetTest, PointsTo_01) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/basic_01_cpp_dbg.ll");

  auto PT = createSVFVFSPointsToInfo(IRDB);

  const auto *V = IRDB.getInstruction(5);
  ASSERT_TRUE(V && V->getType()->isPointerTy());

  const auto *Alloc = IRDB.getInstruction(0);

  auto PSet = PT.getPointsToSet(V, V->getNextNode());
  ASSERT_EQ(1, PSet.size());
  EXPECT_EQ(Alloc, PT.asPointerOrNull(*PSet.begin()));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
