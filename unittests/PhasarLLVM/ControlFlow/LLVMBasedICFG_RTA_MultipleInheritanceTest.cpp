#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/Casting.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

static const llvm::CallBase *getCallInLine(const llvm::Function &F,
                                           uint32_t Line) {
  for (const auto &I : llvm::instructions(F)) {
    const auto *CB = llvm::dyn_cast<llvm::CallBase>(&I);
    if (!CB) {
      continue;
    }

    auto CBLine = getLineFromIR(CB);
    if (CBLine == Line) {
      return CB;
    }
  }
  return nullptr;
}

TEST(LLVMBasedICFG_RTATest, VirtualCallSite_10) {
  // TODO: test if getNonPureVirtualVFTEntry gets the correct inheritance and
  // not just the first one every time

  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/virtual_call_10_cpp_dbg.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH);
  const llvm::Function *MainF = IRDB.getFunctionDefinition("main");
  ASSERT_TRUE(MainF);

  // --- At Line 20: ABptr->bar();

  const auto *CallToBar = getCallInLine(*MainF, 20);
  ASSERT_TRUE(CallToBar);
  const auto *BarF = IRDB.getFunction("_ZThn8_N6ABImpl3barEv");
  ASSERT_TRUE(BarF);

  auto BarCallees = ICFG.getCalleesOfCallAt(CallToBar);
  // non-virtual thunk to ABImpl::bar()
  EXPECT_EQ(llvm::ArrayRef{BarF}, BarCallees);

  // --- At Line 21: delete ABptr;

  const auto *CallToDtor = getCallInLine(*MainF, 21);
  ASSERT_TRUE(CallToDtor);
  const auto *DtorF = IRDB.getFunction("_ZThn8_N6ABImplD0Ev");
  ASSERT_TRUE(DtorF);

  auto DtorCallees = ICFG.getCalleesOfCallAt(CallToDtor);

  // non-virtual thunk to ABImpl::~ABImpl()
  EXPECT_EQ(llvm::ArrayRef{DtorF}, DtorCallees);
}

TEST(LLVMBasedICFG_RTATest, VirtualCallSite_11) {
  // TODO: test if getNonPureVirtualVFTEntry gets the correct inheritance and
  // not just the first one every time

  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/virtual_call_11_cpp_dbg.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH);
  const llvm::Function *CallFooF = IRDB.getFunctionDefinition("_Z7callFooR1A");
  const llvm::Function *CallBarF = IRDB.getFunctionDefinition("_Z7callBarR1B");
  const llvm::Function *CallBazF = IRDB.getFunctionDefinition("_Z7callBazR1C");
  ASSERT_TRUE(CallFooF);
  ASSERT_TRUE(CallBarF);
  ASSERT_TRUE(CallBazF);

  // --- At Line 28:   a.foo();

  const auto *CallToFoo = getCallInLine(*CallFooF, 28);
  ASSERT_TRUE(CallToFoo);
  const auto *FooF = IRDB.getFunction("_ZThn8_N7ABCImpl3fooEv");
  ASSERT_TRUE(FooF);

  auto FooCallees = ICFG.getCalleesOfCallAt(CallToFoo);
  // non-virtual thunk to ABCImpl::foo()
  EXPECT_EQ(llvm::ArrayRef{FooF}, FooCallees);

  // --- At Line 32:   b.bar();

  const auto *CallToBar = getCallInLine(*CallBarF, 32);
  ASSERT_TRUE(CallToBar);
  const auto *BarF = IRDB.getFunction("_ZThn16_N7ABCImpl3barEv");
  ASSERT_TRUE(BarF);

  auto BarCallees = ICFG.getCalleesOfCallAt(CallToBar);
  // non-virtual thunk to ABCImpl::bar()
  EXPECT_EQ(llvm::ArrayRef{BarF}, BarCallees);

  // --- At Line 36:   c.baz();

  const auto *CallToBaz = getCallInLine(*CallBazF, 36);
  ASSERT_TRUE(CallToBaz);
  const auto *BazF = IRDB.getFunction("_ZN7ABCImpl3bazEv");
  ASSERT_TRUE(BazF);

  auto BazCallees = ICFG.getCalleesOfCallAt(CallToBaz);
  // ABCImpl::baz()
  EXPECT_EQ(llvm::ArrayRef{BazF}, BazCallees);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
