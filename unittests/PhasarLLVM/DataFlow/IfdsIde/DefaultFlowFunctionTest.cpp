#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAliasAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultNoAliasIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultReachableAllocationSitesIDEProblem.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <set>

using namespace psr;

namespace {

class IDEAliasImpl : public DefaultAliasAwareIFDSProblem {
public:
  IDEAliasImpl(LLVMProjectIRDB *IRDB)
      : DefaultAliasAwareIFDSProblem(IRDB, &PT, {}, {}), PT(IRDB) {};

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return {};
  };

private:
  FilteredLLVMAliasSet PT;
};

class IDENoAliasImpl : public DefaultNoAliasIFDSProblem {
public:
  using typename DefaultNoAliasIFDSProblem::d_t;
  using typename DefaultNoAliasIFDSProblem::l_t;
  using typename DefaultNoAliasIFDSProblem::n_t;

  IDENoAliasImpl(LLVMProjectIRDB *IRDB)
      : DefaultNoAliasIFDSProblem(IRDB, {}, {}) {};

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return {};
  };
};

class IDEReachableAllocationSitesImpl
    : public DefaultReachableAllocationSitesIFDSProblem {
public:
  IDEReachableAllocationSitesImpl(LLVMProjectIRDB *IRDB)
      : DefaultReachableAllocationSitesIFDSProblem(IRDB, &PT, {}, {}),
        PT(IRDB) {};

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return {};
  };

private:
  FilteredLLVMAliasSet PT;
};

std::set<const llvm::Value *>
getNormalFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                      const llvm::Value *Arg) {
  const auto AliasNormalFlowFunc =
      AliasImpl.getNormalFlowFunction(Instr, nullptr);
  const auto AliasLLVMValueSet = AliasNormalFlowFunc->computeTargets(Arg);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getNormalFlowValueSet(const llvm::Instruction *Instr,
                      IDENoAliasImpl &NoAliasImpl, const llvm::Value *Arg) {
  const auto NoAliasNormalFlowFunc =
      NoAliasImpl.getNormalFlowFunction(Instr, nullptr);
  const auto NoAliasLLVMValueSet = NoAliasNormalFlowFunc->computeTargets(Arg);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getNormalFlowValueSet(const llvm::Instruction *Instr,
                      IDEReachableAllocationSitesImpl &RASImpl,
                      const llvm::Value *Arg) {
  const auto RASNormalFlowFunc = RASImpl.getNormalFlowFunction(Instr, nullptr);
  const auto RASLLVMValueSet = RASNormalFlowFunc->computeTargets(Arg);
  return RASLLVMValueSet;
}

std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  const auto AliasCallFlowFunc =
      AliasImpl.getCallFlowFunction(Instr, CalleeFunc);
  std::set<const llvm::Value *> AliasLLVMValueSet =
      AliasCallFlowFunc->computeTargets(Arg);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr, IDENoAliasImpl &NoAliasImpl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  const auto NoAliasCallFlowFunc =
      NoAliasImpl.getCallFlowFunction(Instr, CalleeFunc);
  std::set<const llvm::Value *> NoAliasLLVMValueSet =
      NoAliasCallFlowFunc->computeTargets(Arg);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr,
                    IDEReachableAllocationSitesImpl &RASImpl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  const auto RASCallFlowFunc = RASImpl.getCallFlowFunction(Instr, CalleeFunc);
  std::set<const llvm::Value *> RASLLVMValueSet =
      RASCallFlowFunc->computeTargets(Arg);
  return RASLLVMValueSet;
}

std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr, IDEAliasImpl &AliasImpl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  const auto AliasRetFlowFunc =
      AliasImpl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr);
  std::set<const llvm::Value *> AliasLLVMValueSet =
      AliasRetFlowFunc->computeTargets(Arg);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr, IDENoAliasImpl &NoAliasImpl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  const auto NoAliasRetFlowFunc =
      NoAliasImpl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr);
  std::set<const llvm::Value *> NoAliasLLVMValueSet =
      NoAliasRetFlowFunc->computeTargets(Arg);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr,
                   IDEReachableAllocationSitesImpl &RASImpl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  const auto RASRetFlowFunc =
      RASImpl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr);
  std::set<const llvm::Value *> RASLLVMValueSet =
      RASRetFlowFunc->computeTargets(Arg);
  return RASLLVMValueSet;
}

