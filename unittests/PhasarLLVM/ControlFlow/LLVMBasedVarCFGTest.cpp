/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include <sstream>
#include <tuple>

#include "gtest/gtest.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarCFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

#define DELETE(x)                                                              \
  if (x) {                                                                     \
    delete (x);                                                                \
    (x) = nullptr;                                                             \
  }

using namespace psr;

/* ============== TEST FIXTURE ============== */
class VariabilityCFGTest : public ::testing::Test {
protected:
  const std::string PathToLLFiles =
      unittest::PathToLLTestFiles + "/variability/linear_constant/basic/";

  ProjectIRDB *IRDB = nullptr;
  LLVMBasedVarCFG *VCFG = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  void initialize(const std::string &llvmFilePath) {
    IRDB = new ProjectIRDB({PathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    ValueAnnotationPass::resetValueID();
    VCFG = new LLVMBasedVarCFG(*IRDB);
  }

  z3::expr doAnalysis(const llvm::Instruction *currInst,
                      const llvm::Instruction *succInst) {
    return VCFG->getPPConstraintOrTrue(currInst, succInst);
  }

  void TearDown() override {
    DELETE(VCFG);
    DELETE(IRDB);
  }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(const z3::expr &groundTruth, const z3::expr &result) {
    std::stringstream s1, s2;
    s1 << groundTruth;
    s2 << result;
    // find better way of comparing
    EXPECT_EQ(s1.str(), s2.str());
  }
}; // Test Fixture

TEST_F(VariabilityCFGTest, Basic02) {
  initialize("basic_02_c_dbg_xtc.ll");
  const auto *Main = IRDB->getFunctionDefinition("__main_0");
  const auto *currInst = getNthInstruction(Main, 5);
  const auto *succInst = getNthInstruction(Main, 6);
  llvm::outs() << "\n\nTEST-DATA:\n";
  currInst->print(llvm::outs());
  llvm::outs() << '\n';
  succInst->print(llvm::outs());
  llvm::outs() << "\n\n";
  // ASSERT_NE(currInst, nullptr);
  // ASSERT_NE(succInst, nullptr);
  // EXPECT_TRUE(VCFG->isBranchTarget(currInst, succInst));
  // EXPECT_TRUE(VCFG->isPPBranchTarget(currInst, succInst));
  // auto &ctx = VICFG->getContext();
  // auto res = VICFG->getPPConstraintOrTrue(currInst, succInst);
  // std::stringstream resstr;
  // resstr << res;
  // std::cout << "actual constraint: " << resstr.str() << std::endl;
  // compareResults(VICFG->getPPConstraintOrTrue(currInst, succInst),
  //  ctx.bool_const("(declare-fun |(defined A)| () Bool)(assert |(defined
  //  A)|)"));
}
// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
