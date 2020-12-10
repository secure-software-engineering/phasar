/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>
#include <set>
#include <string>
#include <tuple>

#include "gtest/gtest.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEInstInteractionAnalysisTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      unittest::PathToLLTestFiles + "inst_interaction/";
  const std::set<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Values
  using IIACompactResult_t =
      std::tuple<std::string, std::size_t, std::string,
                 IDEInstInteractionAnalysisT<std::string, true>::l_t>;
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
    setLoggerFilterLevel(DFADEBUG);
  }

  //   IDEInstInteractionAnalysis::lca_restults_t
  void
  doAnalysisAndCompareResults(const std::string &LlvmFilePath,
                              const std::set<IIACompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    IRDB = new ProjectIRDB({PathToLlFiles + LlvmFilePath}, IRDBOptions::WPA);
    if (PrintDump) {
      IRDB->emitPreprocessedIR(std::cout, false);
    }
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToSet PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::CHA, EntryPoints, &TH,
                       &PT);
    IDEInstInteractionAnalysisT<std::string, true> IIAProblem(IRDB, &TH, &ICFG,
                                                              &PT, EntryPoints);
    // use Phasar's instruction ids as testing labels
    auto Generator = [](const llvm::Instruction *I) -> std::set<std::string> {
      std::set<std::string> Labels;
      if (I->hasMetadata()) {
        std::string Label =
            llvm::cast<llvm::MDString>(
                I->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
                ->getString()
                .str();
        Labels.insert(Label);
      }
      return Labels;
    };
    // register the above generator function
    IIAProblem.registerEdgeFactGenerator(Generator);
    IDESolver_P<IDEInstInteractionAnalysisT<std::string, true>> IIASolver(
        IIAProblem);
    std::cout << "Start solving the problem.\n";
    IIASolver.solve();
    if (PrintDump) {
      IIASolver.dumpResults();
    }
    // IIASolver.emitESGasDot();
    // do the comparison
    for (const auto &Truth : GroundTruth) {
      const auto *Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      const auto *Line = getNthInstruction(Fun, std::get<1>(Truth));
      auto ResultMap = IIASolver.resultsAt(Line);
      for (auto &[Fact, Value] : ResultMap) {
        std::string FactStr = llvmIRToString(Fact);
        llvm::StringRef FactRef(FactStr);
        if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                        << "Checking variable: " << FactStr);
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
  }

  void TearDown() override { delete IRDB; }

}; // Test Fixture

// /* ============== BASIC TESTS for int ====== */

// // have a minimal individual test that instantiates the
// // IDEInstInteractionAnalysisT for a non-string template parameter
// // IDEInstInteractionAnalysisT<int>
// TEST(IDEInstInteractionAnalysisTTest, HandleInterger) {
//   bool printDump = false;
//   boost::log::core::get()->set_logging_enabled(false);
//   ProjectIRDB IRDB(
//       {PhasarConfig::getPhasarConfig().PhasarDirectory() +
//        "build/test/llvm_test_code/inst_interaction/basic_01_cpp.ll"},
//       IRDBOptions::WPA);
//   if (printDump) {
//     IRDB.emitPreprocessedIR(std::cout, false);
//   }
//   ValueAnnotationPass::resetValueID();
//   LLVMTypeHierarchy TH(IRDB);
//   LLVMPointsToInfo PT(IRDB);
//   std::set<std::string> EntryPoints({"main"});
//   LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, EntryPoints, &TH,
//   &PT); IDEInstInteractionAnalysisT<int> IIAProblem(&IRDB, &TH, &ICFG, &PT,
//                                               EntryPoints);
//   // use Phasar's instruction ids as testing labels
//   auto Generator = [](const llvm::Instruction *I, const llvm::Value *SrcNode,
//                       const llvm::Value *DestNode) -> std::set<int> {
//     std::set<int> Labels;
//     if (I->hasMetadata()) {
//       std::string Label =
//           llvm::cast<llvm::MDString>(
//               I->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
//               ->getString()
//               .str();
//       Labels.insert(std::stoi(Label));
//     }
//     return Labels;
//   };
//   // register the above generator function
//   IIAProblem.registerEdgeFactGenerator(Generator);
//   IDESolver_P<IDEInstInteractionAnalysisT<int>> IIASolver(IIAProblem);
//   IIASolver.solve();
//   if (printDump) {
//     IIASolver.dumpResults();
//   }
//   // IIASolver.emitESGasDot();
//   // specify the ground truth
//   using IIACompactResultInteger_t =
//       std::tuple<std::string, std::size_t, std::string,
//                  IDEInstInteractionAnalysisT<int>::l_t>;
//   std::set<IIACompactResultInteger_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string, BitVectorSet<int>>(
//           "main", 9, "i", {1, 4, 5}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string, BitVectorSet<int>>(
//           "main", 9, "j", {1, 2, 4, 7}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string, BitVectorSet<int>>(
//           "main", 9, "retval", {0, 3}));
//   // do the comparison
//   for (auto &Truth : GroundTruth) {
//     auto Fun = IRDB.getFunctionDefinition(std::get<0>(Truth));
//     auto Line = getNthInstruction(Fun, std::get<1>(Truth));
//     auto ResultMap = IIASolver.resultsAt(Line);
//     for (auto &[Fact, Value] : ResultMap) {
//       std::string FactStr = llvmIRToString(Fact);
//       llvm::StringRef FactRef(FactStr);
//       if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
//         std::cout << "Checking variable: " << FactStr << std::endl;
//         EXPECT_EQ(std::get<3>(Truth), Value);
//       }
//     }
//   }
// }