std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr,
                         IDEAliasImpl &AliasImpl, const llvm::Value *Arg) {
  const auto AliasCallToRetFlowFunc =
      AliasImpl.getCallToRetFlowFunction(Instr, nullptr, {});
  const auto AliasLLVMValueSet = AliasCallToRetFlowFunc->computeTargets(Arg);
  return AliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr,
                         IDENoAliasImpl &NoAliasImpl, const llvm::Value *Arg) {
  const auto NoAliasCallToRetFlowFunc =
      NoAliasImpl.getCallToRetFlowFunction(Instr, nullptr, {});
  const auto NoAliasLLVMValueSet =
      NoAliasCallToRetFlowFunc->computeTargets(Arg);
  return NoAliasLLVMValueSet;
}

std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr,
                         IDEReachableAllocationSitesImpl &RASImpl,
                         const llvm::Value *Arg) {
  const auto RASCallToRetFlowFunc =
      RASImpl.getCallToRetFlowFunction(Instr, nullptr, {});
  const auto RASLLVMValueSet = RASCallToRetFlowFunc->computeTargets(Arg);
  return RASLLVMValueSet;
}

std::string stringifyValueSet(const std::set<const llvm::Value *> &Vals) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);

  ROS << "{ ";

  llvm::interleaveComma(
      Vals, ROS, [&ROS](const auto *Val) { ROS << llvmIRToString(Val); });

  ROS << " }";

  return Ret;
}

TEST(PureFlow, NormalFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  const auto *MainFunc = IRDB.getFunction("main");
  // %0
  const auto *PercentZero = MainFunc->getArg(0);
  // %1
  const auto *PercentOne = MainFunc->getArg(1);
  // %.addr = alloca i32, align 4, !psr.id !216; | ID: 1
  const auto *Instr1 = IRDB.getInstruction(1);
  // %.addr1 = alloca ptr, align 8, !psr.id !217; | ID: 2
  const auto *Instr2 = IRDB.getInstruction(2);
  //%One = alloca i32, align 4, !psr.id !218; | ID: 3
  const auto *Instr3 = IRDB.getInstruction(3);
  // %Two = alloca i32, align 4, !psr.id !219; | ID: 4
  const auto *Instr4 = IRDB.getInstruction(4);
  // %OnePtr = alloca ptr, align 8, !psr.id !220; | ID: 5
  const auto *Instr5 = IRDB.getInstruction(5);
  // %TwoAddr = alloca ptr, align 8, !psr.id !221; | ID: 6
  const auto *Instr6 = IRDB.getInstruction(6);

  // store i32 %0, ptr %.addr, align 4
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);
  EXPECT_EQ((std::set<const llvm::Value *>{PercentZero, Instr1}),
            getNormalFlowValueSet(Instr8, AliasImpl, PercentZero));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentZero, Instr1}),
            getNormalFlowValueSet(Instr8, NoAliasImpl, PercentZero));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentZero, Instr1}),
            getNormalFlowValueSet(Instr8, RASImpl, PercentZero));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr8, AliasImpl, Instr1));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr8, NoAliasImpl, Instr1));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr8, RASImpl, Instr1));

  // store ptr %1, ptr %.addr1, align 8
  const auto *Instr10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instr10);
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, Instr2}),
            getNormalFlowValueSet(Instr10, AliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, Instr2}),
            getNormalFlowValueSet(Instr10, NoAliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, Instr2}),
            getNormalFlowValueSet(Instr10, RASImpl, PercentOne));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr10, AliasImpl, Instr2));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr10, NoAliasImpl, Instr2));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr10, RASImpl, Instr2));

  // store ptr %One, ptr %OnePtr, align 8, !dbg !225
  const auto *Instr17 = IRDB.getInstruction(17);
  ASSERT_TRUE(Instr17);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr5}),
            getNormalFlowValueSet(Instr17, AliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr5}),
            getNormalFlowValueSet(Instr17, NoAliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr5}),
            getNormalFlowValueSet(Instr17, RASImpl, Instr3));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr17, AliasImpl, Instr5));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr17, NoAliasImpl, Instr5));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr17, RASImpl, Instr5));

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !228
  const auto *Instr19 = IRDB.getInstruction(19);
  ASSERT_TRUE(Instr19);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr4, Instr6}),
            getNormalFlowValueSet(Instr19, AliasImpl, Instr4));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr4, Instr6}),
            getNormalFlowValueSet(Instr19, NoAliasImpl, Instr4));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr4, Instr6}),
            getNormalFlowValueSet(Instr19, RASImpl, Instr4));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr19, AliasImpl, Instr6));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr19, NoAliasImpl, Instr6));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getNormalFlowValueSet(Instr19, RASImpl, Instr6));

  // Other arg
  EXPECT_EQ(std::set<const llvm::Value *>{Instr19},
            getNormalFlowValueSet(Instr19, AliasImpl, Instr19));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr19},
            getNormalFlowValueSet(Instr19, NoAliasImpl, Instr19));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr19},
            getNormalFlowValueSet(Instr19, RASImpl, Instr19));
}

