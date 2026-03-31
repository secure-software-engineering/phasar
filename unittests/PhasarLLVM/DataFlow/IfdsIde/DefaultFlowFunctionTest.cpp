#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAliasAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultNoAliasIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultReachableAllocationSitesIDEProblem.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <set>

using namespace psr;
using namespace psr::unittest;

namespace {

class IDEAliasImpl : public DefaultAliasAwareIFDSProblem {
public:
  IDEAliasImpl(LLVMProjectIRDB *IRDB)
      : DefaultAliasAwareIFDSProblem(IRDB, &PT, {}, {}), AS(IRDB) {};

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return {};
  };

private:
  LLVMAliasSet AS;
  FilteredLLVMAliasSet PT{&AS};
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
        AS(IRDB) {};

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return {};
  };

private:
  LLVMAliasSet AS;
  FilteredLLVMAliasSet PT{&AS};
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

using GTMap = std::vector<unittest::TestingSrcLocation>;

TEST(PureFlow, NormalFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // main function arg 0
  const auto *Arg0 =
      testingLocInIR(ArgInFun{.Idx = 0, .InFunction = "main"}, IRDB);
  ASSERT_TRUE(Arg0);
  // main function arg 1
  const auto *Arg1 =
      testingLocInIR(ArgInFun{.Idx = 1, .InFunction = "main"}, IRDB);
  ASSERT_TRUE(Arg1);

  // store i32 %0, ptr %.addr, align 4
  const auto *ValueStorePercent0 =
      testingLocInIR(LineColFunOp{.Line = 3,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(ValueStorePercent0);
  const auto *StorePercent0 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueStorePercent0);
  ASSERT_TRUE(StorePercent0);

  // store ptr %1, ptr %.addr1, align 8
  const auto *ValueStorePercent1 =
      testingLocInIR(LineColFunOp{.Line = 4,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(ValueStorePercent1);
  const auto *StorePercent1 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueStorePercent1);
  ASSERT_TRUE(StorePercent1);

  // store ptr %One, ptr %OnePtr, align 8, !dbg !225
  const auto *ValueStorePercentOne =
      testingLocInIR(LineColFunOp{.Line = 4,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(ValueStorePercentOne);
  const auto *StorePercentOne =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueStorePercentOne);
  ASSERT_TRUE(StorePercentOne);

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !228
  const auto *ValueStorePercentTwo =
      testingLocInIR(LineColFunOp{.Line = 5,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(ValueStorePercentTwo);
  const auto *StorePercentTwo =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueStorePercentTwo);
  ASSERT_TRUE(StorePercentTwo);

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, AliasImpl, StorePercent0));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, NoAliasImpl, StorePercent0));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, RASImpl, StorePercent0));

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent1},
            getNormalFlowValueSet(StorePercent1, AliasImpl, StorePercent1));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent1},
            getNormalFlowValueSet(StorePercent1, NoAliasImpl, StorePercent1));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent1},
            getNormalFlowValueSet(StorePercent1, RASImpl, StorePercent1));

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentOne},
            getNormalFlowValueSet(StorePercentOne, AliasImpl, StorePercentOne));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentOne},
      getNormalFlowValueSet(StorePercentOne, NoAliasImpl, StorePercentOne));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentOne},
            getNormalFlowValueSet(StorePercentOne, RASImpl, StorePercentOne));

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentTwo},
            getNormalFlowValueSet(StorePercentTwo, AliasImpl, StorePercentTwo));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentTwo},
      getNormalFlowValueSet(StorePercentTwo, NoAliasImpl, StorePercentTwo));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentTwo},
            getNormalFlowValueSet(StorePercentTwo, RASImpl, StorePercentTwo));
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
  // main function arg 0
  const auto *Arg0 =
      testingLocInIR(ArgInFun{.Idx = 0, .InFunction = "main"}, IRDB);
  ASSERT_TRUE(Arg0);
  // main function arg 1
  const auto *Arg1 =
      testingLocInIR(ArgInFun{.Idx = 1, .InFunction = "main"}, IRDB);
  ASSERT_TRUE(Arg1);

  // store i32 %0, ptr %.addr, align 4, !psr.id !227; | ID: 8
  const auto *ValueStorePercent0 =
      testingLocInIR(LineColFunOp{.Line = 5,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(ValueStorePercent0);
  const auto *StorePercent0 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueStorePercent0);
  ASSERT_TRUE(StorePercent0);

  // store ptr %1, ptr %.addr1, align 8, !psr.id !231; | ID: 10
  const auto *ValueStorePercent1 =
      testingLocInIR(LineColFunOp{.Line = 5,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(ValueStorePercent1);
  const auto *StorePercent1 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueStorePercent1);
  ASSERT_TRUE(StorePercent1);

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, AliasImpl, StorePercent0));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, NoAliasImpl, StorePercent0));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, RASImpl, StorePercent0));

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, AliasImpl, StorePercent0));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, NoAliasImpl, StorePercent0));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent0},
            getNormalFlowValueSet(StorePercent0, RASImpl, StorePercent0));

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent1},
            getNormalFlowValueSet(StorePercent1, AliasImpl, StorePercent1));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent1},
            getNormalFlowValueSet(StorePercent1, NoAliasImpl, StorePercent1));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercent1},
            getNormalFlowValueSet(StorePercent1, RASImpl, StorePercent1));
}

