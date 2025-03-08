#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IDEAliasInfoTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IDENoAliasInfoTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

namespace {

struct DFFAnalysisDomain : public psr::LLVMAnalysisDomainDefault {
  using l_t = BinaryDomain; // required
};

class IDEAliasImpl : public IDEAliasInfoTabulationProblem<DFFAnalysisDomain> {
public:
  IDEAliasImpl(LLVMProjectIRDB *IRDB)
      : psr::IDEAliasInfoTabulationProblem<DFFAnalysisDomain>(IRDB, &PT, {},
                                                              {}),
        PT(IRDB){};
  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return {};
  };
  EdgeFunction<l_t> getNormalEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                          n_t /*Succ*/,
                                          d_t /*SuccNode*/) override {
    return {};
  };

  EdgeFunction<l_t> getCallEdgeFunction(n_t /*CallInst*/, d_t /*SrcNode*/,
                                        f_t /*CalleeFun*/,
                                        d_t /*DestNode*/) override {
    return {};
  };

  EdgeFunction<l_t> getReturnEdgeFunction(n_t /*CallSite*/, f_t /*CalleeFun*/,
                                          n_t /*ExitInst*/, d_t /*ExitNode*/,
                                          n_t /*RetSite*/,
                                          d_t /*RetNode*/) override {
    return {};
  };

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/,
                           d_t /*RetSiteNode*/,
                           llvm::ArrayRef<f_t> /*Callees*/) override {
    return {};
  }

private:
  LLVMAliasSet PT;
};

class IDENoAliasImpl
    : public IDENoAliasInfoTabulationProblem<DFFAnalysisDomain> {
public:
  IDENoAliasImpl(LLVMProjectIRDB *IRDB)
      : psr::IDENoAliasInfoTabulationProblem<DFFAnalysisDomain>(IRDB, {}, {}){};
  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return {};
  };
  EdgeFunction<l_t> getNormalEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                          n_t /*Succ*/,
                                          d_t /*SuccNode*/) override {
    return {};
  };

  EdgeFunction<l_t> getCallEdgeFunction(n_t /*CallInst*/, d_t /*SrcNode*/,
                                        f_t /*CalleeFun*/,
                                        d_t /*DestNode*/) override {
    return {};
  };

  EdgeFunction<l_t> getReturnEdgeFunction(n_t /*CallSite*/, f_t /*CalleeFun*/,
                                          n_t /*ExitInst*/, d_t /*ExitNode*/,
                                          n_t /*RetSite*/,
                                          d_t /*RetNode*/) override {
    return {};
  };

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/,
                           d_t /*RetSiteNode*/,
                           llvm::ArrayRef<f_t> /*Callees*/) override {
    return {};
  }
};

void printValueSet(const std::set<const llvm::Value *> &Values) {
  llvm::outs() << "\nValue Set\n";
  for (const auto *CurrValue : Values) {
    llvm::outs() << "  - ";
    if (CurrValue) {
      llvm::outs() << *CurrValue << "\n";
    } else {
      llvm::outs() << "Was null\n";
    }
  }
  llvm::outs() << "\nAfter Value Set loop\n";
}

