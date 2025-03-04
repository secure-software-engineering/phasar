#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IDEAliasInfoTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IDENoAliasInfoTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

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
                      const LLVMProjectIRDB &IRDB) {
  const auto AliasNormalFlowFunc =
      AliasImpl.getNormalFlowFunction(Instr, nullptr);
  const auto AliasLLVMValueSet = AliasNormalFlowFunc->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getNormalFlowValueSet(const llvm::Instruction *Instr,
                      IDENoAliasImpl &NoAliasImpl,
                      const LLVMProjectIRDB &IRDB) {
  const auto NoAliasNormalFlowFunc =
      NoAliasImpl.getNormalFlowFunction(Instr, nullptr);
  const auto NoAliasLLVMValueSet = NoAliasNormalFlowFunc->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  // printValueSet(NoAliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  const auto AliasCallFlowFunc =
      AliasImpl.getCallFlowFunction(Instr, CalleeFunc);
  std::set<const llvm::Value *> AliasLLVMValueSet{};
  if (Arg) {
    AliasLLVMValueSet = AliasCallFlowFunc->computeTargets(Arg);
  }
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr, IDENoAliasImpl &NoAliasImpl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  const auto NoAliasCallFlowFunc =
      NoAliasImpl.getCallFlowFunction(Instr, CalleeFunc);
  std::set<const llvm::Value *> NoAliasLLVMValueSet{};
  if (Arg) {
    NoAliasLLVMValueSet = NoAliasCallFlowFunc->computeTargets(Arg);
  }
  // printValueSet(AliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  const auto AliasRetFlowFunc =
      AliasImpl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr);
  std::set<const llvm::Value *> AliasLLVMValueSet{};
  if (Arg) {
    AliasLLVMValueSet = AliasRetFlowFunc->computeTargets(Arg);
  }
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr, IDENoAliasImpl &NoAliasImpl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  const auto NoAliasRetFlowFunc =
      NoAliasImpl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr);
  std::set<const llvm::Value *> NoAliasLLVMValueSet{};
  if (Arg) {
    NoAliasLLVMValueSet = NoAliasRetFlowFunc->computeTargets(Arg);
  }
  // printValueSet(NoAliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}

#if false
std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr,
                         IDEAliasImpl &AliasImpl, const LLVMProjectIRDB &IRDB) {
  const auto AliasCallToRetFlowFunc =
      AliasImpl.getCallToRetFlowFunction(Instr, nullptr);
  const auto AliasLLVMValueSet = AliasCallToRetFlowFunc->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  // printValueSet(AliasLLVMValueSet);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr,
                         IDENoAliasImpl &NoAliasImpl,
                         const LLVMProjectIRDB &IRDB) {
  const auto NoAliasCallToRetFlowFunc =
      NoAliasImpl.getCallToRetFlowFunction(Instr, nullptr);
  const auto NoAliasLLVMValueSet = NoAliasCallToRetFlowFunc->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  // printValueSet(NoAliasLLVMValueSet);
  return NoAliasLLVMValueSet;
}
#endif

