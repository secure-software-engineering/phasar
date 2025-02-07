#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/GenericFlowFunction.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IDEAliasInfoTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "llvm/IR/Instruction.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <cstddef>
#include <iterator>
#include <utility>

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

// TODO: Fabian fragen, ob es für die Alias und NoAlias zwei verschiedene
// Testdateien geben soll
class IDENoAliasImpl : public IDEAliasInfoTabulationProblem<DFFAnalysisDomain> {
public:
  IDENoAliasImpl(const LLVMProjectIRDB *IRDB)
      : psr::IDEAliasInfoTabulationProblem<DFFAnalysisDomain>(IRDB, {}, {},
                                                              {}){};
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
  llvm::outs() << "Value Set\n";
  for (const auto *CurrValue : Values) {
    llvm::outs() << *CurrValue << "\n";
  }
}

TEST(PureFlow, NormalFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());
  llvm::outs() << "1\n";

  // store i32 0, ptr %retval, align 4
  const auto *Instr7 = IRDB.getInstruction(7);
  ASSERT_TRUE(Instr7);
  llvm::outs() << "1.2\n";
  const auto NFF7 = NoAliasImpl.getNormalFlowFunction(Instr7, nullptr);
  llvm::outs() << "1.3\n";
  const auto LLVMValueSet7 = NFF7->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet7);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet7);
  llvm::outs() << "1.4\n";

  // store i32 %0, ptr %.addr, align 4
  const auto *Instr8 = IRDB.getInstruction(8);
  ASSERT_TRUE(Instr8);
  const auto NFF8 = NoAliasImpl.getNormalFlowFunction(Instr8, nullptr);
  // TODO: IRDB.getInstruction(0)->getValueID()) isn't correct here, use correct
  // value
  const auto LLVMValueSet8 = NFF8->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet8);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet8);

  // store ptr %1, ptr %.addr1, align 8
  const auto *Instr10 = IRDB.getInstruction(10);
  ASSERT_TRUE(Instr10);
  const auto NFF10 = NoAliasImpl.getNormalFlowFunction(Instr10, nullptr);
  // TODO: IRDB.getInstruction(0)->getValueID()) isn't correct here, use correct
  // value
  const auto LLVMValueSet10 = NFF10->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet10);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet10);

  // store i32 1, ptr %One, align 4, !dbg !220
  const auto *Instr13 = IRDB.getInstruction(13);
  ASSERT_TRUE(Instr13);
  const auto NFF13 = NoAliasImpl.getNormalFlowFunction(Instr13, nullptr);
  // TODO: IRDB.getInstruction(0)->getValueID()) isn't correct here, use correct
  // value
  const auto LLVMValueSet13 = NFF13->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet13);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet13);

  // store i32 2, ptr %Two, align 4, !dbg !222
  const auto *Instr15 = IRDB.getInstruction(15);
  ASSERT_TRUE(Instr15);
  const auto NFF15 = NoAliasImpl.getNormalFlowFunction(Instr15, nullptr);
  // TODO: IRDB.getInstruction(0)->getValueID()) isn't correct here, use correct
  // value
  const auto LLVMValueSet15 = NFF15->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet15);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet15);

  // store ptr %One, ptr %OnePtr, align 8, !dbg !225
  const auto *Instr17 = IRDB.getInstruction(17);
  ASSERT_TRUE(Instr17);
  const auto NFF17 = NoAliasImpl.getNormalFlowFunction(Instr17, nullptr);
  // TODO: IRDB.getInstruction(0)->getValueID()) isn't correct here, use correct
  // value
  const auto LLVMValueSet17 = NFF17->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet17);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet17);

  // store ptr %Two, ptr %TwoAddr, align 8, !dbg !228
  const auto *Instr19 = IRDB.getInstruction(19);
  ASSERT_TRUE(Instr19);
  const auto NFF19 = NoAliasImpl.getNormalFlowFunction(Instr19, nullptr);
  // TODO: IRDB.getInstruction(0)->getValueID()) isn't correct here, use correct
  // value
  const auto LLVMValueSet19 = NFF19->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet19);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet19);

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
  printValueSet(LLVMValueSet7);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet7);
*/