std::set<const llvm::Value *>
getNormalFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                      const llvm::Value *Arg) {
  const auto AliasNormalFlowFunc =
      AliasImpl.getNormalFlowFunction(Instr, nullptr);
  const auto AliasLLVMValueSet = AliasNormalFlowFunc->computeTargets(Arg);
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getNormalFlowValueSet(const llvm::Instruction *Instr,
                      IDENoAliasImpl &NoAliasImpl, const llvm::Value *Arg) {
  const auto NoAliasNormalFlowFunc =
      NoAliasImpl.getNormalFlowFunction(Instr, nullptr);
  const auto NoAliasLLVMValueSet = NoAliasNormalFlowFunc->computeTargets(Arg);
  // printValueSet(NoAliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  const auto AliasCallFlowFunc =
      AliasImpl.getCallFlowFunction(Instr, CalleeFunc);
  std::set<const llvm::Value *> AliasLLVMValueSet =
      AliasCallFlowFunc->computeTargets(Arg);
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr, IDENoAliasImpl &NoAliasImpl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  const auto NoAliasCallFlowFunc =
      NoAliasImpl.getCallFlowFunction(Instr, CalleeFunc);
  std::set<const llvm::Value *> NoAliasLLVMValueSet =
      NoAliasCallFlowFunc->computeTargets(Arg);
  // printValueSet(AliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  const auto AliasRetFlowFunc =
      AliasImpl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr);
  std::set<const llvm::Value *> AliasLLVMValueSet =
      AliasRetFlowFunc->computeTargets(Arg);
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr, IDENoAliasImpl &NoAliasImpl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  const auto NoAliasRetFlowFunc =
      NoAliasImpl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr);
  std::set<const llvm::Value *> NoAliasLLVMValueSet =
      NoAliasRetFlowFunc->computeTargets(Arg);
  // printValueSet(NoAliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr,
                         IDEAliasImpl &AliasImpl, const LLVMProjectIRDB &IRDB) {
  // Second and third argument of getCallToRetFlowFunction are unused, that's
  // why we can just set them to nullptr or an empty set without issue
  const auto AliasCallToRetFlowFunc =
      AliasImpl.getCallToRetFlowFunction(Instr, nullptr, {});
  const auto AliasLLVMValueSet = AliasCallToRetFlowFunc->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr,
                         IDENoAliasImpl &NoAliasImpl,
                         const LLVMProjectIRDB &IRDB) {
  // Second and third argument of getCallToRetFlowFunction are unused, that's
  // why we can just set them to nullptr or an empty set without issue
  const auto NoAliasCallToRetFlowFunc =
      NoAliasImpl.getCallToRetFlowFunction(Instr, nullptr, {});
  const auto NoAliasLLVMValueSet = NoAliasCallToRetFlowFunc->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  // printValueSet(NoAliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}

/*
  TODO: go over ground truths with Fabian, to be extra sure. Also, go over all
  TODOs, if not resolved already.
*/

/*
  TODO: add normal flow function test for not just store instr but also for the
  other stuff like unaryop etc
*/
TEST(PureFlow, NormalFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  // IRDB.emitPreprocessedIR(llvm::outs());

  // store i32 0, ptr %retval, align 4
  const auto *Instr7 = IRDB.getInstruction(7);
  ASSERT_TRUE(Instr7);
  EXPECT_EQ(std::set<const llvm::Value *>{Instr7},
            getNormalFlowValueSet(Instr7, AliasImpl, Instr7));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr7},
            getNormalFlowValueSet(Instr7, NoAliasImpl, Instr7));

  // store i32 %0, ptr %.addr, align 4
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);
  EXPECT_EQ(std::set<const llvm::Value *>{Instr8},
            getNormalFlowValueSet(Instr8, AliasImpl, Instr8));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr8},
            getNormalFlowValueSet(Instr8, NoAliasImpl, Instr8));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr7},
            getNormalFlowValueSet(Instr8, AliasImpl, Instr7));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr7},
            getNormalFlowValueSet(Instr8, NoAliasImpl, Instr7));

  // store ptr %1, ptr %.addr1, align 8
  const auto *Instr10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instr10);
  EXPECT_EQ(std::set<const llvm::Value *>{Instr10},
            getNormalFlowValueSet(Instr10, AliasImpl, Instr10));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr10},
            getNormalFlowValueSet(Instr10, NoAliasImpl, Instr10));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr8},
            getNormalFlowValueSet(Instr10, AliasImpl, Instr8));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr8},
            getNormalFlowValueSet(Instr10, NoAliasImpl, Instr8));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr7},
            getNormalFlowValueSet(Instr10, AliasImpl, Instr7));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr7},
            getNormalFlowValueSet(Instr10, NoAliasImpl, Instr7));

  // store i32 1, ptr %One, align 4, !dbg !220
  const auto *Instr13 = IRDB.getInstruction(13);
  ASSERT_TRUE(Instr13);
  EXPECT_EQ(std::set<const llvm::Value *>{Instr13},
            getNormalFlowValueSet(Instr13, AliasImpl, Instr13));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr13},
            getNormalFlowValueSet(Instr13, NoAliasImpl, Instr13));

  // store i32 2, ptr %Two, align 4, !dbg !222
  const auto *Instr15 = IRDB.getInstruction(15);
  ASSERT_TRUE(Instr15);
  EXPECT_EQ(std::set<const llvm::Value *>{Instr15},
            getNormalFlowValueSet(Instr15, AliasImpl, Instr15));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr15},
            getNormalFlowValueSet(Instr15, NoAliasImpl, Instr15));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr13},
            getNormalFlowValueSet(Instr15, AliasImpl, Instr13));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr13},
            getNormalFlowValueSet(Instr15, NoAliasImpl, Instr13));

  // store ptr %One, ptr %OnePtr, align 8, !dbg !225
  const auto *Instr17 = IRDB.getInstruction(17);
  ASSERT_TRUE(Instr17);
  // TODO: determine ground truth. How are pointers progapated properly?
  EXPECT_EQ(std::set<const llvm::Value *>{Instr17},
            getNormalFlowValueSet(Instr17, AliasImpl, Instr17));
  // TODO: determine ground truth. How are pointers progapated properly?
  EXPECT_EQ(std::set<const llvm::Value *>{Instr17},
            getNormalFlowValueSet(Instr17, NoAliasImpl, Instr17));

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !228
  const auto *Instr19 = IRDB.getInstruction(19);
  ASSERT_TRUE(Instr19);
  // TODO: determine ground truth.  How are addresses progapated properly?
  EXPECT_EQ(std::set<const llvm::Value *>{Instr19},
            getNormalFlowValueSet(Instr19, AliasImpl, Instr19));
  // TODO: determine ground truth. How are addresses progapated properly?
  EXPECT_EQ(std::set<const llvm::Value *>{Instr19},
            getNormalFlowValueSet(Instr19, NoAliasImpl, Instr19));

  // TODO: fix negative tests. These fail, but I do not know why. Instr8 should
  // not be propagated at Instr7, because it doesn't exist then, right?
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr7, AliasImpl, Instr8));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr7, NoAliasImpl, Instr8));
}

