/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarCFG.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/StringRef.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <optional>

namespace {
using namespace psr;

/* ============== TEST FIXTURE ============== */
class VariabilityCFGTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles =
      PHASAR_BUILD_SUBFOLDER("/variability/linear_constant/basic/");

  std::unique_ptr<LLVMProjectIRDB> IRDB{};
  std::optional<VarCFG<LLVMBasedICFG, z3::expr>> VCFG{};

  void initialize(llvm::StringRef IRFilePath) {
    IRDB = std::make_unique<LLVMProjectIRDB>(PathToLLFiles + IRFilePath);
    ASSERT_TRUE(IRDB->isValid());
    VCFG.emplace(*IRDB, LLVMBasedCFG());
  }

  /**
   * @param Result actual result
   * @param GroundTruth expected results
   */
  void compareResults(const z3::expr &Result,
                      llvm::StringRef GroundTruthBoolConst) {

    // find better way of comparing
    EXPECT_EQ(Result.to_string(), GroundTruthBoolConst);
  }
}; // Test Fixture

TEST_F(VariabilityCFGTest, Basic02) {
  initialize("basic_02_xtc_c_dbg.ll");
  const auto *Main = IRDB->getFunctionDefinition("__main_0");
  ASSERT_TRUE(Main);
  const auto *CurrInst = getNthInstruction(Main, 5);
  const auto *SuccInst = getNthInstruction(Main, 6);
  ASSERT_NE(CurrInst, nullptr);
  ASSERT_NE(SuccInst, nullptr);
  // EXPECT_TRUE(VCFG->isBranchTarget(currInst, succInst));
  EXPECT_TRUE(VCFG->isPPBranchTarget(CurrInst, SuccInst));
  compareResults(VCFG->getPPConstraintOrTrue(CurrInst, SuccInst),
                 "|(defined A)|");
}

} // namespace

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
