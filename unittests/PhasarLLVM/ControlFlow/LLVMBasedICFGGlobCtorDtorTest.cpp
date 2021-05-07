#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

static const std::string PathToLLFiles =
    unittest::PathToLLTestFiles + "globals/";

TEST(LLVMBasedICFGGlobCtorDtorTest, CtorTest) {
  boost::log::core::get()->set_logging_enabled(false);
  ValueAnnotationPass::resetValueID();

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

  auto PrevOfFirstMainInst = ICFG.getPredsOf(FirstMainInst);
  ASSERT_EQ(1, PrevOfFirstMainInst.size());
  EXPECT_EQ(SecondCtorInst, *PrevOfFirstMainInst.begin());
}

TEST(LLVMBasedICFGGlobCtorDtorTest, CtorTest2) {
  boost::log::core::get()->set_logging_enabled(false);
  ValueAnnotationPass::resetValueID();

  ProjectIRDB IRDB({PathToLLFiles + "globals_ctor_2_1_cpp.ll",
                    PathToLLFiles + "globals_ctor_2_2_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::SOUNDY, /*IncludeGlobals*/ true);

  auto *GlobalCtor = ICFG.getFirstGlobalCtorOrNull();
  EXPECT_TRUE(GlobalCtor != nullptr);

  // IRDB.print();

  auto *FirstGlobCtor = IRDB.getFunction("_GLOBAL__sub_I_globals_ctor_2_1.cpp");
  auto *SecondGlobCtor =
      IRDB.getFunction("_GLOBAL__sub_I_globals_ctor_2_2.cpp");

  auto *LastCtor1Inst = &FirstGlobCtor->back().back();
  auto *LastCtor2Inst = &SecondGlobCtor->back().back();

  auto *FirstCtor1Inst = &FirstGlobCtor->front().front();
  auto *FirstCtor2Inst = &SecondGlobCtor->front().front();
  auto *FirstMainInst = &IRDB.getFunction("main")->front().front();

  auto _NextOfLastCtor1Inst = ICFG.getSuccsOf(LastCtor1Inst);
  auto _NextOfLastCtor2Inst = ICFG.getSuccsOf(LastCtor2Inst);

  auto _PrevOfFirstMainInst = ICFG.getPredsOf(FirstMainInst);
  auto _PrevOfFirstCtor1Inst = ICFG.getPredsOf(FirstCtor1Inst);
  auto _PrevOfFirstCtor2Inst = ICFG.getPredsOf(FirstCtor2Inst);

  ASSERT_EQ(1, _NextOfLastCtor1Inst.size());
  ASSERT_EQ(1, _NextOfLastCtor2Inst.size());
  ASSERT_EQ(1, _PrevOfFirstMainInst.size());
  ASSERT_EQ(1, _PrevOfFirstCtor1Inst.size() + _PrevOfFirstCtor2Inst.size());

  auto *NextOfLastCtor1Inst = *_NextOfLastCtor1Inst.begin();
  auto *NextOfLastCtor2Inst = *_NextOfLastCtor2Inst.begin();

  // Ctor1 and Ctor2 have the same priority, so their order is undefined

  EXPECT_TRUE(NextOfLastCtor1Inst == FirstCtor2Inst ||
              NextOfLastCtor1Inst == FirstMainInst);
  EXPECT_TRUE(NextOfLastCtor2Inst == FirstCtor1Inst ||
              NextOfLastCtor2Inst == FirstMainInst);

  if (NextOfLastCtor1Inst == FirstCtor2Inst) {
    // Ctor1 comes before Ctor2
    EXPECT_EQ(FirstMainInst, NextOfLastCtor2Inst);

    EXPECT_EQ(LastCtor2Inst, *_PrevOfFirstMainInst.begin());
    ASSERT_EQ(1, _PrevOfFirstCtor2Inst.size());
    EXPECT_EQ(LastCtor1Inst, *_PrevOfFirstCtor2Inst.begin());

  } else { // Ctor2 comes before Ctor1
    EXPECT_EQ(FirstCtor1Inst, NextOfLastCtor2Inst);

    EXPECT_EQ(LastCtor1Inst, *_PrevOfFirstMainInst.begin());
    ASSERT_EQ(1, _PrevOfFirstCtor1Inst.size());
    EXPECT_EQ(LastCtor2Inst, *_PrevOfFirstCtor1Inst.begin());
  }
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