TEST(PureFlow, NormalFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());
  // store i32 0, ptr %retval, align 4, !psr.id !226; | ID: 7

  // store i32 %0, ptr %.addr, align 4, !psr.id !227; | ID: 8

  // store ptr %1, ptr %.addr1, align 8, !psr.id !231; | ID: 10

  // store i32 1, ptr %One, align 4, !dbg !236, !psr.id !238; | ID: 13

  // store i32 2, ptr %Two, align 4, !dbg !241, !psr.id !243; | ID: 15

  // store i32 3, ptr %Three, align 4, !dbg !247, !psr.id !249; | ID: 18
}

TEST(PureFlow, NormalFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
}

/*
  CallFlow
*/

TEST(PureFlow, CallFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  // IRDB.emitPreprocessedIR(llvm::outs());

  // %Two = alloca i32, align 4, !psr.id !235; | ID: 12
  const auto *UnusedValue = IRDB.getValueFromId(12);

  // call void @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg !261, !psr.id
  // !262; | ID: 26
  const auto *Instr26 = IRDB.getInstruction(26);
  ASSERT_TRUE(Instr26);
  const auto *FuncForInstr26 = IRDB.getFunction("_Z4callii");

  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Instr26)) {
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr26->getArg(0)},
              getCallFlowValueSet(Instr26, AliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr26->getArg(1)},
              getCallFlowValueSet(Instr26, AliasImpl,
                                  CallSite->getArgOperand(1), FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr26->getArg(0)},
              getCallFlowValueSet(Instr26, NoAliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr26->getArg(1)},
              getCallFlowValueSet(Instr26, NoAliasImpl,
                                  CallSite->getArgOperand(1), FuncForInstr26));
  } else {
    FAIL();
  }

  // negative tests
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr26, NoAliasImpl, UnusedValue, FuncForInstr26));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr26, NoAliasImpl, UnusedValue, FuncForInstr26));
}