TEST(PureFlow, NormalFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %One = alloca i32, align 4, !psr.id !222; | ID: 3
  const auto *PercentOne =
      testingLocInIR(LineColFunOp{.Line = 8,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentOne);

  // %ForGEP = alloca %struct.StructOne, align 4, !psr.id !227; | ID: 8
  const auto *PercentForGEP =
      testingLocInIR(LineColFunOp{.Line = 13,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentForGEP);

  // %2 = load i32, ptr %One, align 4, !dbg !245, !psr.id !246; | ID: 18
  const auto *ValueLoadPercent2 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValueLoadPercent2);
  const auto *LoadPercent2 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueLoadPercent2);
  ASSERT_TRUE(LoadPercent2);

  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent2}),
            getNormalFlowValueSet(LoadPercent2, AliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent2}),
            getNormalFlowValueSet(LoadPercent2, NoAliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent2}),
            getNormalFlowValueSet(LoadPercent2, RASImpl, PercentOne));

  // %3 = load i32, ptr %One, align 4, !dbg !251, !psr.id !252; | ID: 21
  const auto *ValueLoadPercent3 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValueLoadPercent3);
  const auto *LoadPercent3 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueLoadPercent3);
  ASSERT_TRUE(LoadPercent3);
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent3}),
            getNormalFlowValueSet(LoadPercent3, AliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent3}),
            getNormalFlowValueSet(LoadPercent3, NoAliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent3}),
            getNormalFlowValueSet(LoadPercent3, RASImpl, PercentOne));

  // %tobool = icmp ne i32 %3, 0, !dbg !251, !psr.id !253; | ID: 22
  const auto *ValuePercenttobool =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValuePercenttobool);
  const auto *Percenttobool =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValuePercenttobool);
  ASSERT_TRUE(Percenttobool);

  // %lnot = xor i1 %tobool, true, !dbg !254, !psr.id !255; | ID: 23
  const auto *ValuePercentlnot =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValuePercentlnot);
  const auto *Percentlnot =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValuePercenttobool);
  ASSERT_TRUE(Percentlnot);

  EXPECT_EQ((std::set<const llvm::Value *>{Percenttobool, Percentlnot}),
            getNormalFlowValueSet(Percentlnot, AliasImpl, Percenttobool));
  EXPECT_EQ((std::set<const llvm::Value *>{Percenttobool, Percentlnot}),
            getNormalFlowValueSet(Percentlnot, NoAliasImpl, Percenttobool));
  EXPECT_EQ((std::set<const llvm::Value *>{Percenttobool, Percentlnot}),
            getNormalFlowValueSet(Percentlnot, RASImpl, Percenttobool));

  // %4 = load i32, ptr %One, align 4, !dbg !261, !psr.id !262; | ID: 27
  const auto *ValueLoadPercent4 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValueLoadPercent4);
  const auto *LoadPercent4 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueLoadPercent4);
  ASSERT_TRUE(LoadPercent4);

  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent4}),
            getNormalFlowValueSet(LoadPercent4, AliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent4}),
            getNormalFlowValueSet(LoadPercent4, NoAliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent4}),
            getNormalFlowValueSet(LoadPercent4, RASImpl, PercentOne));

  // %5 = load i32, ptr %One, align 4, !dbg !263, !psr.id !264; | ID: 28
  const auto *ValueLoadPercent5 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValueLoadPercent5);
  const auto *LoadPercent5 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValueLoadPercent5);
  ASSERT_TRUE(LoadPercent5);

  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent5}),
            getNormalFlowValueSet(LoadPercent5, AliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent5}),
            getNormalFlowValueSet(LoadPercent5, NoAliasImpl, PercentOne));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne, LoadPercent5}),
            getNormalFlowValueSet(LoadPercent5, RASImpl, PercentOne));

  // %add = add nsw i32 %4, %5, !dbg !265, !psr.id !266; | ID: 29
  const auto *ValueAdd =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValueAdd);
  const auto *Add = llvm::dyn_cast_or_null<llvm::Instruction>(ValueAdd);
  ASSERT_TRUE(Add);

  EXPECT_EQ((std::set<const llvm::Value *>{LoadPercent4, Add}),
            getNormalFlowValueSet(Add, AliasImpl, LoadPercent4));
  EXPECT_EQ((std::set<const llvm::Value *>{LoadPercent4, Add}),
            getNormalFlowValueSet(Add, NoAliasImpl, LoadPercent4));
  EXPECT_EQ((std::set<const llvm::Value *>{LoadPercent4, Add}),
            getNormalFlowValueSet(Add, RASImpl, LoadPercent4));
  EXPECT_EQ((std::set<const llvm::Value *>{LoadPercent5, Add}),
            getNormalFlowValueSet(Add, AliasImpl, LoadPercent5));
  EXPECT_EQ((std::set<const llvm::Value *>{LoadPercent5, Add}),
            getNormalFlowValueSet(Add, NoAliasImpl, LoadPercent5));
  EXPECT_EQ((std::set<const llvm::Value *>{LoadPercent5, Add}),
            getNormalFlowValueSet(Add, RASImpl, LoadPercent5));

  // %One2 = getelementptr inbounds %struct.StructOne, ptr %ForGEP, i32 0, i32
  // 0, !dbg !282, !psr.id !283; | ID: 37
  const auto *ValuePercentOne2 =
      testingLocInIR(LineColFunOp{.Line = 14,
                                  .Col = 20,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::GetElementPtr},
                     IRDB);
  ASSERT_TRUE(ValuePercentOne2);
  const auto *PercentOne2 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValuePercentOne2);
  ASSERT_TRUE(PercentOne2);

  EXPECT_EQ((std::set<const llvm::Value *>{PercentForGEP, PercentOne2}),
            getNormalFlowValueSet(PercentOne2, AliasImpl, PercentForGEP));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentForGEP, PercentOne2}),
            getNormalFlowValueSet(PercentOne2, NoAliasImpl, PercentForGEP));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentForGEP, PercentOne2}),
            getNormalFlowValueSet(PercentOne2, RASImpl, PercentForGEP));

  // %6 = load i32, ptr %One2, align 4, !dbg !282, !psr.id !284; | ID: 38
  const auto *ValuePercent6 =
      testingLocInIR(LineColFunOp{.Line = 14,
                                  .Col = 20,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValuePercent6);
  const auto *Percent6 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValuePercent6);
  ASSERT_TRUE(Percent6);

  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne2, Percent6}),
            getNormalFlowValueSet(Percent6, AliasImpl, PercentOne2));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne2, Percent6}),
            getNormalFlowValueSet(Percent6, NoAliasImpl, PercentOne2));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentOne2, Percent6}),
            getNormalFlowValueSet(Percent6, RASImpl, PercentOne2));
}

