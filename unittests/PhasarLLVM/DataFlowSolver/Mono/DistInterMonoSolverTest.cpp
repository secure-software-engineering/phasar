/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Ajay Subramanya Kudli Prasanna Kumar, Philipp Schubert and others
 *****************************************************************************/

#include <gtest/gtest.h>

#include <iostream>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/DistInterMonoSolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class DistInterMonoSolverTest : public ::testing::Test {
protected:
  void doAnalysisAndCompare() {
    std::cout << "Simple unit test playground.\n";
    EXPECT_TRUE(1 == 1);
  }

}; // Test Fixture

TEST_F(DistInterMonoSolverTest, BasicTest_01) {
  doAnalysisAndCompare();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
