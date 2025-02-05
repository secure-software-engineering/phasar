#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/GenericFlowFunction.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IDEAliasInfoTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
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

// TODO: IDEAliasInfoTabProblem implementieren mit default funktionen
class IDEImpl : public IDEAliasInfoTabulationProblem<DFFAnalysisDomain> {
public:
  IDEImpl(const LLVMProjectIRDB *IRDB)
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

TEST(PureFlow, NormalFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  const auto *IDEight = IRDB.getInstruction(8);
  const auto NFF = Solver.getNormalFlowFunction(IDEight, nullptr);

  const auto *const FuncOfIDEight = IDEight->getFunction();
  const auto *Arg0 = FuncOfIDEight->getArg(0);
  const auto *Test = IRDB.getInstruction(1);

  const auto TestSet = NFF->computeTargets(Arg0);
  const auto TestSet2 = NFF->computeTargets(Test);

  // Falls wir leer erwarten
  EXPECT_EQ(std::set<const llvm::Value *>{}, TestSet);
  // Falls wir z.B. nur Source erwarten
  EXPECT_EQ(std::set<const llvm::Value *>{Arg0}, TestSet);
  // Falls wir Source und getPointerOp erwarten
  EXPECT_EQ(std::set<const llvm::Value *>({Arg0, Test}), TestSet);

  IRDB.emitPreprocessedIR(llvm::outs());
  // TODO: Fabian fragen, das die richtigen Instructions sind?
  // IRDB.getInstruction(7); // store i32 0, ptr %retval, align 4, !psr.id !222
  // IRDB.getInstruction(8); // store i32 %0, ptr %.addr, align 4, !psr.id !223

  // IRDB.getInstruction(13); // store i32 1, ptr %One, align 4, !dbg !232,
  // !psr.id !234
  // IRDB.getInstruction(15); // store i32 2, ptr %Two, align 4,
  // !dbg !236, !psr.id !238
}

TEST(PureFlow, NormalFlow02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_02_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
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
  // GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  // GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

TEST(PureFlow, NormalFlow03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/normal_flow/normal_flow_03_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
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
  // GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  // GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

#if false

TEST(PureFlow, IntCallNoParamsNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_no_params_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

TEST(PureFlow, IntCallOneParamNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_one_param_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

TEST(PureFlow, IntCallPointerParamNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_pointer_param_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

TEST(PureFlow, AllFlowFunctionsNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/all_flow_functions_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

TEST(PureFlow, VoidCallNoParamsNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/void_call_no_params_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

TEST(PureFlow, VoidCallOneParamsNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/void_call_one_param_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}

TEST(PureFlow, VoidCallPointerParamNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/void_call_pointer_param_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow1();
  GenericFlowFunction<DFFAnalysisDomain::n_t> NormalFlow2();
}
#endif

/*
  CallFlow
*/

TEST(PureFlow, CallFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/call_flow/call_flow_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
  // IRDB.getInstruction(8);
  IRDB.emitPreprocessedIR(llvm::outs());
}

/*
  RetFlow
*/

TEST(PureFlow, RetFlow01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/ret_flow/ret_flow_01_cpp_dbg.ll"});
  IDEImpl Solver = IDEImpl(&IRDB);
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
  IDEImpl Solver = IDEImpl(&IRDB);
  IRDB.emitPreprocessedIR(llvm::outs());
}

}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