TEST(PureFlow, NormalFlow04) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_04_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %Deref1 = alloca i32, align 4, !psr.id !222; | ID: 7
  const auto *PercentDeref1 =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentDeref1);
  // %Deref2 = alloca i32, align 4, !psr.id !223; | ID: 8
  const auto *PercentDeref2 =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentDeref2);
  // %Deref3 = alloca i32, align 4, !psr.id !224; | ID: 9
  const auto *PercentDeref3 =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentDeref3);
  // %3 = load i32, ptr %2, align 4, !dbg !258, !psr.id !259; | ID: 25
  const auto *Percent3 =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 16,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(Percent3);
  // %6 = load i32, ptr %5, align 4, !dbg !268, !psr.id !269; | ID: 30
  const auto *Percent6 =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 16,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(Percent6);
  // %10 = load i32, ptr %9, align 4, !dbg !280, !psr.id !281; | ID: 36
  const auto *Percent10 =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 16,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(Percent10);
  // store i32 %3, ptr %Deref1, align 4, !dbg !254, !psr.id !260; | ID: 26
  const auto *StorePercent3Value =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercent3Value);
  const auto *StorePercent3 =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercent3Value);
  ASSERT_TRUE(StorePercent3);

  EXPECT_EQ((std::set<const llvm::Value *>{Percent3, PercentDeref1}),
            getNormalFlowValueSet(StorePercent3, AliasImpl, Percent3));
  EXPECT_EQ((std::set<const llvm::Value *>{Percent3, PercentDeref1}),
            getNormalFlowValueSet(StorePercent3, NoAliasImpl, Percent3));
  EXPECT_EQ((std::set<const llvm::Value *>{Percent3, PercentDeref1}),
            getNormalFlowValueSet(StorePercent3, RASImpl, Percent3));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent3, AliasImpl, PercentDeref1));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent3, NoAliasImpl, PercentDeref1));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent3, RASImpl, PercentDeref1));

  // store i32 %6, ptr %Deref2, align 4, !dbg !262, !psr.id !270; | ID: 31
  const auto *StorePercent6Value =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercent6Value);
  const auto *StorePercent6 =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercent6Value);
  ASSERT_TRUE(StorePercent6);

  EXPECT_EQ((std::set<const llvm::Value *>{PercentDeref2, Percent6}),
            getNormalFlowValueSet(StorePercent6, AliasImpl, Percent6));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentDeref2, Percent6}),
            getNormalFlowValueSet(StorePercent6, NoAliasImpl, Percent6));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentDeref2, Percent6}),
            getNormalFlowValueSet(StorePercent6, RASImpl, Percent6));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent6, AliasImpl, PercentDeref2));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent6, NoAliasImpl, PercentDeref2));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent6, RASImpl, PercentDeref2));

  // store i32 %10, ptr %Deref3, align 4, !dbg !272, !psr.id !282; | ID: 37
  const auto *StorePercent10Value =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercent10Value);
  const auto *StorePercent10 =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercent10Value);
  ASSERT_TRUE(StorePercent10);

  EXPECT_EQ((std::set<const llvm::Value *>{PercentDeref3, Percent10}),
            getNormalFlowValueSet(StorePercent10, AliasImpl, Percent10));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentDeref3, Percent10}),
            getNormalFlowValueSet(StorePercent10, NoAliasImpl, Percent10));
  EXPECT_EQ((std::set<const llvm::Value *>{PercentDeref3, Percent10}),
            getNormalFlowValueSet(StorePercent10, RASImpl, Percent10));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent10, AliasImpl, PercentDeref3));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent10, NoAliasImpl, PercentDeref3));
  EXPECT_EQ((std::set<const llvm::Value *>{}),
            getNormalFlowValueSet(StorePercent10, RASImpl, PercentDeref3));
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
  const auto *CallValue =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(CallValue);
  const auto *Call = llvm::dyn_cast_or_null<llvm::Instruction>(CallValue);
  ASSERT_TRUE(Call);

  auto FuncTSL = FuncByName{.FuncName = "_Z4callii"};
  const auto *CallFuncValue = testingLocInIR(FuncTSL, IRDB);
  ASSERT_TRUE(CallFuncValue);
  const auto *CallFunc = llvm::dyn_cast<llvm::Function>(CallFuncValue);
  ASSERT_TRUE(CallFunc);
  const auto *Param0 = CallFunc->getArg(0);
  ASSERT_TRUE(Param0);
  const auto *Param1 = CallFunc->getArg(1);
  ASSERT_TRUE(Param1);

  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Call)) {
    EXPECT_EQ(std::set<const llvm::Value *>{Param0},
              getCallFlowValueSet(Call, AliasImpl, CallSite->getArgOperand(0),
                                  CallFunc));
    EXPECT_EQ(std::set<const llvm::Value *>{Param1},
              getCallFlowValueSet(Call, AliasImpl, CallSite->getArgOperand(1),
                                  CallFunc));
    EXPECT_EQ(std::set<const llvm::Value *>{Param0},
              getCallFlowValueSet(Call, NoAliasImpl, CallSite->getArgOperand(0),
                                  CallFunc));
    EXPECT_EQ(std::set<const llvm::Value *>{Param1},
              getCallFlowValueSet(Call, NoAliasImpl, CallSite->getArgOperand(1),
                                  CallFunc));
    EXPECT_EQ(std::set<const llvm::Value *>{Param0},
              getCallFlowValueSet(Call, RASImpl, CallSite->getArgOperand(0),
                                  CallFunc));
    EXPECT_EQ(std::set<const llvm::Value *>{Param1},
              getCallFlowValueSet(Call, RASImpl, CallSite->getArgOperand(1),
                                  CallFunc));
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

  // call void @_Z4callPi(ptr noundef %2), !dbg !307, !psr.id !308; | ID: 50
  const auto *CallValue =
      testingLocInIR(LineColFunOp{.Line = 19,
                                  .Col = 3,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(CallValue);
  const auto *CallInstr = llvm::dyn_cast<llvm::Instruction>(CallValue);
  ASSERT_TRUE(CallInstr);
  // call function
  const auto *CallFuncValue =
      testingLocInIR(FuncByName{.FuncName = "_Z4callPi"}, IRDB);
  ASSERT_TRUE(CallFuncValue);
  const auto *CallFunc = llvm::dyn_cast<llvm::Function>(CallFuncValue);
  ASSERT_TRUE(CallFunc);
  // call void @_Z10secondCallPiPS_PS0_(ptr noundef %3, ptr noundef %4, ptr
  // noundef %5), !dbg !315, !psr.id !316; | ID: 54
  const auto *SecondCallValue =
      testingLocInIR(LineColFunOp{.Line = 20,
                                  .Col = 3,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(SecondCallValue);
  const auto *SecondCallInstr =
      llvm::dyn_cast<llvm::Instruction>(SecondCallValue);
  ASSERT_TRUE(SecondCallInstr);
  // second call function
  const auto *SecondCallFuncValue =
      testingLocInIR(FuncByName{.FuncName = "_Z10secondCallPiPS_PS0_"}, IRDB);
  ASSERT_TRUE(SecondCallFuncValue);
  const auto *SecondCallFunc =
      llvm::dyn_cast<llvm::Function>(SecondCallFuncValue);
  ASSERT_TRUE(SecondCallFunc);

  const auto *CSCallFunc = llvm::cast<llvm::CallBase>(CallInstr);
  const auto *CSSecondCallFunc = llvm::cast<llvm::CallBase>(SecondCallInstr);

  EXPECT_EQ((std::set<const llvm::Value *>{CallFunc->getArg(0)}),
            getCallFlowValueSet(CallInstr, AliasImpl,
                                CSCallFunc->getArgOperand(0), CallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{CallFunc->getArg(0)}),
            getCallFlowValueSet(CallInstr, NoAliasImpl,
                                CSCallFunc->getArgOperand(0), CallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{CallFunc->getArg(0)}),
            getCallFlowValueSet(CallInstr, RASImpl,
                                CSCallFunc->getArgOperand(0), CallFunc));

  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(0)}),
            getCallFlowValueSet(SecondCallInstr, AliasImpl,
                                CSSecondCallFunc->getArgOperand(0),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(0)}),
            getCallFlowValueSet(SecondCallInstr, NoAliasImpl,
                                CSSecondCallFunc->getArgOperand(0),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(0)}),
            getCallFlowValueSet(SecondCallInstr, RASImpl,
                                CSSecondCallFunc->getArgOperand(0),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(1)}),
            getCallFlowValueSet(SecondCallInstr, AliasImpl,
                                CSSecondCallFunc->getArgOperand(1),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(1)}),
            getCallFlowValueSet(SecondCallInstr, NoAliasImpl,
                                CSSecondCallFunc->getArgOperand(1),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(1)}),
            getCallFlowValueSet(SecondCallInstr, RASImpl,
                                CSSecondCallFunc->getArgOperand(1),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(2)}),
            getCallFlowValueSet(SecondCallInstr, AliasImpl,
                                CSSecondCallFunc->getArgOperand(2),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(2)}),
            getCallFlowValueSet(SecondCallInstr, NoAliasImpl,
                                CSSecondCallFunc->getArgOperand(2),
                                SecondCallFunc));
  EXPECT_EQ((std::set<const llvm::Value *>{SecondCallFunc->getArg(2)}),
            getCallFlowValueSet(SecondCallInstr, RASImpl,
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
  const auto *PercentTwo =
      testingLocInIR(LineColFunOp{.Line = 13,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  // %call = call noundef i32 @_Z6getTwov(), !dbg !231, !psr.id !232; | ID: 9
  const auto *PercentCallGetTwoValue =
      testingLocInIR(LineColFunOp{.Line = 6,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCallGetTwoValue);
  const auto *PercentCallGetTwo =
      llvm::dyn_cast<llvm::Instruction>(PercentCallGetTwoValue);
  ASSERT_TRUE(PercentCallGetTwo);
  // ret i32 2, !dbg !212, !psr.id !213; | ID: 0
  const auto *RetValGetTwo =
      testingLocInIR(LineColFunOp{.Line = 3,
                                  .Col = 0,
                                  .InFunction = "_Z6getTwov",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetValGetTwo);
  const auto *RetValGetTwoInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValGetTwo);
  ASSERT_TRUE(RetValGetTwoInstr);

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, AliasImpl, PercentTwo,
                               RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, NoAliasImpl, PercentTwo,
                               RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, RASImpl, PercentTwo,
                               RetValGetTwoInstr));

  // %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  // !281, !psr.id !282; | ID: 36
  const auto *PercentCallValue =
      testingLocInIR(LineColFunOp{.Line = 15,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCallValue);
  const auto *PercentCall =
      llvm::dyn_cast_or_null<llvm::Instruction>(PercentCallValue);
  ASSERT_TRUE(PercentCall);
  // ret i32 %add, !dbg !240, !psr.id !241; | ID: 14
  const auto *RetValCallValue =
      testingLocInIR(LineColFunOp{.Line = 7,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetValCallValue);
  const auto *RetValCall =
      llvm::dyn_cast_or_null<llvm::Instruction>(RetValCallValue);
  ASSERT_TRUE(RetValCall);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");
  ASSERT_TRUE(FuncZ4callii);
  ASSERT_TRUE(FuncZ4callii->getArg(0));
  ASSERT_TRUE(FuncZ4callii->getArg(1));

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, AliasImpl, FuncZ4callii->getArg(0),
                               RetValCall));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, AliasImpl, FuncZ4callii->getArg(1),
                               RetValCall));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, NoAliasImpl,
                               FuncZ4callii->getArg(0), RetValCall));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, NoAliasImpl,
                               FuncZ4callii->getArg(1), RetValCall));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, RASImpl, FuncZ4callii->getArg(0),
                               RetValCall));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, RASImpl, FuncZ4callii->getArg(1),
                               RetValCall));

  // negative tests
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, AliasImpl,
                               FuncZ4callii->getArg(1), RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, NoAliasImpl,
                               FuncZ4callii->getArg(1), RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, RASImpl,
                               FuncZ4callii->getArg(1), RetValGetTwoInstr));
}