TEST(PureFlow, NormalFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  const auto *MainFunc = IRDB.getFunction("main");
  ASSERT_TRUE(MainFunc);
  // %0
  const auto *PercentZero = MainFunc->getArg(0);
  ASSERT_TRUE(PercentZero);
  // %1
  const auto *PercentOne = MainFunc->getArg(1);
  ASSERT_TRUE(PercentOne);
  // %.addr = alloca i32, align 4, !psr.id !221; | ID: 2
  const auto *Instr2 = IRDB.getInstruction(2);
  ASSERT_TRUE(Instr2);
  // %.addr1 = alloca ptr, align 8, !psr.id !222; | ID: 3
  const auto *Instr3 = IRDB.getInstruction(3);
  ASSERT_TRUE(Instr3);

  // store i32 %0, ptr %.addr, align 4, !psr.id !227; | ID: 8
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);
  EXPECT_EQ((std::set<const llvm::Value *>{PercentZero, Instr2}),
            getNormalFlowValueSet(Instr8, AliasImpl, PercentZero));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentZero, Instr2}),
            getNormalFlowValueSet(Instr8, NoAliasImpl, PercentZero));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentZero, Instr2}),
            getNormalFlowValueSet(Instr8, RASImpl, PercentZero));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr8, AliasImpl, Instr2));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr8, NoAliasImpl, Instr2));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr8, RASImpl, Instr2));

  // store ptr %1, ptr %.addr1, align 8, !psr.id !231; | ID: 10
  const auto *Instr10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instr10);
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, Instr3}),
            getNormalFlowValueSet(Instr10, AliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, Instr3}),
            getNormalFlowValueSet(Instr10, NoAliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, Instr3}),
            getNormalFlowValueSet(Instr10, RASImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr10, AliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr10, NoAliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr10, RASImpl, Instr3));

  // Other arg
  EXPECT_EQ(std::set<const llvm::Value *>{Instr10},
            getNormalFlowValueSet(Instr10, AliasImpl, Instr10));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr10},
            getNormalFlowValueSet(Instr10, NoAliasImpl, Instr10));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr10},
            getNormalFlowValueSet(Instr10, RASImpl, Instr10));
}

