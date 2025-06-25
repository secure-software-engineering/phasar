
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/SVF/SVFPointsToSet.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"

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
  EXPECT_NE(AliasResult::NoAlias, AS.alias(V, Alias));
}

TEST(SVFAliasSetTest, Alias_02) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/basic_01_cpp_dbg.ll");

  LLVMAliasSet AS(&IRDB, false, AliasAnalysisType::SVFDDA);

  const auto *V = IRDB.getInstruction(5);
  ASSERT_TRUE(V && V->getType()->isPointerTy());

  const auto *Alias = IRDB.getInstruction(0);

  auto ASet = AS.getAliasSet(V);
  LLVMAliasSet::AliasSetTy GroundTruth = {V, Alias};
  EXPECT_EQ(GroundTruth, *ASet);
  EXPECT_NE(AliasResult::NoAlias, AS.alias(V, Alias));
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

  auto AllocObjs = PT.getPointsToSet(Alloc, Alloc->getNextNode());
  ASSERT_EQ(1, AllocObjs.size());
  auto AllocObj = *AllocObjs.begin();
  EXPECT_TRUE(PT.mayPointsTo(V, AllocObj, V->getNextNode()));
}

TEST(SVFAliasSetTest, PointsTo_02) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/basic_01_cpp_dbg.ll");

  auto PT = createSVFDDAPointsToInfo(IRDB);

  const auto *V = IRDB.getInstruction(5);
  ASSERT_TRUE(V && V->getType()->isPointerTy());

  const auto *Alloc = IRDB.getInstruction(0);

  auto PSet = PT.getPointsToSet(V, V->getNextNode());
  ASSERT_EQ(1, PSet.size());
  EXPECT_EQ(Alloc, PT.asPointerOrNull(*PSet.begin()));

  auto AllocObjs = PT.getPointsToSet(Alloc, Alloc->getNextNode());
  ASSERT_EQ(1, AllocObjs.size());
  auto AllocObj = *AllocObjs.begin();
  EXPECT_TRUE(PT.mayPointsTo(V, AllocObj, V->getNextNode()));
}

TEST(SVFAliasSetTest, PointsTo_03) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/basic_01_cpp_dbg.ll");

  auto PT = createLLVMSVFPointsToIterator(IRDB, SVFPointsToAnalysisType::VFS);

  const auto *V = IRDB.getInstruction(5);
  ASSERT_TRUE(V && V->getType()->isPointerTy());

  const auto *Alloc = IRDB.getInstruction(0);

  auto PSet = PT.asSet(V, V->getNextNode());
  ASSERT_EQ(1, PSet.size());
  EXPECT_EQ(Alloc, *PSet.begin());

  auto AllocObjs = PT.asSet(Alloc, Alloc->getNextNode());
  ASSERT_EQ(1, AllocObjs.size());
  const auto *AllocObj = *AllocObjs.begin();
  EXPECT_TRUE(PT.mayPointsTo(V, AllocObj, V->getNextNode()));
}

TEST(SVFAliasSetTest, PointsTo_04) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/call_01_cpp_dbg.ll");

  auto PT = createLLVMSVFPointsToIterator(IRDB, SVFPointsToAnalysisType::DDA);

  const auto *V = IRDB.getInstruction(3);
  ASSERT_TRUE(V && V->getType()->isPointerTy());

  auto PSet = PT.asSet(V, V->getNextNode());
  llvm::errs() << "PointsToSet of " << llvmIRToString(V) << ":\n";
  for (const auto *Pt : PSet) {
    llvm::errs() << "  --> " << llvmIRToString(Pt) << '\n';
  }
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