/*
  TODO: Check if Alias and NoAlias tests need more diverse testing. I.e. does
  Alias even need a store instr for example or are other instructions not
  covered by NoAlias viable too?
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
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{Instr7},
            getNormalFlowValueSet(Instr7, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr7, NoAliasImpl, IRDB));

  // store i32 %0, ptr %.addr, align 4
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr8, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr8, NoAliasImpl, IRDB));

  // store ptr %1, ptr %.addr1, align 8
  const auto *Instr10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instr10);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr10, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr10, NoAliasImpl, IRDB));

  // store i32 1, ptr %One, align 4, !dbg !220
  const auto *Instr13 = IRDB.getInstruction(13);
  ASSERT_TRUE(Instr13);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr13, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr13, NoAliasImpl, IRDB));

  // store i32 2, ptr %Two, align 4, !dbg !222
  const auto *Instr15 = IRDB.getInstruction(15);
  ASSERT_TRUE(Instr15);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr15, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr15, NoAliasImpl, IRDB));

  // store ptr %One, ptr %OnePtr, align 8, !dbg !225
  const auto *Instr17 = IRDB.getInstruction(17);
  ASSERT_TRUE(Instr17);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr17, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr17, NoAliasImpl, IRDB));

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !228
  const auto *Instr19 = IRDB.getInstruction(19);
  ASSERT_TRUE(Instr19);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr19, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr19, NoAliasImpl, IRDB));

#if false
  llvm::outs() << "2\n";
  const auto NFF8 = AliasImpl.getNormalFlowFunction(Instr8, nullptr);
  llvm::outs() << "3\n";
  const auto *const FuncOfIDEight = Instr8->getFunction();
  llvm::outs() << "4\n";
  const auto *Arg0 = FuncOfIDEight->getArg(0);
  llvm::outs() << "5\n";
  const auto *Test = IRDB.getInstruction(1);
  llvm::outs() << "6\n";
  const auto TestSet = NFF8->computeTargets(Arg0);
  llvm::outs() << "7\n";
  const auto TestSet2 = NFF8->computeTargets(Test);
  llvm::outs() << "8\n";
  // Falls wir leer erwarten
  EXPECT_EQ(std::set<const llvm::Value *>{}, TestSet);
  llvm::outs() << "9\n";
  // Falls wir z.B. nur Source erwarten
  EXPECT_EQ(std::set<const llvm::Value *>{Arg0}, TestSet);
  llvm::outs() << "10\n";
  // Falls wir Source und getPointerOp erwarten
  EXPECT_EQ(std::set<const llvm::Value *>({Arg0, Test}), TestSet);

  llvm::outs() << "11\n";
  // TODO: Fabian fragen, das die richtigen Instructions sind?
  // IRDB.getInstruction(7); // store i32 0, ptr %retval, align 4, !psr.id !222
  // IRDB.getInstruction(8); // store i32 %0, ptr %.addr, align 4, !psr.id !223

  // IRDB.getInstruction(13); // store i32 1, ptr %One, align 4, !dbg !232,
  // !psr.id !234
  // IRDB.getInstruction(15); // store i32 2, ptr %Two, align 4,
  // !dbg !236, !psr.id !238
#endif
}

/*
  // TODO: comment instruction here
  const auto *Instr7 = IRDB.getInstruction(7);
  ASSERT_TRUE(Instr7);
  const auto NFF7 = NoAliasImpl.getNormalFlowFunction(Instr7, nullptr);
  const auto LLVMValueSet7 = NFF7->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  // printValueSet(LLVMValueSet7);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet7);
*/

TEST(PureFlow, NormalFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);

  // IRDB.emitPreprocessedIR(llvm::outs());
  //  TODO: double checken
  //  IRDB.getInstruction(3);
  //  IRDB.getInstruction(5);
  //  IRDB.getInstruction(12);
  //  IRDB.getInstruction(28);
  //  IRDB.getInstruction(29);
  //  IRDB.getInstruction(32);
  //  IRDB.getInstruction(34);
  //  IRDB.getInstruction(36);

  // TODO: get instr of call function as well

  // store i32 %ThreeArg, ptr %ThreeArg.addr, align 4, !psr.id !219
  const auto *Instr3 = IRDB.getInstruction(3);
  ASSERT_TRUE(Instr3);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr3, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr3, NoAliasImpl, IRDB));

  // store ptr %TwoPtrArg, ptr %TwoPtrArg.addr, align 8, !psr.id !223
  const auto *Instr5 = IRDB.getInstruction(5);
  ASSERT_TRUE(Instr5);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr5, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr5, NoAliasImpl, IRDB));

  // store i32 %add, ptr %Five, align 4, !dbg !228, !psr.id !238
  const auto *Instr12 = IRDB.getInstruction(12);
  ASSERT_TRUE(Instr12);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr12, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr12, NoAliasImpl, IRDB));

  // store i32 1, ptr %One, align 4, !dbg !233, !psr.id !235
  const auto *Instr28 = IRDB.getInstruction(28);
  ASSERT_TRUE(Instr28);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr28, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr28, NoAliasImpl, IRDB));

  // store i32 2, ptr %Two, align 4, !dbg !237, !psr.id !239
  const auto *Instr30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instr30);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr30, AliasImpl, IRDB));
// find out why test below crashes
#if false
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr30, NoAliasImpl, IRDB));
#endif

  // store ptr %One, ptr %OnePtr, align 8, !dbg !243, !psr.id !245
  const auto *Instr32 = IRDB.getInstruction(32);
  ASSERT_TRUE(Instr32);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr32, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr32, NoAliasImpl, IRDB));

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !248, !psr.id !250
  const auto *Instr34 = IRDB.getInstruction(34);
  ASSERT_TRUE(Instr34);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr34, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr34, NoAliasImpl, IRDB));

  // store i32 3, ptr %Three, align 4, !dbg !252, !psr.id !254
  const auto *Instr36 = IRDB.getInstruction(36);
  ASSERT_TRUE(Instr36);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr36, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr36, NoAliasImpl, IRDB));
}

