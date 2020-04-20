#include <iostream>
#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

#define DELETE(x)                                                              \
  if (x) {                                                                     \
    delete (x);                                                                \
    (x) = nullptr;                                                             \
  }

using namespace psr;

typedef std::tuple<const IDEGeneralizedLCA::v_t, unsigned, unsigned>
    groundTruth_t;

/* ============== TEST FIXTURE ============== */

class IDEGeneralizedLCATest : public ::testing::Test {

protected:
  // TODO: move IR files into another directory
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/general_linear_constant";

  ProjectIRDB *irdb = nullptr;
  LLVMTypeHierarchy *th = nullptr;
  LLVMBasedICFG *icfg = nullptr;
  IDEGeneralizedLCA *LCP = nullptr;
  IDESolver<const llvm::Instruction *, const llvm::Value *,
            const llvm::Function *, const llvm::StructType *,
            const llvm::Value *, EdgeValueSet, LLVMBasedICFG> *solver = nullptr;

  IDEGeneralizedLCATest() {}
  virtual ~IDEGeneralizedLCATest() {}

  void Initialize(const std::string &llFile, size_t maxSetSize = 2) {
    // ASSERT_EQ(true, std::filesystem::exists(pathToLLFiles + llFile))
    //    << "for file " << llFile;

    irdb = new ProjectIRDB({pathToLLFiles + llFile}, IRDBOptions::WPA);

    // irdb->preprocessIR();
    // irdb->print();

    th = new LLVMTypeHierarchy(*irdb);

    icfg = new LLVMBasedICFG(*irdb, CallGraphAnalysisType::RTA, {"main"}, th);

    // TODO: remove this
    // is PointsToInfo object necessary?
    // if not, update constructor
    const PointsToInfo<const llvm::Value *, const llvm::Instruction *> *pt;
    LCP = new IDEGeneralizedLCA(irdb, th, icfg, pt, {"main"}, maxSetSize);

    solver =
        new IDESolver<const llvm::Instruction *, const llvm::Value *,
                      const llvm::Function *, const llvm::StructType *,
                      const llvm::Value *, EdgeValueSet, LLVMBasedICFG>(*LCP);
    // try {
    solver->solve();
    /*} catch (std::exception &e) {

      std::cerr << "ERROR: " << e.what()
                << " at: " << boost::stacktrace::stacktrace() << std::endl;
    }
    std::cout << "Hello" << std::endl;*/
  }

  void SetUp() override {
    // TODO: fix this, what is bl?
    // bl::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
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

TEST_F(IDEGeneralizedLCATest, SimpleTest) {
  Initialize("SimpleTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(10)}, 3, 20});
  groundTruth.push_back({{EdgeValue(15)}, 4, 20});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, BranchTest) {
  Initialize("BranchTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(25), EdgeValue(43)}, 3, 22});
  groundTruth.push_back({{EdgeValue(24)}, 4, 22});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, FPtest) {
  Initialize("FPtest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(4.5)}, 1, 16});
  groundTruth.push_back({{EdgeValue(2.0)}, 2, 16});
  compareResults(groundTruth);
}
TEST_F(IDEGeneralizedLCATest, StringTest) {
  Initialize("StringTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue("Hello, World")}, 2, 8});
  groundTruth.push_back({{EdgeValue("Hello, World")}, 3, 8});
  compareResults(groundTruth);
}
TEST_F(IDEGeneralizedLCATest, StringBranchTest) {
  Initialize("StringBranchTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back(
      {{EdgeValue("Hello, World"), EdgeValue("Hello Hello")}, 3, 15});
  groundTruth.push_back({{EdgeValue("Hello Hello")}, 4, 15});
  compareResults(groundTruth);
}
TEST_F(IDEGeneralizedLCATest, FloatDivisionTest) {
  Initialize("FloatDivision_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(nullptr)}, 1, 24}); // i
  groundTruth.push_back({{EdgeValue(1.0)}, 2, 24});     // j
  groundTruth.push_back({{EdgeValue(-7.0)}, 3, 24});    // k
  compareResults(groundTruth);
}
TEST_F(IDEGeneralizedLCATest, SimpleFunctionTest) {
  Initialize("SimpleFunctionTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(48)}, 10, 31});      // i
  groundTruth.push_back({{EdgeValue(nullptr)}, 11, 31}); // j
  compareResults(groundTruth);
}
TEST_F(IDEGeneralizedLCATest, GlobalVariableTest) {
  Initialize("GlobalVariableTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(50)}, 7, 13});       // i
  groundTruth.push_back({{EdgeValue(nullptr)}, 10, 13}); // j
  compareResults(groundTruth);
}
TEST_F(IDEGeneralizedLCATest, Imprecision) {
  // bl::core::get()->set_logging_enabled(true);
  Initialize("Imprecision_c.ll", 2);
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
TEST_F(IDEGeneralizedLCATest, ReturnConstTest) {
  Initialize("ReturnConstTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(43)}, 7, 8}); // i
  compareResults(groundTruth);
}
TEST_F(IDEGeneralizedLCATest, NullTest) {
  Initialize("NullTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue("")}, 4, 5}); // foo(null)
  compareResults(groundTruth);
}
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