TEST(PureFlow, NormalFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %One = alloca i32, align 4, !psr.id !222; | ID: 3
  const auto *Instr3 = IRDB.getInstruction(3);
  ASSERT_TRUE(Instr3);
  // %ForGEP = alloca %struct.StructOne, align 4, !psr.id !227; | ID: 8
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);
  // %2 = load i32, ptr %One, align 4, !dbg !245, !psr.id !246; | ID: 18
  const auto *Instr18 = IRDB.getInstruction(18);
  ASSERT_TRUE(Instr18);

  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr18}),
            getNormalFlowValueSet(Instr18, AliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr18}),
            getNormalFlowValueSet(Instr18, NoAliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr18}),
            getNormalFlowValueSet(Instr18, RASImpl, Instr3));
  // %3 = load i32, ptr %One, align 4, !dbg !251, !psr.id !252; | ID: 21
  const auto *Instr21 = IRDB.getInstruction(21);
  ASSERT_TRUE(Instr21);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr21}),
            getNormalFlowValueSet(Instr21, AliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr21}),
            getNormalFlowValueSet(Instr21, NoAliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr21}),
            getNormalFlowValueSet(Instr21, RASImpl, Instr3));

  // %tobool = icmp ne i32 %3, 0, !dbg !251, !psr.id !253; | ID: 22
  const auto *Instr22 = IRDB.getInstruction(22);
  ASSERT_TRUE(Instr22);
  // %lnot = xor i1 %tobool, true, !dbg !254, !psr.id !255; | ID: 23
  const auto *Instr23 = IRDB.getInstruction(23);
  ASSERT_TRUE(Instr23);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr22, Instr23}),
            getNormalFlowValueSet(Instr23, AliasImpl, Instr22));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr22, Instr23}),
            getNormalFlowValueSet(Instr23, NoAliasImpl, Instr22));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr22, Instr23}),
            getNormalFlowValueSet(Instr23, RASImpl, Instr22));

  // %4 = load i32, ptr %One, align 4, !dbg !261, !psr.id !262; | ID: 27
  const auto *Instr27 = IRDB.getInstruction(27);
  ASSERT_TRUE(Instr27);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr27}),
            getNormalFlowValueSet(Instr27, AliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr27}),
            getNormalFlowValueSet(Instr27, NoAliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr27}),
            getNormalFlowValueSet(Instr27, RASImpl, Instr3));

  // %5 = load i32, ptr %One, align 4, !dbg !263, !psr.id !264; | ID: 28
  const auto *Instr28 = IRDB.getInstruction(28);
  ASSERT_TRUE(Instr28);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr28}),
            getNormalFlowValueSet(Instr28, AliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr28}),
            getNormalFlowValueSet(Instr28, NoAliasImpl, Instr3));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr3, Instr28}),
            getNormalFlowValueSet(Instr28, RASImpl, Instr3));

  // %add = add nsw i32 %4, %5, !dbg !265, !psr.id !266; | ID: 29
  const auto *Instr29 = IRDB.getInstruction(29);
  ASSERT_TRUE(Instr29);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr27, Instr29}),
            getNormalFlowValueSet(Instr29, AliasImpl, Instr27));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr27, Instr29}),
            getNormalFlowValueSet(Instr29, NoAliasImpl, Instr27));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr27, Instr29}),
            getNormalFlowValueSet(Instr29, RASImpl, Instr27));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr28, Instr29}),
            getNormalFlowValueSet(Instr29, AliasImpl, Instr28));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr28, Instr29}),
            getNormalFlowValueSet(Instr29, NoAliasImpl, Instr28));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr28, Instr29}),
            getNormalFlowValueSet(Instr29, RASImpl, Instr28));

  // %One2 = getelementptr inbounds %struct.StructOne, ptr %ForGEP, i32 0, i32
  // 0, !dbg !282, !psr.id !283; | ID: 37
  const auto *Instr37 = IRDB.getInstruction(37);
  ASSERT_TRUE(Instr37);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr8, Instr37}),
            getNormalFlowValueSet(Instr37, AliasImpl, Instr8));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr8, Instr37}),
            getNormalFlowValueSet(Instr37, NoAliasImpl, Instr8));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr8, Instr37}),
            getNormalFlowValueSet(Instr37, RASImpl, Instr8));

  // %6 = load i32, ptr %One2, align 4, !dbg !282, !psr.id !284; | ID: 38
  const auto *Instr38 = IRDB.getInstruction(38);
  ASSERT_TRUE(Instr38);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr37, Instr38}),
            getNormalFlowValueSet(Instr38, AliasImpl, Instr37));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr37, Instr38}),
            getNormalFlowValueSet(Instr38, NoAliasImpl, Instr37));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr37, Instr38}),
            getNormalFlowValueSet(Instr38, RASImpl, Instr37));
}

TEST(PureFlow, NormalFlow04) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_04_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %Deref1 = alloca i32, align 4, !psr.id !222; | ID: 7
  const auto *Instr7 = IRDB.getInstruction(7);
  ASSERT_TRUE(Instr7);
  // %Deref2 = alloca i32, align 4, !psr.id !223; | ID: 8
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);
  // %Deref3 = alloca i32, align 4, !psr.id !224; | ID: 9
  const auto *Instr9 = IRDB.getInstruction(9);
  ASSERT_TRUE(Instr9);
  // %3 = load i32, ptr %2, align 4, !dbg !258, !psr.id !259; | ID: 25
  const auto *Instr25 = IRDB.getInstruction(25);
  ASSERT_TRUE(Instr25);
  // %6 = load i32, ptr %5, align 4, !dbg !268, !psr.id !269; | ID: 30
  const auto *Instr30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instr30);
  // %10 = load i32, ptr %9, align 4, !dbg !280, !psr.id !281; | ID: 36
  const auto *Instr36 = IRDB.getInstruction(36);
  ASSERT_TRUE(Instr36);

  // store i32 %3, ptr %Deref1, align 4, !dbg !254, !psr.id !260; | ID: 26
  const auto *Instr26 = IRDB.getInstruction(26);
  ASSERT_TRUE(Instr26);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr25, Instr7}),
            getNormalFlowValueSet(Instr26, AliasImpl, Instr25));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr25, Instr7}),
            getNormalFlowValueSet(Instr26, NoAliasImpl, Instr25));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr25, Instr7}),
            getNormalFlowValueSet(Instr26, RASImpl, Instr25));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr26, AliasImpl, Instr7));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr26, NoAliasImpl, Instr7));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr26, RASImpl, Instr7));

  // store i32 %6, ptr %Deref2, align 4, !dbg !262, !psr.id !270; | ID: 31
  const auto *Instr31 = IRDB.getInstruction(31);
  ASSERT_TRUE(Instr31);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr8, Instr30}),
            getNormalFlowValueSet(Instr31, AliasImpl, Instr30));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr8, Instr30}),
            getNormalFlowValueSet(Instr31, NoAliasImpl, Instr30));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr8, Instr30}),
            getNormalFlowValueSet(Instr31, RASImpl, Instr30));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr31, AliasImpl, Instr8));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr31, NoAliasImpl, Instr8));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr31, RASImpl, Instr8));

  // store i32 %10, ptr %Deref3, align 4, !dbg !272, !psr.id !282; | ID: 37
  const auto *Instr37 = IRDB.getInstruction(37);
  ASSERT_TRUE(Instr37);
  EXPECT_EQ((std::set<const llvm::Value *>{Instr9, Instr36}),
            getNormalFlowValueSet(Instr37, AliasImpl, Instr36));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr9, Instr36}),
            getNormalFlowValueSet(Instr37, NoAliasImpl, Instr36));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr9, Instr36}),
            getNormalFlowValueSet(Instr37, RASImpl, Instr36));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr37, AliasImpl, Instr9));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr37, NoAliasImpl, Instr9));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(Instr37, RASImpl, Instr9));
}