TEST(PureFlow, NormalFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);

  // IRDB.emitPreprocessedIR(llvm::outs());
  //  TODO: double checken
  //  IRDB.getInstruction(2);
  //  IRDB.getInstruction(4);
  //  IRDB.getInstruction(17);
  //  IRDB.getInstruction(19);
  //  IRDB.getInstruction(21);
  //  IRDB.getInstruction(26);
  //  IRDB.getInstruction(30);
  //  IRDB.getInstruction(47);
  //  IRDB.getInstruction(49);
  //  IRDB.getInstruction(51);
  //  IRDB.getInstruction(53);
  //  IRDB.getInstruction(55);
  //  IRDB.getInstruction(57);

  // store ptr %OnePtrArg, ptr %OnePtrArg.addr, align 8, !psr.id !218
  const auto *Instr2 = IRDB.getInstruction(2);
  ASSERT_TRUE(Instr2);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr2, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr2, NoAliasImpl, IRDB));

  // store ptr %FourPtrArg, ptr %FourPtrArg.addr, align 8, !psr.id !222
  const auto *Instr4 = IRDB.getInstruction(4);
  ASSERT_TRUE(Instr4);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr4, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr4, NoAliasImpl, IRDB));

  // store ptr %OnePtrArg, ptr %OnePtrArg.addr, align 8, !psr.id !222
  const auto *Instr17 = IRDB.getInstruction(17);
  ASSERT_TRUE(Instr17);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr17, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr17, NoAliasImpl, IRDB));

  // store ptr %TwoPtrArg, ptr %TwoPtrArg.addr, align 8, !psr.id !226
  const auto *Instr19 = IRDB.getInstruction(19);
  ASSERT_TRUE(Instr19);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr19, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr19, NoAliasImpl, IRDB));

  // store i32 %ThreeArg, ptr %ThreeArg.addr, align 4, !psr.id !230
  const auto *Instr21 = IRDB.getInstruction(21);
  ASSERT_TRUE(Instr21);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr21, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr21, NoAliasImpl, IRDB));

  // store i32 %add, ptr %Four, align 4, !dbg !235, !psr.id !241
  const auto *Instr26 = IRDB.getInstruction(26);
  ASSERT_TRUE(Instr26);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr26, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr26, NoAliasImpl, IRDB));

  // store i32 %call, ptr %Five, align 4, !dbg !243, !psr.id !249
  const auto *Instr30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instr30);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr30, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr30, NoAliasImpl, IRDB));

  // store i32 1, ptr %One, align 4, !dbg !234, !psr.id !236
  const auto *Instr47 = IRDB.getInstruction(47);
  ASSERT_TRUE(Instr47);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr47, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr47, NoAliasImpl, IRDB));

  // store i32 2, ptr %Two, align 4, !dbg !238, !psr.id !240
  const auto *Instr49 = IRDB.getInstruction(49);
  ASSERT_TRUE(Instr49);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr49, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr49, NoAliasImpl, IRDB));

  // store i32 3, ptr %Three, align 4, !dbg !242, !psr.id !244
  const auto *Instr51 = IRDB.getInstruction(51);
  ASSERT_TRUE(Instr51);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr51, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr51, NoAliasImpl, IRDB));

  // store i32 0, ptr %Zero, align 4, !dbg !246, !psr.id !248
  const auto *Instr53 = IRDB.getInstruction(53);
  ASSERT_TRUE(Instr53);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr53, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr53, NoAliasImpl, IRDB));

  // store ptr %One, ptr %OnePtr, align 8, !dbg !251, !psr.id !253
  const auto *Instr55 = IRDB.getInstruction(55);
  ASSERT_TRUE(Instr55);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr55, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr55, NoAliasImpl, IRDB));

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !256, !psr.id !258
  const auto *Instr57 = IRDB.getInstruction(57);
  ASSERT_TRUE(Instr57);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr57, AliasImpl, IRDB));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr57, NoAliasImpl, IRDB));
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

  // call void @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg !256, !psr.id
  // !257; | ID: 23
  const auto *Instr23 = IRDB.getInstruction(23);
  ASSERT_TRUE(Instr23);
  const auto *FuncForInstr23 = IRDB.getFunction("_Z4callii");

  // TODO: go over ground truth with Fabian
  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Instr23)) {
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr23->getArg(0)},
              getCallFlowValueSet(Instr23, AliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr23));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr23->getArg(1)},
              getCallFlowValueSet(Instr23, AliasImpl,
                                  CallSite->getArgOperand(1), FuncForInstr23));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr23->getArg(0)},
              getCallFlowValueSet(Instr23, NoAliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr23));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr23->getArg(1)},
              getCallFlowValueSet(Instr23, NoAliasImpl,
                                  CallSite->getArgOperand(1), FuncForInstr23));
  } else {
    FAIL();
  }
}

