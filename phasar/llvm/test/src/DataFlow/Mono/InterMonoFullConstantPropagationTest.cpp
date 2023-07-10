/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann, and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/InterMonoFullConstantPropagation.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DataFlow/Mono/Solver/InterMonoSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
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

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class InterMonoFullConstantPropagationTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("full_constant/");
  const std::vector<std::string> EntryPoints = {"main"};

  using IMFCPCompactResult_t =
      std::tuple<std::string, std::size_t, std::string,
                 LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>;
  std::optional<HelperAnalyses> HA;

  void
  doAnalysisAndCompareResults(llvm::StringRef LlvmFilePath,
                              const std::set<IMFCPCompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    HA.emplace(PathToLlFiles + LlvmFilePath, EntryPoints);

    if (PrintDump) {
      HA->getProjectIRDB().dump();
    }
    ValueAnnotationPass::resetValueID();
    auto FCP = createAnalysisProblem<InterMonoFullConstantPropagation>(
        *HA, EntryPoints);

    InterMonoSolver_P<InterMonoFullConstantPropagation, 3> IMSolver(FCP);
    IMSolver.solve();
    if (PrintDump) {
      IMSolver.dumpResults();
    }
    llvm::outs() << "Done analysis!\n";
    // do the comparison
    bool ResultNotEmpty = false;
    for (const auto &Truth : GroundTruth) {
      const auto *Fun =
          HA->getProjectIRDB().getFunctionDefinition(std::get<0>(Truth));
      const auto *Line = getNthInstruction(Fun, std::get<1>(Truth));
      auto ResultSet = IMSolver.getResultsAt(Line);
      for (const auto &[Fact, Value] : ResultSet) {
        std::string FactStr = llvmIRToString(Fact);
        llvm::StringRef FactRef(FactStr);
        if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
          llvm::outs() << "Checking variable: " << FactStr << '\n';
          ResultNotEmpty = true;
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
    EXPECT_TRUE(ResultNotEmpty);
  }

}; // Test Fixture

// // Test for Case I of Store
// TEST_F(InterMonoFullConstantPropagationTest, BasicTest_01) {
//   std::set<IMFCPCompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//                  LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
//           "main", 5, "i", 13));
//   doAnalysisAndCompareResults("basic_01.ll", GroundTruth, true);
// }

// // Test for Case II of Store and Load Inst
// TEST_F(InterMonoFullConstantPropagationTest, BasicTest_02) {
//   std::set<IMFCPCompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//                  LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
//           "main", 8, "i", 13));
//   doAnalysisAndCompareResults("basic_02.ll", GroundTruth, true);
// }

// // Test for Operators
// TEST_F(InterMonoFullConstantPropagationTest, BasicTest_03) {
//   std::set<IMFCPCompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//                  LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
//           "main", 9, "i", 13));
//   doAnalysisAndCompareResults("basic_03.ll", GroundTruth, true);
// }

// // Test for return Flow
// TEST_F(InterMonoFullConstantPropagationTest, AdvancedTest_01) {
//   std::set<IMFCPCompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//                  LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
//           "main", 6, "i", 13));
//   doAnalysisAndCompareResults("advanced_01.ll", GroundTruth, true);
// }

// // Test for Call Flow
// TEST_F(InterMonoFullConstantPropagationTest, AdvancedTest_02) {
//   std::set<IMFCPCompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//                  LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
//           "main", 6, "i", 13));
//   doAnalysisAndCompareResults("advanced_02.ll", GroundTruth, true);
// }

// // Test for Call Flow
// TEST_F(InterMonoFullConstantPropagationTest, AdvancedTest_03) {
//   std::set<IMFCPCompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//                  LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
//           "main", 9, "i", 5));
//   doAnalysisAndCompareResults("advanced_03.ll", GroundTruth, true);
// }