/*
  CallFlow
*/

TEST(PureFlow, CallFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // call void @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg !261, !psr.id
  // !262; | ID: 26
  const auto *Instr26 = IRDB.getInstruction(26);
  ASSERT_TRUE(Instr26);
  const auto *FuncForInstr26 = IRDB.getFunction("_Z4callii");
  ASSERT_TRUE(FuncForInstr26);
  ASSERT_EQ(FuncForInstr26->arg_size(), 2);
  const auto *Param0 = FuncForInstr26->getArg(0);
  const auto *Param1 = FuncForInstr26->getArg(1);

  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Instr26)) {
    EXPECT_EQ(std::set<const llvm::Value *>{Param0},
              getCallFlowValueSet(Instr26, AliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{Param1},
              getCallFlowValueSet(Instr26, AliasImpl,
                                  CallSite->getArgOperand(1), FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{Param0},
              getCallFlowValueSet(Instr26, NoAliasImpl,
                                  CallSite->getArgOperand(0), FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{Param1},
              getCallFlowValueSet(Instr26, NoAliasImpl,
                                  CallSite->getArgOperand(1), FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{Param0},
              getCallFlowValueSet(Instr26, RASImpl, CallSite->getArgOperand(0),
                                  FuncForInstr26));
    EXPECT_EQ(std::set<const llvm::Value *>{Param1},
              getCallFlowValueSet(Instr26, RASImpl, CallSite->getArgOperand(1),
                                  FuncForInstr26));
  } else {
    FAIL();
  }
}

TEST(PureFlow, CallFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %One = alloca i32, align 4, !psr.id !251; | ID: 17
  const auto *Instr17 = IRDB.getInstruction(17);
  ASSERT_TRUE(Instr17);
  // %Two = alloca i32, align 4, !psr.id !252; | ID: 18
  const auto *Instr18 = IRDB.getInstruction(18);
  ASSERT_TRUE(Instr18);
  // %Three = alloca i32, align 4, !psr.id !253; | ID: 19
  const auto *Instr19 = IRDB.getInstruction(19);
  ASSERT_TRUE(Instr19);
  // %PtrToOne = alloca ptr, align 8, !psr.id !254; | ID: 20
  const auto *Instr20 = IRDB.getInstruction(20);
  ASSERT_TRUE(Instr20);
  // %PtrToTwo = alloca ptr, align 8, !psr.id !255; | ID: 21
  const auto *Instr21 = IRDB.getInstruction(21);
  ASSERT_TRUE(Instr21);
  // %PtrPtrToTwo = alloca ptr, align 8, !psr.id !256; | ID: 22
  const auto *Instr22 = IRDB.getInstruction(22);
  ASSERT_TRUE(Instr22);
  // %PtrToThree = alloca ptr, align 8, !psr.id !257; | ID: 23
  const auto *Instr23 = IRDB.getInstruction(23);
  ASSERT_TRUE(Instr23);
  // %PtrPtrToThree = alloca ptr, align 8, !psr.id !258; | ID: 24
  const auto *Instr24 = IRDB.getInstruction(24);
  ASSERT_TRUE(Instr24);
  // %PtrPtrPtrToThree = alloca ptr, align 8, !psr.id !259; | ID: 25
  const auto *Instr25 = IRDB.getInstruction(25);
  ASSERT_TRUE(Instr25);
  // call void @_Z4callPi(ptr noundef %2), !dbg !307, !psr.id !308; | ID: 50
  const auto *Instr50 = IRDB.getInstruction(50);
  ASSERT_TRUE(Instr50);
  // call void @_Z10secondCallPiPS_PS0_(ptr noundef %3, ptr noundef %4, ptr
  // noundef %5), !dbg !315, !psr.id !316; | ID: 54
  const auto *Instr54 = IRDB.getInstruction(54);
  ASSERT_TRUE(Instr54);

  // call function
  const auto *CallFunc = IRDB.getFunction("_Z4callPi");
  ASSERT_TRUE(CallFunc);
  // second call function
  const auto *SecondCallFunc = IRDB.getFunction("_Z10secondCallPiPS_PS0_");
  ASSERT_TRUE(SecondCallFunc);

  const auto *CSCallFunc = llvm::cast<llvm::CallBase>(Instr50);
  const auto *CSSecondCallFunc = llvm::cast<llvm::CallBase>(Instr54);

  EXPECT_EQ((std::set<const llvm::Value *>{CallFunc->getArg(0)}),
            getCallFlowValueSet(Instr50, AliasImpl,
                                CSCallFunc->getArgOperand(0), CallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{CallFunc->getArg(0)}),
            getCallFlowValueSet(Instr50, NoAliasImpl,
                                CSCallFunc->getArgOperand(0), CallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{CallFunc->getArg(0)}),
            getCallFlowValueSet(Instr50, RASImpl, CSCallFunc->getArgOperand(0),
                                CallFunc));

  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(0)}),
            getCallFlowValueSet(Instr54, AliasImpl,
                                CSSecondCallFunc->getArgOperand(0),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(0)}),
            getCallFlowValueSet(Instr54, NoAliasImpl,
                                CSSecondCallFunc->getArgOperand(0),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(0)}),
            getCallFlowValueSet(Instr54, RASImpl,
                                CSSecondCallFunc->getArgOperand(0),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(1)}),
            getCallFlowValueSet(Instr54, AliasImpl,
                                CSSecondCallFunc->getArgOperand(1),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(1)}),
            getCallFlowValueSet(Instr54, NoAliasImpl,
                                CSSecondCallFunc->getArgOperand(1),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(1)}),
            getCallFlowValueSet(Instr54, RASImpl,
                                CSSecondCallFunc->getArgOperand(1),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(2)}),
            getCallFlowValueSet(Instr54, AliasImpl,
                                CSSecondCallFunc->getArgOperand(2),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(2)}),
            getCallFlowValueSet(Instr54, NoAliasImpl,
                                CSSecondCallFunc->getArgOperand(2),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(2)}),
            getCallFlowValueSet(Instr54, RASImpl,
                                CSSecondCallFunc->getArgOperand(2),
                                SecondCallFunc));
}

