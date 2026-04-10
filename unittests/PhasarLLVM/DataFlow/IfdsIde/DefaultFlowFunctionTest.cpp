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

template <typename ImplT>
std::set<const llvm::Value *>
getNormalFlowValueSet(const llvm::Instruction *Instr, ImplT &Impl,
                      const llvm::Value *Arg) {
  return Impl.getNormalFlowFunction(Instr, nullptr)->computeTargets(Arg);
}

template <typename ImplT>
std::set<const llvm::Value *>
getCallFlowValueSet(const llvm::Instruction *Instr, ImplT &Impl,
                    const llvm::Value *Arg, const llvm::Function *CalleeFunc) {
  return Impl.getCallFlowFunction(Instr, CalleeFunc)->computeTargets(Arg);
}

template <typename ImplT>
std::set<const llvm::Value *>
getRetFlowValueSet(const llvm::Instruction *Instr, ImplT &Impl,
                   const llvm::Value *Arg, const llvm::Instruction *ExitInst) {
  return Impl.getRetFlowFunction(Instr, nullptr, ExitInst, nullptr)
      ->computeTargets(Arg);
}

template <typename ImplT>
std::set<const llvm::Value *>
getCallToRetFlowValueSet(const llvm::Instruction *Instr, ImplT &Impl,
                         const llvm::Value *Arg) {
  return Impl.getCallToRetFlowFunction(Instr, nullptr, {})->computeTargets(Arg);
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

struct Impls {
  IDEAliasImpl Alias;
  IDENoAliasImpl NoAlias;
  IDEReachableAllocationSitesImpl RAS;

  Impls(LLVMProjectIRDB *IRDB) : Alias(IRDB), NoAlias(IRDB), RAS(IRDB) {}
};

void expectNormalFlow(const std::set<const llvm::Value *> &Expected,
                      const llvm::Instruction *Instr, const llvm::Value *Arg,
                      Impls &I) {
  EXPECT_EQ(Expected, getNormalFlowValueSet(Instr, I.Alias, Arg));
  EXPECT_EQ(Expected, getNormalFlowValueSet(Instr, I.NoAlias, Arg));
  EXPECT_EQ(Expected, getNormalFlowValueSet(Instr, I.RAS, Arg));
}

void expectCallFlow(const std::set<const llvm::Value *> &Expected,
                    const llvm::Instruction *Instr, const llvm::Value *Arg,
                    const llvm::Function *Callee, Impls &I) {
  EXPECT_EQ(Expected, getCallFlowValueSet(Instr, I.Alias, Arg, Callee));
  EXPECT_EQ(Expected, getCallFlowValueSet(Instr, I.NoAlias, Arg, Callee));
  EXPECT_EQ(Expected, getCallFlowValueSet(Instr, I.RAS, Arg, Callee));
}

void expectRetFlow(const std::set<const llvm::Value *> &Expected,
                   const llvm::Instruction *CallSite, const llvm::Value *Arg,
                   const llvm::Instruction *ExitInst, Impls &I) {
  EXPECT_EQ(Expected, getRetFlowValueSet(CallSite, I.Alias, Arg, ExitInst));
  EXPECT_EQ(Expected, getRetFlowValueSet(CallSite, I.NoAlias, Arg, ExitInst));
  EXPECT_EQ(Expected, getRetFlowValueSet(CallSite, I.RAS, Arg, ExitInst));
}

void expectCallToRetFlow(const std::set<const llvm::Value *> &Expected,
                         const llvm::Instruction *Instr, const llvm::Value *Arg,
                         Impls &I) {
  EXPECT_EQ(Expected, getCallToRetFlowValueSet(Instr, I.Alias, Arg));
  EXPECT_EQ(Expected, getCallToRetFlowValueSet(Instr, I.NoAlias, Arg));
  EXPECT_EQ(Expected, getCallToRetFlowValueSet(Instr, I.RAS, Arg));
}

using GTMap = std::vector<unittest::TestingSrcLocation>;

TEST(PureFlow, NormalFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_01_cpp_dbg.ll"});
  Impls I(&IRDB);

  // store i32 %0, ptr %.addr, align 4
  const auto *ValueStorePercent0 =
      testingLocInIR(LineColFunOp{.Line = 3,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercent0 =
      llvm::dyn_cast<llvm::Instruction>(ValueStorePercent0);
  ASSERT_TRUE(StorePercent0);

  // store ptr %1, ptr %.addr1, align 8
  const auto *ValueStorePercent1 =
      testingLocInIR(LineColFunOp{.Line = 4,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercent1 =
      llvm::dyn_cast<llvm::Instruction>(ValueStorePercent1);
  ASSERT_TRUE(StorePercent1);

  // store ptr %One, ptr %OnePtr, align 8, !dbg !225
  const auto *ValueStorePercentOne =
      testingLocInIR(LineColFunOp{.Line = 4,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentOne =
      llvm::dyn_cast<llvm::Instruction>(ValueStorePercentOne);
  ASSERT_TRUE(StorePercentOne);

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !228
  const auto *ValueStorePercentTwo =
      testingLocInIR(LineColFunOp{.Line = 5,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentTwo =
      llvm::dyn_cast<llvm::Instruction>(ValueStorePercentTwo);
  ASSERT_TRUE(StorePercentTwo);

  expectNormalFlow({StorePercent0}, StorePercent0, StorePercent0, I);
  expectNormalFlow({StorePercent1}, StorePercent1, StorePercent1, I);
  expectNormalFlow({StorePercentOne}, StorePercentOne, StorePercentOne, I);
  expectNormalFlow({StorePercentTwo}, StorePercentTwo, StorePercentTwo, I);
}

TEST(PureFlow, NormalFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_02_cpp_dbg.ll"});
  Impls I(&IRDB);

  const auto *MainFunc = IRDB.getFunction("main");
  ASSERT_TRUE(MainFunc);

  // store i32 %0, ptr %.addr, align 4, !psr.id !227; | ID: 8
  const auto *ValueStorePercent0 =
      testingLocInIR(LineColFunOp{.Line = 5,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercent0 =
      llvm::dyn_cast<llvm::Instruction>(ValueStorePercent0);
  ASSERT_TRUE(StorePercent0);

  // store ptr %1, ptr %.addr1, align 8, !psr.id !231; | ID: 10
  const auto *ValueStorePercent1 =
      testingLocInIR(LineColFunOp{.Line = 5,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercent1 =
      llvm::dyn_cast<llvm::Instruction>(ValueStorePercent1);
  ASSERT_TRUE(StorePercent1);

  expectNormalFlow({StorePercent0}, StorePercent0, StorePercent0, I);
  expectNormalFlow({StorePercent0}, StorePercent0, StorePercent0,
                   I); // duplicate - issue #6
  expectNormalFlow({StorePercent1}, StorePercent1, StorePercent1, I);
}

TEST(PureFlow, NormalFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_03_cpp_dbg.ll"});
  Impls I(&IRDB);

  // %One = alloca i32, align 4, !psr.id !222; | ID: 3
  const auto *PercentOne =
      testingLocInIR(LineColFunOp{.Line = 8,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);

  // %ForGEP = alloca %struct.StructOne, align 4, !psr.id !227; | ID: 8
  const auto *PercentForGEP =
      testingLocInIR(LineColFunOp{.Line = 13,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);

  // %2 = load i32, ptr %One, align 4, !dbg !245, !psr.id !246; | ID: 18
  const auto *ValueLoadPercent2 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *LoadPercent2 =
      llvm::dyn_cast<llvm::Instruction>(ValueLoadPercent2);
  ASSERT_TRUE(LoadPercent2);

  expectNormalFlow({PercentOne, LoadPercent2}, LoadPercent2, PercentOne, I);

  // %3 = load i32, ptr %One, align 4, !dbg !251, !psr.id !252; | ID: 21
  const auto *ValueLoadPercent3 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *LoadPercent3 =
      llvm::dyn_cast<llvm::Instruction>(ValueLoadPercent3);
  ASSERT_TRUE(LoadPercent3);
  expectNormalFlow({PercentOne, LoadPercent3}, LoadPercent3, PercentOne, I);

  // %tobool = icmp ne i32 %3, 0, !dbg !251, !psr.id !253; | ID: 22
  const auto *ValuePercenttobool =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *Percenttobool =
      llvm::dyn_cast<llvm::Instruction>(ValuePercenttobool);
  ASSERT_TRUE(Percenttobool);

  // %lnot = xor i1 %tobool, true, !dbg !254, !psr.id !255; | ID: 23
  const auto *Percentlnot =
      llvm::dyn_cast<llvm::Instruction>(ValuePercenttobool);
  ASSERT_TRUE(Percentlnot);

  expectNormalFlow({Percenttobool, Percentlnot}, Percentlnot, Percenttobool, I);

  // %4 = load i32, ptr %One, align 4, !dbg !261, !psr.id !262; | ID: 27
  const auto *ValueLoadPercent4 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *LoadPercent4 =
      llvm::dyn_cast<llvm::Instruction>(ValueLoadPercent4);
  ASSERT_TRUE(LoadPercent4);

  expectNormalFlow({PercentOne, LoadPercent4}, LoadPercent4, PercentOne, I);

  // %5 = load i32, ptr %One, align 4, !dbg !263, !psr.id !264; | ID: 28
  const auto *ValueLoadPercent5 =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *LoadPercent5 =
      llvm::dyn_cast<llvm::Instruction>(ValueLoadPercent5);
  ASSERT_TRUE(LoadPercent5);

  expectNormalFlow({PercentOne, LoadPercent5}, LoadPercent5, PercentOne, I);

  // %add = add nsw i32 %4, %5, !dbg !265, !psr.id !266; | ID: 29
  const auto *ValueAdd =
      testingLocInIR(LineColFunOp{.Line = 9,
                                  .Col = 18,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *Add = llvm::dyn_cast<llvm::Instruction>(ValueAdd);
  ASSERT_TRUE(Add);

  expectNormalFlow({LoadPercent4, Add}, Add, LoadPercent4, I);
  expectNormalFlow({LoadPercent5, Add}, Add, LoadPercent5, I);

  // %One2 = getelementptr inbounds %struct.StructOne, ptr %ForGEP, i32 0, i32
  // 0, !dbg !282, !psr.id !283; | ID: 37
  const auto *ValuePercentOne2 =
      testingLocInIR(LineColFunOp{.Line = 14,
                                  .Col = 20,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::GetElementPtr},
                     IRDB);
  const auto *PercentOne2 = llvm::dyn_cast<llvm::Instruction>(ValuePercentOne2);
  ASSERT_TRUE(PercentOne2);

  expectNormalFlow({PercentForGEP, PercentOne2}, PercentOne2, PercentForGEP, I);

  // %6 = load i32, ptr %One2, align 4, !dbg !282, !psr.id !284; | ID: 38
  const auto *ValuePercent6 =
      testingLocInIR(LineColFunOp{.Line = 14,
                                  .Col = 20,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *Percent6 = llvm::dyn_cast<llvm::Instruction>(ValuePercent6);
  ASSERT_TRUE(Percent6);

  expectNormalFlow({PercentOne2, Percent6}, Percent6, PercentOne2, I);
}

TEST(PureFlow, NormalFlow04) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_04_cpp_dbg.ll"});
  Impls I(&IRDB);

  // %Deref1 = alloca i32, align 4, !psr.id !222; | ID: 7
  const auto *PercentDeref1 =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  // %Deref2 = alloca i32, align 4, !psr.id !223; | ID: 8
  const auto *PercentDeref2 =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  // %Deref3 = alloca i32, align 4, !psr.id !224; | ID: 9
  const auto *PercentDeref3 =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  // %3 = load i32, ptr %2, align 4, !dbg !258, !psr.id !259; | ID: 25
  const auto *Percent3 =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 16,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  // %6 = load i32, ptr %5, align 4, !dbg !268, !psr.id !269; | ID: 30
  const auto *Percent6 =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 16,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  // %10 = load i32, ptr %9, align 4, !dbg !280, !psr.id !281; | ID: 36
  const auto *Percent10 =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 16,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  // store i32 %3, ptr %Deref1, align 4, !dbg !254, !psr.id !260; | ID: 26
  const auto *StorePercent3Value =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercent3 =
      llvm::dyn_cast<llvm::Instruction>(StorePercent3Value);
  ASSERT_TRUE(StorePercent3);

  expectNormalFlow({Percent3, PercentDeref1}, StorePercent3, Percent3, I);
  expectNormalFlow({}, StorePercent3, PercentDeref1, I);

  // store i32 %6, ptr %Deref2, align 4, !dbg !262, !psr.id !270; | ID: 31
  const auto *StorePercent6Value =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercent6 =
      llvm::dyn_cast<llvm::Instruction>(StorePercent6Value);
  ASSERT_TRUE(StorePercent6);

  expectNormalFlow({PercentDeref2, Percent6}, StorePercent6, Percent6, I);
  expectNormalFlow({}, StorePercent6, PercentDeref2, I);

  // store i32 %10, ptr %Deref3, align 4, !dbg !272, !psr.id !282; | ID: 37
  const auto *StorePercent10Value =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercent10 =
      llvm::dyn_cast<llvm::Instruction>(StorePercent10Value);
  ASSERT_TRUE(StorePercent10);

  expectNormalFlow({PercentDeref3, Percent10}, StorePercent10, Percent10, I);
  expectNormalFlow({}, StorePercent10, PercentDeref3, I);
}

/*
  CallFlow
*/

TEST(PureFlow, CallFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_01_cpp_dbg.ll"});
  Impls I(&IRDB);

  // call void @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg !261, !psr.id
  // !262; | ID: 26
  const auto *CallValue =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  const auto *Call = llvm::dyn_cast<llvm::Instruction>(CallValue);
  ASSERT_TRUE(Call);

  auto FuncTSL = FuncByName{.FuncName = "_Z4callii"};
  const auto *CallFuncValue = testingLocInIR(FuncTSL, IRDB);
  const auto *CallFunc = llvm::dyn_cast<llvm::Function>(CallFuncValue);
  ASSERT_TRUE(CallFunc);
  const auto *Param0 = CallFunc->getArg(0);
  ASSERT_TRUE(Param0);
  const auto *Param1 = CallFunc->getArg(1);
  ASSERT_TRUE(Param1);

  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Call);
  ASSERT_TRUE(CallSite);
  expectCallFlow({Param0}, Call, CallSite->getArgOperand(0), CallFunc, I);
  expectCallFlow({Param1}, Call, CallSite->getArgOperand(1), CallFunc, I);
}

TEST(PureFlow, CallFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_02_cpp_dbg.ll"});
  Impls I(&IRDB);

  // call void @_Z4callPi(ptr noundef %2), !dbg !307, !psr.id !308; | ID: 50
  const auto *CallValue =
      testingLocInIR(LineColFunOp{.Line = 19,
                                  .Col = 3,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  const auto *CallInstr = llvm::dyn_cast<llvm::Instruction>(CallValue);
  ASSERT_TRUE(CallInstr);
  // call function
  const auto *CallFuncValue =
      testingLocInIR(FuncByName{.FuncName = "_Z4callPi"}, IRDB);
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
  const auto *SecondCallInstr =
      llvm::dyn_cast<llvm::Instruction>(SecondCallValue);
  ASSERT_TRUE(SecondCallInstr);
  // second call function
  const auto *SecondCallFuncValue =
      testingLocInIR(FuncByName{.FuncName = "_Z10secondCallPiPS_PS0_"}, IRDB);
  const auto *SecondCallFunc =
      llvm::dyn_cast<llvm::Function>(SecondCallFuncValue);
  ASSERT_TRUE(SecondCallFunc);

  const auto *CSCallFunc = llvm::cast<llvm::CallBase>(CallInstr);
  const auto *CSSecondCallFunc = llvm::cast<llvm::CallBase>(SecondCallInstr);

  expectCallFlow({CallFunc->getArg(0)}, CallInstr, CSCallFunc->getArgOperand(0),
                 CallFunc, I);

  expectCallFlow({SecondCallFunc->getArg(0)}, SecondCallInstr,
                 CSSecondCallFunc->getArgOperand(0), SecondCallFunc, I);
  expectCallFlow({SecondCallFunc->getArg(1)}, SecondCallInstr,
                 CSSecondCallFunc->getArgOperand(1), SecondCallFunc, I);
  expectCallFlow({SecondCallFunc->getArg(2)}, SecondCallInstr,
                 CSSecondCallFunc->getArgOperand(2), SecondCallFunc, I);
}

/*
  RetFlow
*/

TEST(PureFlow, RetFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_01_cpp_dbg.ll"});
  Impls I(&IRDB);

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
  const auto *RetValGetTwoInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValGetTwo);
  ASSERT_TRUE(RetValGetTwoInstr);

  expectRetFlow({}, PercentCallGetTwo, PercentTwo, RetValGetTwoInstr, I);

  // %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  // !281, !psr.id !282; | ID: 36
  const auto *PercentCallValue =
      testingLocInIR(LineColFunOp{.Line = 15,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  const auto *PercentCall = llvm::dyn_cast<llvm::Instruction>(PercentCallValue);
  ASSERT_TRUE(PercentCall);
  // ret i32 %add, !dbg !240, !psr.id !241; | ID: 14
  const auto *RetValCallValue =
      testingLocInIR(LineColFunOp{.Line = 7,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetValCall = llvm::dyn_cast<llvm::Instruction>(RetValCallValue);
  ASSERT_TRUE(RetValCall);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");
  ASSERT_TRUE(FuncZ4callii);
  ASSERT_TRUE(FuncZ4callii->getArg(0));
  ASSERT_TRUE(FuncZ4callii->getArg(1));

  expectRetFlow({}, PercentCall, FuncZ4callii->getArg(0), RetValCall, I);
  expectRetFlow({}, PercentCall, FuncZ4callii->getArg(1), RetValCall, I);

  // negative tests
  expectRetFlow({}, PercentCallGetTwo, FuncZ4callii->getArg(1),
                RetValGetTwoInstr, I);
}

TEST(PureFlow, RetFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_02_cpp_dbg.ll"});
  Impls I(&IRDB);

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

  expectRetFlow({}, PercentCallGetTwo, PercentTwo, RetValGetTwoInstr, I);

  // %call1 = call noundef i32 @_Z8newThreev(), !dbg !247, !psr.id !248; | ID:
  // 18
  const auto *PercentCall1 =
      testingLocInIR(LineColFunOp{.Line = 14,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
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
  const auto *RetValNewThreeInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValNewThree);
  ASSERT_TRUE(RetValNewThreeInstr);

  expectRetFlow({}, PercentCallInstr1, PercentTwo, RetValNewThreeInstr, I);

  //  %call = call noundef i32 @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg
  //  !298, !psr.id !299; | ID: 45
  const auto *PercentCallValue =
      testingLocInIR(LineColFunOp{.Line = 24,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  const auto *PercentCall = llvm::dyn_cast<llvm::Instruction>(PercentCallValue);
  ASSERT_TRUE(PercentCall);
  // ret i32 %add, !dbg !257, !psr.id !258; | ID: 23
  const auto *RetAddValue =
      testingLocInIR(LineColFunOp{.Line = 16,
                                  .Col = 0,
                                  .InFunction = "_Z4callii",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetAdd = llvm::dyn_cast<llvm::Instruction>(RetAddValue);
  ASSERT_TRUE(RetAdd);
  const auto *FuncZ4callii = IRDB.getFunction("_Z4callii");
  ASSERT_TRUE(FuncZ4callii);
  ASSERT_TRUE(FuncZ4callii->getArg(0));
  ASSERT_TRUE(FuncZ4callii->getArg(1));

  expectRetFlow({}, PercentCall, FuncZ4callii->getArg(0), RetAdd, I);
  expectRetFlow({}, PercentCall, FuncZ4callii->getArg(1), RetAdd, I);

  // negative tests
  expectRetFlow({}, PercentCallGetTwo, FuncZ4callii->getArg(1),
                RetValGetTwoInstr, I);
}

TEST(PureFlow, RetFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_03_cpp_dbg.ll"});
  Impls I(&IRDB);

  // %ThreeInCall = alloca i32, align 4, !psr.id !254; | ID: 15
  const auto *PercentThreeInCall =
      testingLocInIR(LineColFunOp{.Line = 16,
                                  .Col = 0,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);

  // %ThreePtrInCall = alloca ptr, align 8, !psr.id !255; | ID: 16
  const auto *PercentThreePtrInCall =
      testingLocInIR(LineColFunOp{.Line = 17,
                                  .Col = 0,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);

  // %0 = load ptr, ptr %ThreePtrInCall, align 8, !dbg !280, !psr.id !281; | ID:
  // 29
  const auto *ValuePercent0 =
      testingLocInIR(LineColFunOp{.Line = 18,
                                  .Col = 40,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Load},
                     IRDB);
  const auto *Percent0 = llvm::dyn_cast<llvm::Instruction>(ValuePercent0);
  ASSERT_TRUE(Percent0);

  // %call = call noundef ptr @_Z8newThreePKi(ptr noundef %0), !dbg !282,
  // !psr.id !283; | ID: 30
  const auto *ValuePercentCall =
      testingLocInIR(LineColFunOp{.Line = 18,
                                  .Col = 31,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  const auto *PercentCall = llvm::dyn_cast<llvm::Instruction>(ValuePercentCall);
  ASSERT_TRUE(PercentCall);

  // ret ptr %1, !dbg !234, !psr.id !235; | ID: 9
  const auto *RetValNewThreeValue =
      testingLocInIR(LineColFunOp{.Line = 7,
                                  .Col = 0,
                                  .InFunction = "_Z8newThreePKi",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetValNewThreeInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValNewThreeValue);
  ASSERT_TRUE(RetValNewThreeInstr);

  const auto *FunctionZ8newThreePKi = IRDB.getFunction("_Z8newThreePKi");
  const auto &Got =
      getRetFlowValueSet(PercentCall, I.Alias, FunctionZ8newThreePKi->getArg(0),
                         RetValNewThreeInstr);
  EXPECT_EQ(std::set<const llvm::Value *>{Percent0},
            getRetFlowValueSet(PercentCall, I.NoAlias,
                               FunctionZ8newThreePKi->getArg(0),
                               RetValNewThreeInstr));
  EXPECT_EQ((std::set<const llvm::Value *>{Percent0, PercentThreeInCall,
                                           PercentCall}),
            getRetFlowValueSet(PercentCall, I.RAS,
                               FunctionZ8newThreePKi->getArg(0),
                               RetValNewThreeInstr));

  EXPECT_EQ(
      (std::set<const llvm::Value *>{PercentThreeInCall, PercentThreePtrInCall,
                                     Percent0, PercentCall}),
      Got)
      << stringifyValueSet(Got);

  // %call3 = call noundef ptr @_Z10getFourPtrv(), !dbg !304, !psr.id !305; |
  // ID: 42
  const auto *PercentCall3Value =
      testingLocInIR(LineColFunOp{.Line = 20,
                                  .Col = 61,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Call},
                     IRDB);
  const auto *PercentCall3 =
      llvm::dyn_cast<llvm::Instruction>(PercentCall3Value);
  ASSERT_TRUE(PercentCall3);

  // ret ptr @GlobalFour, !dbg !246, !psr.id !247; | ID: 11
  const auto *RetValGetFourValue =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "_Z10getFourPtrv",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetValGetFourInstr =
      llvm::dyn_cast<llvm::Instruction>(RetValGetFourValue);
  ASSERT_TRUE(RetValGetFourInstr);

  const auto *RetValGlobalFourSecondValue =
      testingLocInIR(LineColFunOp{.Line = 12,
                                  .Col = 0,
                                  .InFunction = "_Z11getFourAddrv",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
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
  const auto *PercentCall5 =
      llvm::dyn_cast<llvm::Instruction>(PercentCall5Value);
  ASSERT_TRUE(PercentCall5);

  // ret i32 %add6, !dbg !315, !psr.id !316; | ID: 48
  const auto *RetValAdd6Value =
      testingLocInIR(LineColFunOp{.Line = 20,
                                  .Col = 0,
                                  .InFunction = "_Z4callRiPKi",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetValAdd6 = llvm::dyn_cast<llvm::Instruction>(RetValAdd6Value);
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
  const auto *PercentCallZeroOne =
      llvm::dyn_cast<llvm::Instruction>(PercentCallZeroOneValue);
  ASSERT_TRUE(PercentCallZeroOne);
  const auto *FuncZ4callRiPKi = IRDB.getFunction("_Z4callRiPKi");
  // %Zero = alloca i32, align 4, !psr.id !324; | ID: 52
  const auto *PercentZeroValue =
      testingLocInIR(LineColFunOp{.Line = 25,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  const auto *PercentZero = llvm::dyn_cast<llvm::Instruction>(PercentZeroValue);
  ASSERT_TRUE(PercentZero);

  // %One = alloca i32, align 4, !psr.id !325; | ID: 53
  const auto *PercentOneValue =
      testingLocInIR(LineColFunOp{.Line = 26,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Alloca},
                     IRDB);
  const auto *PercentOne = llvm::dyn_cast<llvm::Instruction>(PercentOneValue);
  ASSERT_TRUE(PercentOne);

  expectRetFlow({PercentZero}, PercentCallZeroOne, FuncZ4callRiPKi->getArg(0),
                RetValAdd6, I);
  expectRetFlow({PercentOne}, PercentCallZeroOne, FuncZ4callRiPKi->getArg(1),
                RetValAdd6, I);
}

/*
  CallToRetFlow
*/

TEST(PureFlow, CallToRetFlow01) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles +
       "pure_flow/call_to_ret_flow/call_to_ret_flow_01_cpp_dbg.ll"});
  Impls I(&IRDB);

  // store i32 0, ptr %Zero, align 4, !dbg !252, !psr.id !254; | ID: 22
  const auto *StorePercentZeroValue =
      testingLocInIR(LineColFunOp{.Line = 6,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentZero =
      llvm::dyn_cast<llvm::Instruction>(StorePercentZeroValue);
  ASSERT_TRUE(StorePercentZero);

  // store i32 1, ptr %One, align 4, !dbg !256, !psr.id !258; | ID: 24
  const auto *StorePercentOneValue =
      testingLocInIR(LineColFunOp{.Line = 6,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentOne =
      llvm::dyn_cast<llvm::Instruction>(StorePercentOneValue);
  ASSERT_TRUE(StorePercentOne);

  // ret i32 0, !dbg !269, !psr.id !270; | ID: 30
  const auto *RetMainValue =
      testingLocInIR(LineColFunOp{.Line = 11,
                                  .Col = 0,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetMainInstr = llvm::dyn_cast<llvm::Instruction>(RetMainValue);
  ASSERT_TRUE(RetMainInstr);

  expectCallToRetFlow({StorePercentZero}, RetMainInstr, StorePercentZero, I);
  expectCallToRetFlow({StorePercentOne}, RetMainInstr, StorePercentOne, I);
  expectCallToRetFlow({RetMainInstr}, RetMainInstr, RetMainInstr, I);
}

TEST(PureFlow, CallToRetFlow02) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles +
       "pure_flow/call_to_ret_flow/call_to_ret_flow_02_cpp_dbg.ll"});
  Impls I(&IRDB);

  // store i32 3, ptr %Three, align 4, !dbg !223, !psr.id !225; | ID: 5
  const auto *StorePercentThreeValue =
      testingLocInIR(LineColFunOp{.Line = 6,
                                  .Col = 7,
                                  .InFunction = "_Z4callv",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentThree =
      llvm::dyn_cast<llvm::Instruction>(StorePercentThreeValue);
  ASSERT_TRUE(StorePercentThree);

  // store i32 4, ptr %Four, align 4, !dbg !229, !psr.id !231; | ID: 8
  const auto *StorePercentFourValue =
      testingLocInIR(LineColFunOp{.Line = 8,
                                  .Col = 7,
                                  .InFunction = "_Z4callv",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentFour =
      llvm::dyn_cast<llvm::Instruction>(StorePercentFourValue);
  ASSERT_TRUE(StorePercentFour);

  // ret void, !dbg !234, !psr.id !235; | ID: 10
  const auto *RetCallValue =
      testingLocInIR(LineColFunOp{.Line = 10,
                                  .Col = 0,
                                  .InFunction = "_Z4callv",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetCall = llvm::dyn_cast<llvm::Instruction>(RetCallValue);
  ASSERT_TRUE(RetCall);

  expectCallToRetFlow({StorePercentThree}, RetCall, StorePercentThree, I);
  expectCallToRetFlow({StorePercentFour}, RetCall, StorePercentFour, I);

  // store i32 1, ptr %One, align 4, !dbg !255, !psr.id !257; | ID: 22
  const auto *StorePercentOneValue =
      testingLocInIR(LineColFunOp{.Line = 13,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentOne =
      llvm::dyn_cast<llvm::Instruction>(StorePercentOneValue);
  ASSERT_TRUE(StorePercentOne);

  // store i32 2, ptr %Two, align 4, !dbg !261, !psr.id !263; | ID: 25
  const auto *StorePercentTwoValue =
      testingLocInIR(LineColFunOp{.Line = 15,
                                  .Col = 7,
                                  .InFunction = "main",
                                  .OpCode = llvm::Instruction::Store},
                     IRDB);
  const auto *StorePercentTwo =
      llvm::dyn_cast<llvm::Instruction>(StorePercentTwoValue);
  ASSERT_TRUE(StorePercentTwo);

  // ret i32 0, !dbg !266, !psr.id !267; | ID: 27
  const auto *RetCallOneValue =
      testingLocInIR(LineColFunOp{.Line = 4,
                                  .Col = 0,
                                  .InFunction = "_Z7callOnev",
                                  .OpCode = llvm::Instruction::Ret},
                     IRDB);
  const auto *RetCallOneInstr =
      llvm::dyn_cast<llvm::Instruction>(RetCallOneValue);
  ASSERT_TRUE(RetCallOneInstr);

  expectCallToRetFlow({StorePercentOne}, RetCallOneInstr, StorePercentOne, I);
  expectCallToRetFlow({StorePercentTwo}, RetCallOneInstr, StorePercentTwo, I);
  expectCallToRetFlow({RetCallOneInstr}, RetCallOneInstr, RetCallOneInstr, I);
}

}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