TEST(PureFlow, RetFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %Two = alloca i32, align 4, !psr.id !268; | ID: 29
  const auto *PercentTwo =
      testingLocInIR(LineColFunOp{.Line = 22,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);

  // %call = call noundef i32 @_Z6getTwov(), !dbg !240, !psr.id !241; | ID: 14
  const auto *PercentCallGetTwoValue =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCallGetTwoValue);
  const auto *PercentCallGetTwo =
      llvm::dyn_cast<llvm::Instruction>(PercentCallGetTwoValue);
  ASSERT_TRUE(PercentCallGetTwo);
  // ret i32 2, !dbg !212, !psr.id !213; | ID: 0
  const auto *RetValGetTwoValue =
      testingLocInIR(LineColFunOp{.Line = 3,
                                  .Col = 0,
                                  .InFunction = "_Z6getTwov",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetValGetTwoInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValGetTwoValue);
  ASSERT_TRUE(RetValGetTwoInstr);

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, AliasImpl, PercentTwo,
                               RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, NoAliasImpl, PercentTwo,
                               RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, RASImpl, PercentTwo,
                               RetValGetTwoInstr));

  // %call1 = call noundef i32 @_Z8newThreev(), !dbg !247, !psr.id !248; | ID:
  // 18
  const auto *PercentCall1 =
      testingLocInIR(LineColFunOp{.Line = 14,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCall1);
  const auto *PercentCallInstr1 =
      llvm::dyn_cast<llvm::Instruction>(PercentCall1);
  ASSERT_TRUE(PercentCallInstr1);
  // ret i32 3, !dbg !220, !psr.id !221; | ID: 4
  const auto *RetValNewThree =
      testingLocInIR(LineColFunOp{.Line = 7,
                                  .Col = 0,
                                  .InFunction = "_Z8newThreev",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetValNewThree);
  const auto *RetValNewThreeInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValNewThree);
  ASSERT_TRUE(RetValNewThreeInstr);

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallInstr1, AliasImpl, PercentTwo,
                               RetValNewThreeInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallInstr1, NoAliasImpl, PercentTwo,
                               RetValNewThreeInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallInstr1, RASImpl, PercentTwo,
                               RetValNewThreeInstr));

  //  %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  //  !298, !psr.id !299; | ID: 45
  const auto *PercentCallValue =
      testingLocInIR(LineColFunOp{.Line = 24,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCallValue);
  const auto *PercentCall = llvm::dyn_cast<llvm::Instruction>(PercentCallValue);
  ASSERT_TRUE(PercentCall);
  // ret i32 %add, !dbg !257, !psr.id !258; | ID: 23
  const auto *RetAddValue =
      testingLocInIR(LineColFunOp{.Line = 16,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetAddValue);
  const auto *RetAdd = llvm::dyn_cast_or_null<llvm::Instruction>(RetAddValue);
  ASSERT_TRUE(RetAdd);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");
  ASSERT_TRUE(FuncZ4callii);
  ASSERT_TRUE(FuncZ4callii->getArg(0));
  ASSERT_TRUE(FuncZ4callii->getArg(1));

  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, AliasImpl, FuncZ4callii->getArg(0),
                               RetAdd));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, AliasImpl, FuncZ4callii->getArg(1),
                               RetAdd));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, NoAliasImpl,
                               FuncZ4callii->getArg(0), RetAdd));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, NoAliasImpl,
                               FuncZ4callii->getArg(1), RetAdd));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, RASImpl, FuncZ4callii->getArg(0),
                               RetAdd));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCall, RASImpl, FuncZ4callii->getArg(1),
                               RetAdd));

  // negative tests
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, AliasImpl,
                               FuncZ4callii->getArg(1), RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, NoAliasImpl,
                               FuncZ4callii->getArg(1), RetValGetTwoInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{},
            getRetFlowValueSet(PercentCallGetTwo, RASImpl,
                               FuncZ4callii->getArg(1), RetValGetTwoInstr));
}