/*
  RetFlow
*/

TEST(PureFlow, RetFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

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
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr9, NoAliasImpl, UnusedValue, Instr0));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr9, RASImpl, UnusedValue, Instr0));

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
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr36, RASImpl, FuncZ4callii->getArg(0), Instr14));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr36, RASImpl, FuncZ4callii->getArg(1), Instr14));

  // negative tests
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr9, AliasImpl, FuncZ4callii->getArg(1), Instr0));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr9, NoAliasImpl, FuncZ4callii->getArg(1), Instr0));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr9, RASImpl, FuncZ4callii->getArg(1), Instr0));
}

TEST(PureFlow, RetFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

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
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr14, RASImpl, UnusedValue, Instr0));

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
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr18, RASImpl, UnusedValue, Instr4));

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
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr45, RASImpl, FuncZ4callii->getArg(0), Instr23));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr45, RASImpl, FuncZ4callii->getArg(1), Instr23));

  // negative tests
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr14, AliasImpl, FuncZ4callii->getArg(1), Instr0));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(Instr14, NoAliasImpl, FuncZ4callii->getArg(1),
                               Instr0));
  EXPECT_EQ(
      std::set<const llvm::Value *>{},
      getRetFlowValueSet(Instr14, RASImpl, FuncZ4callii->getArg(1), Instr0));
}

