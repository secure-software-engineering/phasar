// TODO: try running this unit test

#include <experimental/filesystem>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/Utils/Logger.h"

#define DELETE(x)                                                              \
  if (x) {                                                                     \
    delete (x);                                                                \
    (x) = nullptr;                                                             \
  }

using namespace CCPP;
using namespace CCPP::LCUtils;
typedef std::tuple<const IDELinearConstantPropagation::v_t, unsigned, unsigned>
    groundTruth_t;
/* ============== TEST FIXTURE ============== */

class LCPTest : public ::testing::Test {

protected:
  const std::string pathToLLFiles = "../../../test/llvmIR/LCP/";

  psr::ProjectIRDB *irdb = nullptr;
  psr::LLVMTypeHierarchy *th = nullptr;
  psr::LLVMBasedICFG *icfg = nullptr;
  IDELinearConstantPropagation *LCP = nullptr;
  psr::LLVMIDESolver<const llvm::Value *, IDELinearConstantPropagation::v_t,
                     psr::LLVMBasedICFG &> *solver = nullptr;

  LCPTest() {}
  virtual ~LCPTest() {}

  void Initialize(const std::string &llFile, bool generateOutput = false,
                  size_t maxSetSize = 2) {
    ASSERT_EQ(true, std::filesystem::exists(pathToLLFiles + llFile))
        << "for file " << llFile;

    irdb =
        new psr::ProjectIRDB({pathToLLFiles + llFile}, psr::IRDBOptions::WPA);

    irdb->preprocessIR();
    // irdb->print();
    th = new psr::LLVMTypeHierarchy(*irdb);

    icfg = new psr::LLVMBasedICFG(*th, *irdb, psr::CallGraphAnalysisType::RTA,
                                  {"main"});

    LCP = new IDELinearConstantPropagation(*icfg, *th, *irdb, maxSetSize);

    solver = new psr::LLVMIDESolver<const llvm::Value *,
                                    IDELinearConstantPropagation::v_t,
                                    psr::LLVMBasedICFG &>(*LCP, generateOutput);
    // try {
    solver->solve();
    /*} catch (std::exception &e) {

      std::cerr << "ERROR: " << e.what()
                << " at: " << boost::stacktrace::stacktrace() << std::endl;
    }
    std::cout << "Hello" << std::endl;*/
  }

  void SetUp() override {
    bl::core::get()->set_logging_enabled(false);
    psr::ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    DELETE(solver);
    DELETE(LCP);
    DELETE(icfg);
    DELETE(th);
    DELETE(irdb);
  }

  //  compare results
  /// \brief compares the computed results with every given tuple (value,
  /// alloca, inst)
  void compareResults(const std::vector<groundTruth_t> &expected) {
    for (auto &[val, vrId, instId] : expected) {
      auto vr = irdb->getInstruction(vrId);
      auto inst = irdb->getInstruction(instId);
      ASSERT_NE(nullptr, vr);
      ASSERT_NE(nullptr, inst);
      auto result = solver->resultAt(inst, vr);
      EXPECT_EQ(val, result);
    }
  }

}; // class Fixture

TEST_F(LCPTest, SimpleTest) {
  Initialize("SimpleTest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(10)}, 3, 20});
  groundTruth.push_back({{EdgeValue(15)}, 4, 20});
  compareResults(groundTruth);
}

TEST_F(LCPTest, BranchTest) {
  Initialize("BranchTest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(25), EdgeValue(43)}, 3, 22});
  groundTruth.push_back({{EdgeValue(24)}, 4, 22});
  compareResults(groundTruth);
}

TEST_F(LCPTest, FPtest) {
  Initialize("FPtest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(4.5)}, 1, 16});
  groundTruth.push_back({{EdgeValue(2.0)}, 2, 16});
  compareResults(groundTruth);
}
TEST_F(LCPTest, StringTest) {
  Initialize("StringTest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue("Hello, World")}, 2, 8});
  groundTruth.push_back({{EdgeValue("Hello, World")}, 3, 8});
  compareResults(groundTruth);
}
TEST_F(LCPTest, StringBranchTest) {
  Initialize("StringBranchTest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back(
      {{EdgeValue("Hello, World"), EdgeValue("Hello Hello")}, 3, 15});
  groundTruth.push_back({{EdgeValue("Hello Hello")}, 4, 15});
  compareResults(groundTruth);
}
TEST_F(LCPTest, FloatDivisionTest) {
  Initialize("FloatDivision.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(nullptr)}, 1, 24}); // i
  groundTruth.push_back({{EdgeValue(1.0)}, 2, 24});     // j
  groundTruth.push_back({{EdgeValue(-7.0)}, 3, 24});    // k
  compareResults(groundTruth);
}
TEST_F(LCPTest, SimpleFunctionTest) {
  Initialize("SimpleFunctionTest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(48)}, 10, 31});      // i
  groundTruth.push_back({{EdgeValue(nullptr)}, 11, 31}); // j
  compareResults(groundTruth);
}
TEST_F(LCPTest, GlobalVariableTest) {
  Initialize("GlobalVariableTest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(50)}, 7, 13});       // i
  groundTruth.push_back({{EdgeValue(nullptr)}, 10, 13}); // j
  compareResults(groundTruth);
}
TEST_F(LCPTest, Imprecision) {
  // bl::core::get()->set_logging_enabled(true);
  Initialize("Imprecision.ll", false, 2);
  auto xInst = irdb->getInstruction(0); // foo.x
  auto yInst = irdb->getInstruction(1); // foo.y
  auto barInst = irdb->getInstruction(7);

  std::cout << "foo.x = " << solver->resultAt(barInst, xInst) << std::endl;
  std::cout << "foo.y = " << solver->resultAt(barInst, yInst) << std::endl;

  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(1), EdgeValue(2)}, 0, 7}); // i
  groundTruth.push_back({{EdgeValue(2), EdgeValue(3)}, 1, 7}); // j
  compareResults(groundTruth);
}
TEST_F(LCPTest, ReturnConstTest) {
  Initialize("ReturnConstTest.ll", false);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(43)}, 7, 8}); // i
  compareResults(groundTruth);
}
TEST_F(LCPTest, NullTest) {
  Initialize("NullTest.ll", true);
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue("")}, 4, 5}); // foo(null)
  compareResults(groundTruth);
}
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