TEST(PureFlow, CallFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  // IRDB.emitPreprocessedIR(llvm::outs());

  // %Zero = alloca i32, align 4, !psr.id !261; | ID: 19
  const auto *UnusedValue = IRDB.getValueFromId(19);

  // %call = call noundef i32 @_Z6getOnev(), !dbg !282, !psr.id !283; | ID: 32
  const auto *Instr32 = IRDB.getInstruction(32);
  ASSERT_TRUE(Instr32);
  const auto *FuncForInstr32 = IRDB.getFunction("_Z6getOnev");

  // function has no arg, therefore any value can be used for arg
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr32, AliasImpl, UnusedValue, FuncForInstr32));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr32, NoAliasImpl, UnusedValue, FuncForInstr32));

  // %call2 = call noundef i32 @_Z6getTwoi(i32 noundef %2), !dbg !290, !psr.id
  // !291; | ID: 36
  const auto *Instr36 = IRDB.getInstruction(36);
  ASSERT_TRUE(Instr36);
  const auto *FuncForInstr36 = IRDB.getFunction("_Z6getTwoi");
  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Instr36)) {
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr36->getArg(0)},
              getCallFlowValueSet(Instr36, AliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr36));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr36->getArg(0)},
              getCallFlowValueSet(Instr36, NoAliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr36));
  } else {
    FAIL();
  }

  // %call3 = call noundef i32 @_Z8getThreePKi(ptr noundef %Two), !dbg !296,
  // !psr.id !297; | ID: 39
  const auto *Instr39 = IRDB.getInstruction(39);
  ASSERT_TRUE(Instr39);
  const auto *FuncForInstr39 = IRDB.getFunction("_Z8getThreePKi");
  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Instr39)) {
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr39->getArg(0)},
              getCallFlowValueSet(Instr39, AliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr39));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr39->getArg(0)},
              getCallFlowValueSet(Instr39, NoAliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr39));
  } else {
    FAIL();
  }

  // %call4 = call noundef ptr @_Z18getPtrToGlobalFourv(), !dbg !302, !psr.id
  // !303; | ID: 42
  const auto *Instr42 = IRDB.getInstruction(42);
  ASSERT_TRUE(Instr42);
  // TODO: go over ground truth with Fabian!
  const auto *FuncForInstr42 = IRDB.getFunction("_Z18getPtrToGlobalFourv");
  if (FuncForInstr42) {
    EXPECT_EQ(
        std::set<const llvm::Value *>{},
        getCallFlowValueSet(Instr42, AliasImpl, UnusedValue, FuncForInstr42));
    EXPECT_EQ(
        std::set<const llvm::Value *>{},
        getCallFlowValueSet(Instr42, NoAliasImpl, UnusedValue, FuncForInstr42));
  } else {
    FAIL();
  }

  // negative tests
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr36, AliasImpl, UnusedValue, FuncForInstr36));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr36, NoAliasImpl, UnusedValue, FuncForInstr36));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr39, AliasImpl, UnusedValue, FuncForInstr39));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getCallFlowValueSet(Instr39, NoAliasImpl, UnusedValue, FuncForInstr39));
}

/*
  RetFlow
*/

TEST(PureFlow, RetFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  // IRDB.emitPreprocessedIR(llvm::outs());

  // %Two = alloca i32, align 4, !psr.id !251; | ID: 20
  const auto *UnusedValue = IRDB.getValueFromId(20);

  // %call = call noundef i32 @_Z6getTwov(), !dbg !231, !psr.id !232; | ID: 9
  const auto *Instr9 = IRDB.getInstruction(9);
  ASSERT_TRUE(Instr9);
  // ret i32 2, !dbg !212, !psr.id !213; | ID: 0
  const auto *Instr0 = IRDB.getInstruction(0);
  ASSERT_TRUE(Instr0);

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr9, AliasImpl, UnusedValue, Instr0));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr9, NoAliasImpl, UnusedValue, Instr0));

  // %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  // !281, !psr.id !282; | ID: 36
  const auto *Instr36 = IRDB.getInstruction(36);
  ASSERT_TRUE(Instr36);
  // ret i32 %add, !dbg !240, !psr.id !241; | ID: 14
  const auto *Instr14 = IRDB.getInstruction(14);
  ASSERT_TRUE(Instr14);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");

  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr36, AliasImpl, FuncZ4callii->getArg(0), Instr14));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr36, AliasImpl, FuncZ4callii->getArg(1), Instr14));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr36, NoAliasImpl, FuncZ4callii->getArg(0),
                               Instr14));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr36, NoAliasImpl, FuncZ4callii->getArg(1),
                               Instr14));

  // negative test
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr9, NoAliasImpl, FuncZ4callii->getArg(1), Instr0));
}