TEST(PureFlow, NormalFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_02_cpp_dbg.ll"});
  IDEAliasImpl Alias = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());
  // TODO: double checken
  // IRDB.getInstruction(3);
  // IRDB.getInstruction(5);
  // IRDB.getInstruction(12);
  // IRDB.getInstruction(28);
  // IRDB.getInstruction(29);
  // IRDB.getInstruction(32);
  // IRDB.getInstruction(34);
  // IRDB.getInstruction(36);

  // TODO: get instr of call function as well

  // TODO: comment instruction here
  const auto *Instr3 = IRDB.getInstruction(3);
  ASSERT_TRUE(Instr3);
  const auto NFF3 = NoAliasImpl.getNormalFlowFunction(Instr3, nullptr);
  const auto LLVMValueSet3 = NFF3->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet3);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet3);

  // TODO: comment instruction here
  const auto *Instr5 = IRDB.getInstruction(5);
  ASSERT_TRUE(Instr5);
  const auto NFF5 = NoAliasImpl.getNormalFlowFunction(Instr5, nullptr);
  const auto LLVMValueSet5 = NFF5->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet5);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet5);

  // TODO: comment instruction here
  const auto *Instr12 = IRDB.getInstruction(12);
  ASSERT_TRUE(Instr12);
  const auto NFF12 = NoAliasImpl.getNormalFlowFunction(Instr12, nullptr);
  const auto LLVMValueSet12 = NFF12->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet12);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet12);

  // TODO: comment instruction here
  const auto *Instr28 = IRDB.getInstruction(28);
  ASSERT_TRUE(Instr28);
  const auto NFF28 = NoAliasImpl.getNormalFlowFunction(Instr28, nullptr);
  const auto LLVMValueSet28 = NFF28->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet28);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet28);

  // TODO: comment instruction here
  const auto *Instr29 = IRDB.getInstruction(29);
  ASSERT_TRUE(Instr29);
  const auto NFF29 = NoAliasImpl.getNormalFlowFunction(Instr29, nullptr);
  const auto LLVMValueSet29 = NFF29->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet29);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet29);

  // TODO: comment instruction here
  const auto *Instr32 = IRDB.getInstruction(32);
  ASSERT_TRUE(Instr32);
  const auto NFF32 = NoAliasImpl.getNormalFlowFunction(Instr32, nullptr);
  const auto LLVMValueSet32 = NFF32->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet32);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet32);

  // TODO: comment instruction here
  const auto *Instr34 = IRDB.getInstruction(34);
  ASSERT_TRUE(Instr34);
  const auto NFF34 = NoAliasImpl.getNormalFlowFunction(Instr34, nullptr);
  const auto LLVMValueSet34 = NFF34->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet34);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet34);

  // TODO: comment instruction here
  const auto *Instr36 = IRDB.getInstruction(36);
  ASSERT_TRUE(Instr36);
  const auto NFF36 = NoAliasImpl.getNormalFlowFunction(Instr36, nullptr);
  const auto LLVMValueSet36 = NFF36->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet36);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet36);
}

TEST(PureFlow, NormalFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_03_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());
  // TODO: double checken
  // IRDB.getInstruction(2);
  // IRDB.getInstruction(4);
  // IRDB.getInstruction(17);
  // IRDB.getInstruction(19);
  // IRDB.getInstruction(21);
  // IRDB.getInstruction(26);
  // IRDB.getInstruction(30);
  // IRDB.getInstruction(47);
  // IRDB.getInstruction(49);
  // IRDB.getInstruction(51);
  // IRDB.getInstruction(53);
  // IRDB.getInstruction(55);
  // IRDB.getInstruction(57);
  // TODO: comment instruction here
  const auto *Instr2 = IRDB.getInstruction(2);
  ASSERT_TRUE(Instr2);
  const auto NFF2 = NoAliasImpl.getNormalFlowFunction(Instr2, nullptr);
  const auto LLVMValueSet2 = NFF2->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet2);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet2);

  // TODO: comment instruction here
  const auto *Instr4 = IRDB.getInstruction(4);
  ASSERT_TRUE(Instr4);
  const auto NFF4 = NoAliasImpl.getNormalFlowFunction(Instr4, nullptr);
  const auto LLVMValueSet4 = NFF4->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet4);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet4);

  // TODO: comment instruction here
  const auto *Instr17 = IRDB.getInstruction(17);
  ASSERT_TRUE(Instr17);
  const auto NFF17 = NoAliasImpl.getNormalFlowFunction(Instr17, nullptr);
  const auto LLVMValueSet17 = NFF17->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet17);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet17);

  // TODO: comment instruction here
  const auto *Instr19 = IRDB.getInstruction(19);
  ASSERT_TRUE(Instr19);
  const auto NFF19 = NoAliasImpl.getNormalFlowFunction(Instr19, nullptr);
  const auto LLVMValueSet19 = NFF19->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet19);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet19);

  // TODO: comment instruction here
  const auto *Instr21 = IRDB.getInstruction(21);
  ASSERT_TRUE(Instr21);
  const auto NFF21 = NoAliasImpl.getNormalFlowFunction(Instr21, nullptr);
  const auto LLVMValueSet21 = NFF21->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet21);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet21);

  // TODO: comment instruction here
  const auto *Instr26 = IRDB.getInstruction(26);
  ASSERT_TRUE(Instr26);
  const auto NFF26 = NoAliasImpl.getNormalFlowFunction(Instr26, nullptr);
  const auto LLVMValueSet26 = NFF26->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet26);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet26);

  // TODO: comment instruction here
  const auto *Instr30 = IRDB.getInstruction(30);
  ASSERT_TRUE(Instr30);
  const auto NFF30 = NoAliasImpl.getNormalFlowFunction(Instr30, nullptr);
  const auto LLVMValueSet30 = NFF30->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet30);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet30);

  // TODO: comment instruction here
  const auto *Instr47 = IRDB.getInstruction(47);
  ASSERT_TRUE(Instr47);
  const auto NFF47 = NoAliasImpl.getNormalFlowFunction(Instr47, nullptr);
  const auto LLVMValueSet47 = NFF47->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet47);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet47);

  // TODO: comment instruction here
  const auto *Instr49 = IRDB.getInstruction(49);
  ASSERT_TRUE(Instr49);
  const auto NFF49 = NoAliasImpl.getNormalFlowFunction(Instr49, nullptr);
  const auto LLVMValueSet49 = NFF49->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet49);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet49);

  // TODO: comment instruction here
  const auto *Instr51 = IRDB.getInstruction(51);
  ASSERT_TRUE(Instr51);
  const auto NFF51 = NoAliasImpl.getNormalFlowFunction(Instr51, nullptr);
  const auto LLVMValueSet51 = NFF51->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet51);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet51);

  // TODO: comment instruction here
  const auto *Instr53 = IRDB.getInstruction(53);
  ASSERT_TRUE(Instr53);
  const auto NFF53 = NoAliasImpl.getNormalFlowFunction(Instr53, nullptr);
  const auto LLVMValueSet53 = NFF53->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet53);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet53);

  // TODO: comment instruction here
  const auto *Instr55 = IRDB.getInstruction(55);
  ASSERT_TRUE(Instr55);
  const auto NFF55 = NoAliasImpl.getNormalFlowFunction(Instr55, nullptr);
  const auto LLVMValueSet55 = NFF55->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet55);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet55);

  // TODO: comment instruction here
  const auto *Instr57 = IRDB.getInstruction(57);
  ASSERT_TRUE(Instr57);
  const auto NFF57 = NoAliasImpl.getNormalFlowFunction(Instr57, nullptr);
  const auto LLVMValueSet57 = NFF57->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet57);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet57);

  // TODO: comment instruction here
  const auto *Instr7 = IRDB.getInstruction(7);
  ASSERT_TRUE(Instr7);
  const auto NFF7 = NoAliasImpl.getNormalFlowFunction(Instr7, nullptr);
  const auto LLVMValueSet7 = NFF7->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet7);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet7);
}

