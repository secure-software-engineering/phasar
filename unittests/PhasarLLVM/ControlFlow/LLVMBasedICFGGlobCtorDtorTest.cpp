#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

static const std::string PathToLLFiles =
    unittest::PathToLLTestFiles + "globals/";

TEST(LLVMBasedICFGGlobCtorDtorTest, CtorTest) {
  ProjectIRDB IRDB({PathToLLFiles + "globals_ctor_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::SOUNDY, /*IncludeGlobals*/ true);

  auto *GlobalCtor = ICFG.getFirstGlobalCtorOrNull();
  EXPECT_TRUE(GlobalCtor != nullptr);

  auto *ExpectedGlobalCtor =
      IRDB.getFunction("_GLOBAL__sub_I_globals_ctor_1.cpp");

  EXPECT_EQ(ExpectedGlobalCtor, GlobalCtor);

  auto *FirstCtorInst = &ExpectedGlobalCtor->front().front();
  auto *SecondCtorInst = FirstCtorInst->getNextNode();
  auto *MainFn = IRDB.getFunction("main");
  auto *FirstMainInst = &MainFn->front().front();

  auto NextOfFirstCtorInst = ICFG.getSuccsOf(FirstCtorInst);
  ASSERT_EQ(1, NextOfFirstCtorInst.size());
  EXPECT_EQ(SecondCtorInst, *NextOfFirstCtorInst.begin());

  auto NextOfSecondCtorInst = ICFG.getSuccsOf(SecondCtorInst);
  ASSERT_EQ(1, NextOfSecondCtorInst.size());
  EXPECT_EQ(FirstMainInst, *NextOfSecondCtorInst.begin());
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