TEST(PureFlow, RetFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  // IRDB.emitPreprocessedIR(llvm::outs());

  // %Two = alloca i32, align 4, !psr.id !268; | ID: 29
  const auto *UnusedValue = IRDB.getValueFromId(29);

  // %call = call noundef i32 @_Z6getTwov(), !dbg !240, !psr.id !241; | ID: 14
  const auto *Instr14 = IRDB.getInstruction(14);
  ASSERT_TRUE(Instr14);
  // ret i32 2, !dbg !212, !psr.id !213; | ID: 0
  const auto *Instr0 = IRDB.getInstruction(0);
  ASSERT_TRUE(Instr0);

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr14, AliasImpl, UnusedValue, Instr0));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr14, NoAliasImpl, UnusedValue, Instr0));

  // %call1 = call noundef i32 @_Z8newThreev(), !dbg !247, !psr.id !248; | ID:
  // 18
  const auto *Instr18 = IRDB.getInstruction(18);
  ASSERT_TRUE(Instr18);
  // ret i32 3, !dbg !220, !psr.id !221; | ID: 4
  const auto *Instr4 = IRDB.getInstruction(4);
  ASSERT_TRUE(Instr4);

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr18, AliasImpl, UnusedValue, Instr4));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr18, NoAliasImpl, UnusedValue, Instr4));

  //  %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  //  !298, !psr.id !299; | ID: 45
  const auto *Instr45 = IRDB.getInstruction(45);
  ASSERT_TRUE(Instr45);
  // ret i32 %add, !dbg !257, !psr.id !258; | ID: 23
  const auto *Instr23 = IRDB.getInstruction(23);
  ASSERT_TRUE(Instr23);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");

  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr45, AliasImpl, FuncZ4callii->getArg(0), Instr23));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr45, AliasImpl, FuncZ4callii->getArg(1), Instr23));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr45, NoAliasImpl, FuncZ4callii->getArg(0),
                               Instr23));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr45, NoAliasImpl, FuncZ4callii->getArg(1),
                               Instr23));
}

TEST(PureFlow, RetFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());

  // %Two = alloca i32, align 4, !psr.id !326; | ID: 54
  const auto *UnusedValue = IRDB.getValueFromId(54);

  // ret ptr %1, !dbg !234, !psr.id !235; | ID: 9
  const auto *Instruction9 = IRDB.getInstruction(9);
  ASSERT_TRUE(Instruction9);
  // %call = call noundef ptr @_Z8newThreePKi(ptr noundef %0), !dbg !282,
  // !psr.id !283; | ID: 30
  const auto *Instruction30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instruction30);
  const auto *FunctionZ8newThreePKi = IRDB.getFunction("_Z8newThreePKi");

  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction30, AliasImpl,
                               FunctionZ8newThreePKi->getArg(0), Instruction9));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction30, NoAliasImpl,
                               FunctionZ8newThreePKi->getArg(0), Instruction9));

  // ret ptr @GlobalFour, !dbg !240, !psr.id !241; | ID: 10
  const auto *Instruction10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instruction10);
  // %call3 = call noundef ptr @_Z10getFourPtrv(), !dbg !304, !psr.id !305; |
  // ID: 42
  const auto *Instruction42 = IRDB.getInstruction(42);
  ASSERT_TRUE(Instruction42);

  // TODO: determine ground truth
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instruction42, AliasImpl, UnusedValue, Instruction10));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction42, NoAliasImpl, UnusedValue,
                               Instruction10));

  // ret ptr @GlobalFour, !dbg !246, !psr.id !247; | ID: 11
  const auto *Instruction11 = IRDB.getInstruction(11);
  ASSERT_TRUE(Instruction11);
  //  %call5 = call noundef nonnull align 4 dereferenceable(4) ptr
  //  @_Z11getFourAddrv(), !dbg !310, !psr.id !311; | ID: 45
  const auto *Instruction45 = IRDB.getInstruction(45);
  ASSERT_TRUE(Instruction45);

  // TODO: determine ground truth
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instruction45, AliasImpl, UnusedValue, Instruction11));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction45, NoAliasImpl, UnusedValue,
                               Instruction11));

  // ret i32 %add6, !dbg !315, !psr.id !316; | ID: 48
  const auto *Instruction48 = IRDB.getInstruction(48);
  ASSERT_TRUE(Instruction48);
  // %call = call noundef i32 @_Z4callRiPKi(ptr noundef nonnull align 4
  // dereferenceable(4) %Zero, ptr noundef %One), !dbg !352, !psr.id !353; | ID:
  // 68
  const auto *Instruction68 = IRDB.getInstruction(68);
  ASSERT_TRUE(Instruction68);
  const auto *FuncZ4callRiPKi = IRDB.getFunction("_Z4callRiPKi");

  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction68, AliasImpl,
                               FuncZ4callRiPKi->getArg(0), Instruction48));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction68, NoAliasImpl,
                               FuncZ4callRiPKi->getArg(0), Instruction48));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction68, AliasImpl,
                               FuncZ4callRiPKi->getArg(1), Instruction48));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction68, NoAliasImpl,
                               FuncZ4callRiPKi->getArg(1), Instruction48));
}

/*
  CallToRetFlow
*/

TEST(PureFlow, CallToRetFlow01) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles +
       "pure_flow/call_to_ret_flow/call_to_ret_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());
}
}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