TEST(PureFlow, RetFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %ThreeInCall = alloca i32, align 4, !psr.id !254; | ID: 15
  const auto *Instr15 = IRDB.getValueFromId(15);
  ASSERT_TRUE(Instr15);

  // %ThreePtrInCall = alloca ptr, align 8, !psr.id !255; | ID: 16
  const auto *Instr16 = IRDB.getValueFromId(16);
  ASSERT_TRUE(Instr16);

  // %0 = load ptr, ptr %ThreePtrInCall, align 8, !dbg !280, !psr.id !281; | ID:
  // 29
  const auto *Instr29 = IRDB.getValueFromId(29);
  ASSERT_TRUE(Instr29);

  // %call = call noundef ptr @_Z8newThreePKi(ptr noundef %0), !dbg !282,
  // !psr.id !283; | ID: 30
  const auto *Instr30 = IRDB.getValueFromId(30);
  ASSERT_TRUE(Instr30);

  // ret ptr %1, !dbg !234, !psr.id !235; | ID: 9
  const auto *Instruction9 = IRDB.getInstruction(9);
  ASSERT_TRUE(Instruction9);
  // %call = call noundef ptr @_Z8newThreePKi(ptr noundef %0), !dbg !282,
  // !psr.id !283; | ID: 30
  const auto *Instruction30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instruction30);
  const auto *FunctionZ8newThreePKi = IRDB.getFunction("_Z8newThreePKi");

  const auto &Got = getRetFlowValueSet(
      Instruction30, AliasImpl, FunctionZ8newThreePKi->getArg(0), Instruction9);
  EXPECT_EQ(std::set<const llvm::Value *>{Instr29},
            getRetFlowValueSet(Instruction30, NoAliasImpl,
                               FunctionZ8newThreePKi->getArg(0), Instruction9));
  EXPECT_EQ((std::set<const llvm::Value *>{Instr29, Instr15, Instruction30}),
            getRetFlowValueSet(Instruction30, RASImpl,
                               FunctionZ8newThreePKi->getArg(0), Instruction9));

  EXPECT_EQ((std::set<const llvm::Value *>{Instr15, Instr16, Instr29, Instr30}),
            Got)
      << stringifyValueSet(Got);

  // ret ptr @GlobalFour, !dbg !240, !psr.id !241; | ID: 10
  const auto *Instruction10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instruction10);
  // %call3 = call noundef ptr @_Z10getFourPtrv(), !dbg !304, !psr.id !305; |
  // ID: 42
  const auto *Instruction42 = IRDB.getInstruction(42);
  ASSERT_TRUE(Instruction42);

  // ret ptr @GlobalFour, !dbg !246, !psr.id !247; | ID: 11
  const auto *Instruction11 = IRDB.getInstruction(11);
  ASSERT_TRUE(Instruction11);
  //  %call5 = call noundef nonnull align 4 dereferenceable(4) ptr
  //  @_Z11getFourAddrv(), !dbg !310, !psr.id !311; | ID: 45
  const auto *Instruction45 = IRDB.getInstruction(45);
  ASSERT_TRUE(Instruction45);

  // ret i32 %add6, !dbg !315, !psr.id !316; | ID: 48
  const auto *Instruction48 = IRDB.getInstruction(48);
  ASSERT_TRUE(Instruction48);
  // %call = call noundef i32 @_Z4callRiPKi(ptr noundef nonnull align 4
  // dereferenceable(4) %Zero, ptr noundef %One), !dbg !352, !psr.id !353; | ID:
  // 68
  const auto *Instruction68 = IRDB.getInstruction(68);
  ASSERT_TRUE(Instruction68);
  const auto *FuncZ4callRiPKi = IRDB.getFunction("_Z4callRiPKi");
  // %Zero = alloca i32, align 4, !psr.id !324; | ID: 52
  const auto *Instr52 = IRDB.getInstruction(52);
  ASSERT_TRUE(Instr52);

  // %One = alloca i32, align 4, !psr.id !325; | ID: 53
  const auto *Instr53 = IRDB.getInstruction(53);
  ASSERT_TRUE(Instr53);

  EXPECT_EQ(std::set<const llvm::Value *>{Instr52},
            getRetFlowValueSet(Instruction68, AliasImpl,
                               FuncZ4callRiPKi->getArg(0), Instruction48));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr52},
            getRetFlowValueSet(Instruction68, NoAliasImpl,
                               FuncZ4callRiPKi->getArg(0), Instruction48));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr52},
            getRetFlowValueSet(Instruction68, RASImpl,
                               FuncZ4callRiPKi->getArg(0), Instruction48));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr53},
            getRetFlowValueSet(Instruction68, AliasImpl,
                               FuncZ4callRiPKi->getArg(1), Instruction48));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr53},
            getRetFlowValueSet(Instruction68, NoAliasImpl,
                               FuncZ4callRiPKi->getArg(1), Instruction48));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr53},
            getRetFlowValueSet(Instruction68, RASImpl,
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
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // store i32 0, ptr %Zero, align 4, !dbg !252, !psr.id !254; | ID: 22
  const auto *Instr22 = IRDB.getInstruction(22);
  ASSERT_TRUE(Instr22);

  // store i32 1, ptr %One, align 4, !dbg !256, !psr.id !258; | ID: 24
  const auto *Instr24 = IRDB.getInstruction(24);
  ASSERT_TRUE(Instr24);

  // ret i32 0, !dbg !269, !psr.id !270; | ID: 30
  const auto *Instr30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instr30);

  EXPECT_EQ(std::set<const llvm::Value *>{Instr22},
            getCallToRetFlowValueSet(Instr30, AliasImpl, Instr22));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr22},
            getCallToRetFlowValueSet(Instr30, NoAliasImpl, Instr22));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr22},
            getCallToRetFlowValueSet(Instr30, RASImpl, Instr22));

  EXPECT_EQ(std::set<const llvm::Value *>{Instr24},
            getCallToRetFlowValueSet(Instr30, AliasImpl, Instr24));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr24},
            getCallToRetFlowValueSet(Instr30, NoAliasImpl, Instr24));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr24},
            getCallToRetFlowValueSet(Instr30, RASImpl, Instr24));

  EXPECT_EQ(std::set<const llvm::Value *>{Instr30},
            getCallToRetFlowValueSet(Instr30, AliasImpl, Instr30));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr30},
            getCallToRetFlowValueSet(Instr30, NoAliasImpl, Instr30));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr30},
            getCallToRetFlowValueSet(Instr30, RASImpl, Instr30));
}