/*
  CallFlow
*/

TEST(PureFlow, CallFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());

  // TODO: call void @_Z4callii(i32 noundef %2, i32 noundef %3), !dbg !233
  const auto *Instr16 = IRDB.getInstruction(16);
  ASSERT_TRUE(Instr16);
  const auto NFF16 = NoAliasImpl.getNormalFlowFunction(Instr16, nullptr);
  const auto LLVMValueSet16 = NFF16->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet16);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet16);
}

TEST(PureFlow, CallFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_02_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IDENoAliasImpl NoAliasImpl = IDENoAliasImpl(&IRDB);

  // %call = call noundef i32 @_Z6getOnev(), !dbg !244
  const auto *Instr18 = IRDB.getInstruction(18);
  ASSERT_TRUE(Instr18);
  const auto NFF18 = NoAliasImpl.getNormalFlowFunction(Instr18, nullptr);
  const auto LLVMValueSet18 = NFF18->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet18);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet18);

  // %call2 = call noundef i32 @_Z6getTwoi(i32 noundef %2), !dbg !247
  const auto *Instr21 = IRDB.getInstruction(21);
  ASSERT_TRUE(Instr21);
  const auto NFF21 = NoAliasImpl.getNormalFlowFunction(Instr21, nullptr);
  const auto LLVMValueSet21 = NFF21->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet21);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet21);

  // %call3 = call noundef i32 @_Z8getThreePKi(ptr noundef %Two), !dbg !251
  const auto *Instr24 = IRDB.getInstruction(24);
  ASSERT_TRUE(Instr24);
  const auto NFF24 = NoAliasImpl.getNormalFlowFunction(Instr24, nullptr);
  const auto LLVMValueSet24 = NFF24->computeTargets(
      IRDB.getValueFromId(IRDB.getInstruction(0)->getValueID()));
  printValueSet(LLVMValueSet24);
  // TODO: determine ground truth
  EXPECT_EQ(std::set<const llvm::Value *>{}, LLVMValueSet24);
}

/*
  RetFlow
*/

TEST(PureFlow, RetFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  // IRDB.getInstruction(8);
  IRDB.emitPreprocessedIR(llvm::outs());
}

/*
  CallToRetFlow
*/

TEST(PureFlow, CallToRetFlow01) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles +
       "pure_flow/call_to_ret_flow/call_to_ret_flow_01_cpp_dbg.ll"});
  IDEAliasImpl AliasImpl = IDEAliasImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());
}

}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