TEST(PureFlow, CallFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  // IRDB.emitPreprocessedIR(llvm::outs());

  // %call = call noundef i32 @_Z6getOnev(), !dbg !277, !psr.id !278; | ID: 29
  const auto *Instr29 = IRDB.getInstruction(29);
  ASSERT_TRUE(Instr29);
  const auto *FuncForInstr29 = IRDB.getFunction("_Z6getOnev");
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getCallFlowValueSet(Instr29, AliasImpl, nullptr, FuncForInstr29));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getCallFlowValueSet(Instr29, NoAliasImpl, nullptr, FuncForInstr29));

  // %call2 = call noundef i32 @_Z6getTwoi(i32 noundef %2), !dbg !285, !psr.id
  // !286; | ID: 33
  const auto *Instr33 = IRDB.getInstruction(33);
  ASSERT_TRUE(Instr33);
  const auto *FuncForInstr33 = IRDB.getFunction("_Z6getTwoi");
  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Instr33)) {
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr33->getArg(0)},
              getCallFlowValueSet(Instr33, AliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr33));
    EXPECT_EQ(std::set<const llvm::Value *>{FuncForInstr33->getArg(0)},
              getCallFlowValueSet(Instr33, NoAliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr33));
  } else {
    FAIL();
  }

  // %call3 = call noundef i32 @_Z8getThreePKi(ptr noundef %Two), !dbg !291,
  // !psr.id !292; | ID: 36
  const auto *Instr36 = IRDB.getInstruction(36);
  ASSERT_TRUE(Instr36);
  const auto *FuncForInstr36 = IRDB.getFunction("_Z8getThreePKi");
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

  // %call4 = call noundef ptr @_Z18getPtrToGlobalFourv(), !dbg !297, !psr.id
  // !298; | ID: 39
  const auto *Instr39 = IRDB.getInstruction(39);
  ASSERT_TRUE(Instr39);
  // TODO: go over ground truth with Fabian!
  const auto *FuncForInstr39 = IRDB.getFunction("_Z18getPtrToGlobalFourv");
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getCallFlowValueSet(Instr39, AliasImpl, nullptr, FuncForInstr39));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getCallFlowValueSet(Instr39, NoAliasImpl, nullptr, FuncForInstr39));
}

/*
  RetFlow
*/

TEST(PureFlow, RetFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);

  // %call = call noundef i32 @_Z6getTwov(), !dbg !231, !psr.id !232; | ID: 9
  const auto *Instr9 = IRDB.getInstruction(9);
  ASSERT_TRUE(Instr9);
  // ret i32 2, !dbg !212, !psr.id !213; | ID: 0
  const auto *Instr0 = IRDB.getInstruction(0);
  ASSERT_TRUE(Instr0);

  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist und
  // die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr9, AliasImpl, nullptr, Instr0));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr9, NoAliasImpl, nullptr, Instr0));

  // %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  // !276, !psr.id !277; | ID: 33
  const auto *Instr33 = IRDB.getInstruction(33);
  ASSERT_TRUE(Instr33);
  // ret i32 %add, !dbg !240, !psr.id !241; | ID: 14
  const auto *Instr14 = IRDB.getInstruction(14);
  ASSERT_TRUE(Instr14);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");

  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist und
  // die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr33, AliasImpl, FuncZ4callii->getArg(0), Instr14));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr33, AliasImpl, FuncZ4callii->getArg(1), Instr14));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr33, NoAliasImpl, FuncZ4callii->getArg(0),
                               Instr14));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr33, NoAliasImpl, FuncZ4callii->getArg(1),
                               Instr14));
}