TEST(PureFlow, RetFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IDEReachableAllocationSitesImpl RASImpl =
      IDEReachableAllocationSitesImpl(&IRDB);

  // %ThreeInCall = alloca i32, align 4, !psr.id !254; | ID: 15
  const auto *PercentThreeInCall =
      testingLocInIR(LineColFunOp{.Line = 16,
                                  .Col = 0,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentThreeInCall);

  // %ThreePtrInCall = alloca ptr, align 8, !psr.id !255; | ID: 16
  const auto *PercentThreePtrInCall =
      testingLocInIR(LineColFunOp{.Line = 17,
                                  .Col = 0,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentThreePtrInCall);

  // %0 = load ptr, ptr %ThreePtrInCall, align 8, !dbg !280, !psr.id !281; | ID:
  // 29
  const auto *ValuePercent0 =
      testingLocInIR(LineColFunOp{.Line = 18,
                                  .Col = 40,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  ASSERT_TRUE(ValuePercent0);
  const auto *Percent0 =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValuePercent0);
  ASSERT_TRUE(Percent0);

  // %call = call noundef ptr @_Z8newThreePKi(ptr noundef %0), !dbg !282,
  // !psr.id !283; | ID: 30
  const auto *ValuePercentCall =
      testingLocInIR(LineColFunOp{.Line = 18,
                                  .Col = 31,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(ValuePercentCall);
  const auto *PercentCall =
      llvm::dyn_cast_or_null<llvm::Instruction>(ValuePercentCall);
  ASSERT_TRUE(PercentCall);

  // ret ptr %1, !dbg !234, !psr.id !235; | ID: 9
  const auto *RetValNewThreeValue =
      testingLocInIR(LineColFunOp{.Line = 7,
                                  .Col = 0,
                                  .InFunction = "_Z8newThreePKi",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetValNewThreeValue);
  const auto *RetValNewThreeInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValNewThreeValue);
  ASSERT_TRUE(RetValNewThreeInstr);

  const auto *FunctionZ8newThreePKi = IRDB.getFunction("_Z8newThreePKi");
  const auto &Got =
      getRetFlowValueSet(PercentCall, AliasImpl,
                         FunctionZ8newThreePKi->getArg(0), RetValNewThreeInstr);
  EXPECT_EQ(std::set<const llvm::Value *>{Percent0},
            getRetFlowValueSet(PercentCall, NoAliasImpl,
                               FunctionZ8newThreePKi->getArg(0),
                               RetValNewThreeInstr));
  EXPECT_EQ((std::set<const llvm::Value *>{Percent0, PercentThreeInCall,
                                           PercentCall}),
            getRetFlowValueSet(PercentCall, RASImpl,
                               FunctionZ8newThreePKi->getArg(0),
                               RetValNewThreeInstr));

  EXPECT_EQ(
      (std::set<const llvm::Value *>{PercentThreeInCall, PercentThreePtrInCall,
                                     Percent0, PercentCall}),
      Got)
      << stringifyValueSet(Got);

  // ret ptr @GlobalFour, !dbg !240, !psr.id !241; | ID: 10
  const auto *RetValGlobalFourValue =
      testingLocInIR(RetVal{.InFunction = "_Z10getFourPtrv"}, IRDB);
  ASSERT_TRUE(RetValGlobalFourValue);
  // %call3 = call noundef ptr @_Z10getFourPtrv(), !dbg !304, !psr.id !305; |
  // ID: 42
  const auto *PercentCall3Value =
      testingLocInIR(LineColFunOp{.Line = 20,
                                  .Col = 61,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCall3Value);
  const auto *PercentCall3 =
      llvm::dyn_cast_or_null<llvm::Instruction>(PercentCall3Value);
  ASSERT_TRUE(PercentCall3);

  // ret ptr @GlobalFour, !dbg !246, !psr.id !247; | ID: 11
  const auto *RetValGetFourValue =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "_Z10getFourPtrv",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetValGetFourValue);
  const auto *RetValGetFourInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValGetFourValue);
  ASSERT_TRUE(RetValGetFourInstr);

  const auto *RetValGlobalFourSecondValue =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 0,
                                  .InFunction = "_Z11getFourAddrv",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetValGlobalFourSecondValue);
  const auto *RetValGlobalFourSecond =
      llvm::dyn_cast<llvm::Instruction>(RetValGlobalFourSecondValue);
  ASSERT_TRUE(RetValGlobalFourSecond);

  //  %call5 = call noundef nonnull align 4 dereferenceable(4) ptr
  //  @_Z11getFourAddrv(), !dbg !310, !psr.id !311; | ID: 45
  const auto *PercentCall5Value =
      testingLocInIR(LineColFunOp{.Line = 21,
                                  .Col = 10,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCall5Value);
  const auto *PercentCall5 =
      llvm::dyn_cast_or_null<llvm::Instruction>(PercentCall5Value);
  ASSERT_TRUE(PercentCall5);

  // ret i32 %add6, !dbg !315, !psr.id !316; | ID: 48
  const auto *RetValAdd6Value =
      testingLocInIR(LineColFunOp{.Line = 20,
                                  .Col = 0,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetValAdd6Value);
  const auto *RetValAdd6 =
      llvm::dyn_cast_or_null<llvm::Instruction>(RetValAdd6Value);
  ASSERT_TRUE(RetValAdd6);
  // %call = call noundef i32 @_Z4callRiPKi(ptr noundef nonnull align 4
  // dereferenceable(4) %Zero, ptr noundef %One), !dbg !352, !psr.id !353; | ID:
  // 68
  const auto *PercentCallZeroOneValue =
      testingLocInIR(LineColFunOp{.Line = 29,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  ASSERT_TRUE(PercentCallZeroOneValue);
  const auto *PercentCallZeroOne =
      llvm::dyn_cast_or_null<llvm::Instruction>(PercentCallZeroOneValue);
  ASSERT_TRUE(PercentCallZeroOne);
  const auto *FuncZ4callRiPKi = IRDB.getFunction("_Z4callRiPKi");
  // %Zero = alloca i32, align 4, !psr.id !324; | ID: 52
  const auto *PercentZeroValue =
      testingLocInIR(LineColFunOp{.Line = 25,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentZeroValue);
  const auto *PercentZero =
      llvm::dyn_cast_or_null<llvm::Instruction>(PercentZeroValue);
  ASSERT_TRUE(PercentZero);

  // %One = alloca i32, align 4, !psr.id !325; | ID: 53
  const auto *PercentOneValue =
      testingLocInIR(LineColFunOp{.Line = 26,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  ASSERT_TRUE(PercentOneValue);
  const auto *PercentOne =
      llvm::dyn_cast_or_null<llvm::Instruction>(PercentOneValue);
  ASSERT_TRUE(PercentOne);

  EXPECT_EQ(std::set<const llvm::Value *>{PercentZero},
            getRetFlowValueSet(PercentCallZeroOne, AliasImpl,
                               FuncZ4callRiPKi->getArg(0), RetValAdd6));
  EXPECT_EQ(std::set<const llvm::Value *>{PercentZero},
            getRetFlowValueSet(PercentCallZeroOne, NoAliasImpl,
                               FuncZ4callRiPKi->getArg(0), RetValAdd6));
  EXPECT_EQ(std::set<const llvm::Value *>{PercentZero},
            getRetFlowValueSet(PercentCallZeroOne, RASImpl,
                               FuncZ4callRiPKi->getArg(0), RetValAdd6));
  EXPECT_EQ(std::set<const llvm::Value *>{PercentOne},
            getRetFlowValueSet(PercentCallZeroOne, AliasImpl,
                               FuncZ4callRiPKi->getArg(1), RetValAdd6));
  EXPECT_EQ(std::set<const llvm::Value *>{PercentOne},
            getRetFlowValueSet(PercentCallZeroOne, NoAliasImpl,
                               FuncZ4callRiPKi->getArg(1), RetValAdd6));
  EXPECT_EQ(std::set<const llvm::Value *>{PercentOne},
            getRetFlowValueSet(PercentCallZeroOne, RASImpl,
                               FuncZ4callRiPKi->getArg(1), RetValAdd6));
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
  const auto *StorePercentZeroValue =
      testingLocInIR(LineColFunOp{.Line = 6,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercentZeroValue);
  const auto *StorePercentZero =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercentZeroValue);
  ASSERT_TRUE(StorePercentZero);

  // store i32 1, ptr %One, align 4, !dbg !256, !psr.id !258; | ID: 24
  const auto *StorePercentOneValue =
      testingLocInIR(LineColFunOp{.Line = 6,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercentOneValue);
  const auto *StorePercentOne =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercentOneValue);
  ASSERT_TRUE(StorePercentOne);

  // ret i32 0, !dbg !269, !psr.id !270; | ID: 30
  const auto *RetMainValue =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetMainValue);
  const auto *RetMainInstr = llvm::dyn_cast<llvm::Instruction>(RetMainValue);
  ASSERT_TRUE(RetMainInstr);

  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentZero},
      getCallToRetFlowValueSet(RetMainInstr, AliasImpl, StorePercentZero));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentZero},
      getCallToRetFlowValueSet(RetMainInstr, NoAliasImpl, StorePercentZero));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentZero},
            getCallToRetFlowValueSet(RetMainInstr, RASImpl, StorePercentZero));

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentOne},
            getCallToRetFlowValueSet(RetMainInstr, AliasImpl, StorePercentOne));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentOne},
      getCallToRetFlowValueSet(RetMainInstr, NoAliasImpl, StorePercentOne));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentOne},
            getCallToRetFlowValueSet(RetMainInstr, RASImpl, StorePercentOne));

  EXPECT_EQ(std::set<const llvm::Value *>{RetMainInstr},
            getCallToRetFlowValueSet(RetMainInstr, AliasImpl, RetMainInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{RetMainInstr},
            getCallToRetFlowValueSet(RetMainInstr, NoAliasImpl, RetMainInstr));
  EXPECT_EQ(std::set<const llvm::Value *>{RetMainInstr},
            getCallToRetFlowValueSet(RetMainInstr, RASImpl, RetMainInstr));
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
  const auto *StorePercentThreeValue =
      testingLocInIR(LineColFunOp{.Line = 6,
                                  .Col = 7,
                                  .InFunction = "_Z4callv",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercentThreeValue);
  const auto *StorePercentThree =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercentThreeValue);
  ASSERT_TRUE(StorePercentThree);

  // store i32 4, ptr %Four, align 4, !dbg !229, !psr.id !231; | ID: 8
  const auto *StorePercentFourValue =
      testingLocInIR(LineColFunOp{.Line = 8,
                                  .Col = 7,
                                  .InFunction = "_Z4callv",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercentFourValue);
  const auto *StorePercentFour =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercentFourValue);
  ASSERT_TRUE(StorePercentFour);

  // ret void, !dbg !234, !psr.id !235; | ID: 10
  const auto *RetCallValue =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "_Z4callv",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetCallValue);
  const auto *RetCall = llvm::dyn_cast_or_null<llvm::Instruction>(RetCallValue);
  ASSERT_TRUE(RetCall);

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentThree},
            getCallToRetFlowValueSet(RetCall, AliasImpl, StorePercentThree));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentThree},
            getCallToRetFlowValueSet(RetCall, NoAliasImpl, StorePercentThree));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentThree},
            getCallToRetFlowValueSet(RetCall, RASImpl, StorePercentThree));

  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentFour},
            getCallToRetFlowValueSet(RetCall, AliasImpl, StorePercentFour));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentFour},
            getCallToRetFlowValueSet(RetCall, NoAliasImpl, StorePercentFour));
  EXPECT_EQ(std::set<const llvm::Value *>{StorePercentFour},
            getCallToRetFlowValueSet(RetCall, RASImpl, StorePercentFour));

  // store i32 1, ptr %One, align 4, !dbg !255, !psr.id !257; | ID: 22
  const auto *StorePercentOneValue =
      testingLocInIR(LineColFunOp{.Line = 13,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercentOneValue);
  const auto *StorePercentOne =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercentOneValue);
  ASSERT_TRUE(StorePercentOne);

  // store i32 2, ptr %Two, align 4, !dbg !261, !psr.id !263; | ID: 25
  const auto *StorePercentTwoValue =
      testingLocInIR(LineColFunOp{.Line = 15,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  ASSERT_TRUE(StorePercentTwoValue);
  const auto *StorePercentTwo =
      llvm::dyn_cast_or_null<llvm::Instruction>(StorePercentTwoValue);
  ASSERT_TRUE(StorePercentTwo);

  // ret i32 0, !dbg !266, !psr.id !267; | ID: 27
  const auto *RetCallOneValue =
      testingLocInIR(LineColFunOp{.Line = 4,
                                  .Col = 0,
                                  .InFunction = "_Z7callOnev",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  ASSERT_TRUE(RetCallOneValue);
  const auto *RetCallOneInstr =
      llvm::dyn_cast_or_null<llvm::Instruction>(RetCallOneValue);
  ASSERT_TRUE(RetCallOneInstr);

  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentOne},
      getCallToRetFlowValueSet(RetCallOneInstr, AliasImpl, StorePercentOne));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentOne},
      getCallToRetFlowValueSet(RetCallOneInstr, NoAliasImpl, StorePercentOne));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentOne},
      getCallToRetFlowValueSet(RetCallOneInstr, RASImpl, StorePercentOne));

  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentTwo},
      getCallToRetFlowValueSet(RetCallOneInstr, AliasImpl, StorePercentTwo));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentTwo},
      getCallToRetFlowValueSet(RetCallOneInstr, NoAliasImpl, StorePercentTwo));
  EXPECT_EQ(
      std::set<const llvm::Value *>{StorePercentTwo},
      getCallToRetFlowValueSet(RetCallOneInstr, RASImpl, StorePercentTwo));

  EXPECT_EQ(
      std::set<const llvm::Value *>{RetCallOneInstr},
      getCallToRetFlowValueSet(RetCallOneInstr, AliasImpl, RetCallOneInstr));
  EXPECT_EQ(
      std::set<const llvm::Value *>{RetCallOneInstr},
      getCallToRetFlowValueSet(RetCallOneInstr, NoAliasImpl, RetCallOneInstr));
  EXPECT_EQ(
      std::set<const llvm::Value *>{RetCallOneInstr},
      getCallToRetFlowValueSet(RetCallOneInstr, RASImpl, RetCallOneInstr));
}

}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