TEST(PureFlow, CallToRetFlow02) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles +
       "pure_flow/call_to_ret_flow/call_to_ret_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // store i32 3, ptr %Three, align 4, !dbg !223, !psr.id !225; | ID: 5
  const auto *Instr5 = IRDB.getInstruction(5);
  ASSERT_TRUE(Instr5);

  // store i32 4, ptr %Four, align 4, !dbg !229, !psr.id !231; | ID: 8
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);

  // ret void, !dbg !234, !psr.id !235; | ID: 10
  const auto *Instr10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instr10);

  EXPECT_EQ(std::set<const llvm::Value *>{Instr5},
            getCallToRetFlowValueSet(Instr10, AliasImpl, Instr5));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr5},
            getCallToRetFlowValueSet(Instr10, NoAliasImpl, Instr5));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr5},
            getCallToRetFlowValueSet(Instr10, RASImpl, Instr5));

  EXPECT_EQ(std::set<const llvm::Value *>{Instr8},
            getCallToRetFlowValueSet(Instr10, AliasImpl, Instr8));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr8},
            getCallToRetFlowValueSet(Instr10, NoAliasImpl, Instr8));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr8},
            getCallToRetFlowValueSet(Instr10, RASImpl, Instr8));

  // store i32 1, ptr %One, align 4, !dbg !255, !psr.id !257; | ID: 22
  const auto *Instr22 = IRDB.getInstruction(22);
  ASSERT_TRUE(Instr22);

  // store i32 2, ptr %Two, align 4, !dbg !261, !psr.id !263; | ID: 25
  const auto *Instr25 = IRDB.getInstruction(25);
  ASSERT_TRUE(Instr25);

  // ret i32 0, !dbg !266, !psr.id !267; | ID: 27
  const auto *Instr27 = IRDB.getInstruction(27);
  ASSERT_TRUE(Instr27);

  EXPECT_EQ(std::set<const llvm::Value *>{Instr22},
            getCallToRetFlowValueSet(Instr27, AliasImpl, Instr22));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr22},
            getCallToRetFlowValueSet(Instr27, NoAliasImpl, Instr22));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr22},
            getCallToRetFlowValueSet(Instr27, RASImpl, Instr22));

  EXPECT_EQ(std::set<const llvm::Value *>{Instr25},
            getCallToRetFlowValueSet(Instr27, AliasImpl, Instr25));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr25},
            getCallToRetFlowValueSet(Instr27, NoAliasImpl, Instr25));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr25},
            getCallToRetFlowValueSet(Instr27, RASImpl, Instr25));

  EXPECT_EQ(std::set<const llvm::Value *>{Instr27},
            getCallToRetFlowValueSet(Instr27, AliasImpl, Instr27));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr27},
            getCallToRetFlowValueSet(Instr27, NoAliasImpl, Instr27));
  EXPECT_EQ(std::set<const llvm::Value *>{Instr27},
            getCallToRetFlowValueSet(Instr27, RASImpl, Instr27));
}

}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