TEST(PureFlow, RetFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);

  // %call = call noundef i32 @_Z6getTwov(), !dbg !240, !psr.id !241; | ID: 14
  const auto *Instr14 = IRDB.getInstruction(14);
  ASSERT_TRUE(Instr14);
  // ret i32 2, !dbg !212, !psr.id !213; | ID: 0
  const auto *Instr0 = IRDB.getInstruction(0);
  ASSERT_TRUE(Instr0);

  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist und
  // die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr14, AliasImpl, nullptr, Instr0));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr14, NoAliasImpl, nullptr, Instr0));

  // %call1 = call noundef i32 @_Z8newThreev(), !dbg !247, !psr.id !248; | ID:
  // 18
  const auto *Instr18 = IRDB.getInstruction(18);
  ASSERT_TRUE(Instr18);
  // ret i32 3, !dbg !220, !psr.id !221; | ID: 4
  const auto *Instr4 = IRDB.getInstruction(4);
  ASSERT_TRUE(Instr4);

  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist und
  // die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr18, AliasImpl, nullptr, Instr4));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr18, NoAliasImpl, nullptr, Instr4));

  // %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  //! 293, !psr.id !294; | ID: 42
  const auto *Instr42 = IRDB.getInstruction(42);
  ASSERT_TRUE(Instr42);
  // ret i32 %add, !dbg !257, !psr.id !258; | ID: 23
  const auto *Instr23 = IRDB.getInstruction(23);
  ASSERT_TRUE(Instr23);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");

  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist und
  // die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr42, AliasImpl, FuncZ4callii->getArg(0), Instr23));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr42, AliasImpl, FuncZ4callii->getArg(1), Instr23));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr42, NoAliasImpl, FuncZ4callii->getArg(0),
                               Instr23));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr42, NoAliasImpl, FuncZ4callii->getArg(1),
                               Instr23));
}

TEST(PureFlow, RetFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());

  // ret ptr %1, !dbg !234, !psr.id !235; | ID: 9
  const auto *Instruction9 = IRDB.getInstruction(9);
  ASSERT_TRUE(Instruction9);
  // %call = call noundef ptr @_Z8newThreePKi(ptr noundef %0), !dbg !282,
  // !psr.id !283; | ID: 30
  const auto *Instruction30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instruction30);
  const auto *FunctionZ8newThreePKi = IRDB.getFunction("_Z8newThreePKi");

  // TODO: determine ground truth
  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist
  // und die erste Instruction hier die call zu der Funktion ist.
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
  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist
  // und die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instruction42, AliasImpl, nullptr, Instruction10));
  // TODO: determine ground truth
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instruction42, NoAliasImpl, nullptr, Instruction10));

  // ret ptr @GlobalFour, !dbg !246, !psr.id !247; | ID: 11
  const auto *Instruction11 = IRDB.getInstruction(11);
  ASSERT_TRUE(Instruction11);
  //  %call5 = call noundef nonnull align 4 dereferenceable(4) ptr
  //  @_Z11getFourAddrv(), !dbg !310, !psr.id !311; | ID: 45
  const auto *Instruction45 = IRDB.getInstruction(45);
  ASSERT_TRUE(Instruction45);

  // TODO: determine ground truth
  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist
  // und die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instruction45, AliasImpl, nullptr, Instruction11));
  // TODO: determine ground truth
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instruction45, NoAliasImpl, nullptr, Instruction11));

  // ret i32 %add6, !dbg !315, !psr.id !316; | ID: 48
  const auto *Instruction48 = IRDB.getInstruction(48);
  ASSERT_TRUE(Instruction48);
  // %call = call noundef i32 @_Z4callRiPKi(ptr noundef nonnull align 4
  // dereferenceable(4) %Zero, ptr noundef %One), !dbg !347, !psr.id !348; | ID:
  // 65
  const auto *Instruction65 = IRDB.getInstruction(65);
  ASSERT_TRUE(Instruction65);
  const auto *FuncZ4callRiPKi = IRDB.getFunction("_Z4callRiPKi");

  // TODO: determine ground truth
  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist
  // und die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction65, AliasImpl,
                               FuncZ4callRiPKi->getArg(0), Instruction48));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction65, NoAliasImpl,
                               FuncZ4callRiPKi->getArg(0), Instruction48));
  // TODO: determine ground truth
  // TODO: Fabian fragen, ob die Exit Instruction die ret Instruction ist
  // und die erste Instruction hier die call zu der Funktion ist.
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction65, AliasImpl,
                               FuncZ4callRiPKi->getArg(1), Instruction48));
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instruction65, NoAliasImpl,
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
  // IRDB.emitPreprocessedIR(llvm::outs());
}

}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
