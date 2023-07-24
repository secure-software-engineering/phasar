/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/IntraMonoUninitVariables.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DataFlow/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <set>
#include <string>
#include <utility>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IntraMonoUninitVariablesTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles =
      PHASAR_BUILD_SUBFOLDER("/uninitialized_variables/");

  using CompactResults_t = std::set<std::pair<size_t, std::set<std::string>>>;

  const std::vector<std::string> EntryPoints = {"main"};

  void doAnalysisAndCompareResults(llvm::StringRef LlvmFilePath,
                                   const CompactResults_t & /*GroundTruth*/,
                                   bool PrintDump = false) {
    HelperAnalyses HA(PathToLLFiles + LlvmFilePath, EntryPoints);

    if (PrintDump) {
      HA.getProjectIRDB().dump();
    }

    auto Uninit =
        createAnalysisProblem<IntraMonoUninitVariables>(HA, EntryPoints);

    IntraMonoSolver Solver(Uninit);
    Solver.solve();
    if (PrintDump) {
      Solver.dumpResults();
    }
    // for (auto result :
    //      TaintSolver.getResultsAt(IRDB->getInstruction(InstId)).getAsSet()) {
    //   FoundResults.insert(getMetaDataID(result));
    // }
    // EXPECT_EQ(FoundResults, GroundTruth);
  }

}; // Test Fixture

// TEST_F(IntraMonoUninitVariablesTest, Basic_01) {
//   CompactResults_t GroundTruth;
//   doAnalysisAndCompareResults("basic_01.ll", GroundTruth, true);
// }

TEST_F(IntraMonoUninitVariablesTest, Basic_02) {
  CompactResults_t GroundTruth;
  GroundTruth.insert({13, {"%b"}});
  GroundTruth.insert({15, {"%a"}});
  GroundTruth.insert({17, {"%a", "%b"}});
  doAnalysisAndCompareResults("basic_02.ll", GroundTruth, true);
}
