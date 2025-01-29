#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IDEAliasInfoTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "llvm/IR/Instruction.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

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

// TODO: call flow, ret flow, usw eigene tests
TEST(PureFlow, IntCallNoParamsNormalFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_no_params_01_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);

  int FirstCounter = 0;
  for (const auto &FirstInst : IRDB.getAllInstructions()) {
    int SecondCounter = 1;
    for (const auto &SecondInst : IRDB.getAllInstructions()) {
      if (FirstCounter > SecondCounter) {
        continue;
      }
      const auto &NormalFlowFunc =
          IDETP.getNormalFlowFunction(FirstInst, SecondInst);
      llvm::outs() << "NormalFlowFunc:\n";
      llvm::outs() << NormalFlowFunc.get();
      SecondCounter++;
    }
    llvm::outs() << FirstInst << "\n";
    FirstCounter++;
  }
}

TEST(PureFlow, IntCallNoParamsCallFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_no_params_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);
  // IDETP.getCallFlowFunction();
}

TEST(PureFlow, IntCallNoParamsRetFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_no_params_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);
  // IDETP.getRetFlowFunction();
}

TEST(PureFlow, IntCallNoParamsCallToRetFlow) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_no_params_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);
  // IDETP.getCallToRetFlowFunction();
}
#if false
TEST(PureFlow, IntCallOneParams) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/int_call_one_param_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);
  IDETP.getNormalFlowFunction();
  IDETP.getCallFlowFunction();
  IDETP.getRetFlowFunction();
  IDETP.getCallToRetFlowFunction();
}

TEST(PureFlow, VoidCallNoParams) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/void_call_no_params_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);
  IDETP.getNormalFlowFunction();
  IDETP.getCallFlowFunction();
  IDETP.getRetFlowFunction();
  IDETP.getCallToRetFlowFunction();
}

TEST(PureFlow, VoidCallOneParams) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/void_call_one_param_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);
  IDETP.getNormalFlowFunction();
  IDETP.getCallFlowFunction();
  IDETP.getRetFlowFunction();
  IDETP.getCallToRetFlowFunction();
}

TEST(PureFlow, DeepCallNoParams) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "pure_flow/deep_call_no_params_cpp_dbg.ll"});
  IDEImpl IDETP = IDEImpl(&IRDB);
  IDETP.getNormalFlowFunction();
  IDETP.getCallFlowFunction();
  IDETP.getRetFlowFunction();
  IDETP.getCallToRetFlowFunction();
}
#endif

}; // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