/* ============== BASIC TESTS ============== */
TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "i", {"1", "4", "5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "j", {"1", "2", "4", "5", "6", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "retval", {"0", "3"}));
  doAnalysisAndCompareResults("basic_01_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "retval", {"0", "6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argc.addr", {"1", "7", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argv.addr", {"2", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i", {"3", "16", "18", "20"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"4", "3", "10", "12", "9", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"22", "5", "21", "3", "16", "18", "20"}));
  doAnalysisAndCompareResults("basic_02_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"0", "3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "i", {"1", "4", "10", "11", "12", "18"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "x", {"2", "5", "7", "14", "15", "16"}));
  doAnalysisAndCompareResults("basic_03_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "retval", {"7", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "argc.addr", {"8", "14", "21"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argv.addr", {"9", "15"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i", {"10", "16", "17"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"10", "11", "16", "19", "24"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"10", "11", "12", "16", "19", "20", "25", "27"}));
  doAnalysisAndCompareResults("basic_04_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 11, "i", {"1", "5", "7", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 11, "retval", {"0", "2"}));
  doAnalysisAndCompareResults("basic_05_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "retval", {"0", "5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "i", {"1", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "j", {"2", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "k", {"3", "6", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "p", {"1", "2", "4", "9", "11", "14", "16"}));
  doAnalysisAndCompareResults("basic_06_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "retval", {"0", "5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "argc.addr", {"1", "6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "argv.addr", {"2", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "i", {"3", "12", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "j", {"4", "3", "8", "10", "9", "11"}));
  doAnalysisAndCompareResults("basic_07_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_08) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"0", "2"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "i", {"1", "9", "10"}));
  doAnalysisAndCompareResults("basic_08_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_09) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"1", "4", "6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j", {"1", "2", "4", "6", "7", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"0", "3"}));
  doAnalysisAndCompareResults("basic_09_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_10) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "i", {"1", "3", "4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "retval", {"0", "2"}));
  doAnalysisAndCompareResults("basic_10_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "retval", {"4", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "i", {"5", "9", "10"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "j", {"5", "6", "13", "10", "9", "12", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "k",
          {"1", "5", "2", "15", "6", "7", "0", "13", "16", "10", "9", "12",
           "11"}));
  doAnalysisAndCompareResults("call_01_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "retval", {"8", "12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "i", {"9", "13", "15"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "j", {"10", "14", "16"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "k",
          {"1", "4", "19", "5", "2", "15", "6", "14", "0", "3", "13", "16",
           "18", "10", "9", "11"}));
  doAnalysisAndCompareResults("call_02_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"17", "20"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"18", "21", "22"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j",
          {"1", "19", "22", "2", "15", "6", "0", "21", "3", "13", "8", "18",
           "10", "9", "12", "24", "25"}));
  doAnalysisAndCompareResults("call_03_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"29", "33"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "i", {"41", "35", "30", "34"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "j",
          {"1", "31", "2", "15", "6", "0", "3", "13", "8", "10", "9", "12",
           "35", "30", "38", "34", "37", "42"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "k",
          {"1",  "41", "31", "19", "22", "2",  "15", "6",  "0",
           "44", "21", "3",  "13", "8",  "45", "18", "17", "10",
           "9",  "12", "46", "24", "47", "25", "35", "27", "23",
           "26", "30", "32", "38", "34", "37", "40", "42"}));
  doAnalysisAndCompareResults("call_04_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"5", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"1", "11", "6", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j", {"1", "7", "10", "12", "13"}));
  doAnalysisAndCompareResults("call_05_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "retval", {"6", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i",
          {"1", "4", "2", "7", "0", "3", "16", "18", "12", "28"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"1", "4", "19", "2", "0", "21", "3", "13", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"1", "4", "22", "2", "14", "0", "3", "9", "24"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "l", {"1", "4", "2", "15", "0", "3", "10", "25", "27"}));
  doAnalysisAndCompareResults("call_06_cpp.ll", GroundTruth, false);
}

// TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_01) {
//   std::set<IIACompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 9, "retval", {"1", "3"}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 9, "i", {"0", "7", "8"}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 9, "j", {"2", "6", "5", "0"}));
//   doAnalysisAndCompareResults("global_01_cpp.ll", GroundTruth, true);
// }

TEST_F(IDEInstInteractionAnalysisTest, KillTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"0", "4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "i", {"1", "5", "6", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "j", {"2", "10"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "k", {"3", "9", "8", "1", "5", "6"}));
  doAnalysisAndCompareResults("KillTest_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleHeap_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "retval", {"0", "3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "i", {"1", "6", "7", "8", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "j", {"1", "2", "6", "7", "8", "10", "9", "17"}));
  doAnalysisAndCompareResults("heap_01_cpp.ll", GroundTruth, false);
}

// TEST_F(IDEInstInteractionAnalysisTest, HandleStruct_01) {
//   std::set<IIACompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 3, "retval", {"0"}));
//   doAnalysisAndCompareResults("struct_01_cpp.ll", GroundTruth, false);
// }

// TEST_F(IDEInstInteractionAnalysisTest, HandleRealWorldProgram_GZipTest) {
//   doAnalysisAndCompareResults("gzip-gzip-81c9fe4d09.ll", {}, false);
// }

// main function for the test case/*  */
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
