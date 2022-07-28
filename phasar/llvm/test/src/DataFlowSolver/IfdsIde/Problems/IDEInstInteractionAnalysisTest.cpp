/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <variant>

#include "gtest/gtest.h"

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"

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
  const std::string PathToLlFiles = "llvm_test_code/inst_interaction/";
  const std::set<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Values
  using IIACompactResult_t =
      std::tuple<std::string, std::size_t, std::string,
                 IDEInstInteractionAnalysisT<std::string, true>::l_t>;

  std::unique_ptr<ProjectIRDB> IRDB;

  void SetUp() override {}

  //   IDEInstInteractionAnalysis::lca_restults_t
  void
  doAnalysisAndCompareResults(const std::string &LlvmFilePath,
                              const std::set<IIACompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    auto IRFiles = {PathToLlFiles + LlvmFilePath};
    IRDB = std::make_unique<ProjectIRDB>(IRFiles, IRDBOptions::WPA);
    if (PrintDump) {
      IRDB->emitPreprocessedIR(llvm::outs(), false);
    }
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToSet PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::CHA, EntryPoints, &TH,
                       &PT);
    IDEInstInteractionAnalysisT<std::string, true> IIAProblem(
        IRDB.get(), &TH, &ICFG, &PT, EntryPoints);
    // use Phasar's instruction ids as testing labels
    auto Generator =
        [](std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>
               Current) -> std::set<std::string> {
      std::set<std::string> Labels;
      // case we are looking at an instruction
      if (std::holds_alternative<const llvm::Instruction *>(Current)) {
        const llvm::Instruction *CurrentInst =
            std::get<const llvm::Instruction *>(Current);
        if (CurrentInst->hasMetadata()) {
          std::string Label =
              llvm::cast<llvm::MDString>(
                  CurrentInst->getMetadata(PhasarConfig::MetaDataKind())
                      ->getOperand(0))
                  ->getString()
                  .str();
          Labels.insert(Label);
          return Labels;
        }
      }
      // case we are looking at a global variable
      if (std::holds_alternative<const llvm::GlobalVariable *>(Current)) {
        const llvm::GlobalVariable *CurrentGlobalVar =
            std::get<const llvm::GlobalVariable *>(Current);
        if (CurrentGlobalVar->hasMetadata()) {
          std::string Label =
              llvm::cast<llvm::MDString>(
                  CurrentGlobalVar->getMetadata(PhasarConfig::MetaDataKind())
                      ->getOperand(0))
                  ->getString()
                  .str();
          Labels.insert(Label);
          return Labels;
        }
      }
      // default
      return Labels;
    };
    // register the above generator function
    IIAProblem.registerEdgeFactGenerator(Generator);
    IDESolver_P<IDEInstInteractionAnalysisT<std::string, true>> IIASolver(
        IIAProblem);
    llvm::outs() << "Start solving the problem.\n";
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
          PHASAR_LOG_LEVEL(DFADEBUG, "Checking variable: " << FactStr);
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
  }

  void TearDown() override {}

}; // Test Fixture

// /* ============== BASIC TESTS for int ====== */

// // have a minimal individual test that instantiates the
// // IDEInstInteractionAnalysisT for a non-string template parameter
// // IDEInstInteractionAnalysisT<int>
// TEST(IDEInstInteractionAnalysisTTest, HandleInterger) {
//   bool printDump = false;
//   ProjectIRDB IRDB(
//       {"llvm_test_code/inst_interaction/basic_01.ll"},
//       IRDBOptions::WPA);
//   if (printDump) {
//     IRDB.emitPreprocessedIR(llvm::outs(), false);
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
//         llvm::outs() << "Checking variable: " << FactStr << std::endl;
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
          "main", 9, "i", {"4", "5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "j", {"4", "5", "6", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "retval", {"3"}));
  doAnalysisAndCompareResults("basic_01.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "retval", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argc.addr", {"7", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argv.addr", {"8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i", {"16", "18", "20"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"9", "10", "11", "12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"22", "21", "16", "18", "20"}));
  doAnalysisAndCompareResults("basic_02.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "i", {"4", "10", "11", "12", "18"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "x", {"5", "7", "14", "15", "16"}));
  doAnalysisAndCompareResults("basic_03.ll", GroundTruth, false);
}

PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_04) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "retval", {"13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "argc.addr", {"14", "21"}));
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
  doAnalysisAndCompareResults("basic_04.ll", GroundTruth, false);
})

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 11, "i", {"5", "7", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 11, "retval", {"2"}));
  doAnalysisAndCompareResults("basic_05.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "retval", {"5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "i", {"1", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "j", {"2", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "k", {"6", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "p", {"1", "2", "9", "11", "14", "16"}));
  doAnalysisAndCompareResults("basic_06.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "retval", {"5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "argc.addr", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "argv.addr", {"7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "i", {"12", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "j", {"8", "9", "10", "11"}));
  doAnalysisAndCompareResults("basic_07.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_08) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"2"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "i", {"9", "10"}));
  doAnalysisAndCompareResults("basic_08.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_09) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"4", "6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j", {"4", "6", "7", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"3"}));
  doAnalysisAndCompareResults("basic_09.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_10) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "i", {"3", "4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "retval", {"2"}));
  doAnalysisAndCompareResults("basic_10.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_11) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "FeatureSelector", {"5", "7", "8", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"11", "16", "18"}));
  doAnalysisAndCompareResults("basic_11.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "retval", {"8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "i", {"9", "10"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "j", {"13", "12", "9", "10", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "k",
          {"15", "1", "2", "13", "16", "12", "9", "10", "11"}));
  doAnalysisAndCompareResults("call_01.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "retval", {"12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "i", {"13", "15"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "j", {"14", "16"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "k",
          {"4", "19", "5", "15", "6", "3", "14", "2", "13", "16", "18"}));
  doAnalysisAndCompareResults("call_02.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"20"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"21", "22"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j",
          {"22", "15", "6", "21", "3", "2", "13", "8", "9", "12", "10", "24",
           "25"}));
  doAnalysisAndCompareResults("call_03.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"33"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "i", {"41", "35", "34"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "j",
          {"15", "6", "3", "2", "13", "8", "9", "12", "10", "35", "38", "34",
           "37", "42"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "k",
          {"41", "19", "15", "6",  "44", "3",  "2",  "13", "8",
           "45", "18", "9",  "12", "10", "46", "24", "47", "25",
           "35", "27", "23", "26", "38", "34", "37", "42", "40"}));
  doAnalysisAndCompareResults("call_04.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"1", "11", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j", {"1", "10", "12", "13"}));
  doAnalysisAndCompareResults("call_05.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "retval", {"11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i", {"4", "3", "1", "2", "28", "16", "18", "12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"4", "19", "21", "3", "1", "2", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"4", "22", "3", "14", "1", "2", "24"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "l", {"4", "15", "3", "1", "2", "25", "27"}));
  doAnalysisAndCompareResults("call_06.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "i", {"0", "7", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "j", {"0", "5", "6"}));
  doAnalysisAndCompareResults("global_01.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z5initBv", 2, "a", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z5initBv", 2, "b", {"1", "2"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "a", {"0", "10"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "b", {"1", "2", "7", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "c", {"1", "8", "7", "13"}));
  doAnalysisAndCompareResults("global_02.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 1, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 2, "GlobalFeature", {"0", "1"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 17, "GlobalFeature", {"0", "1"}));
  doAnalysisAndCompareResults("global_03.o1.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 1, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 2, "GlobalFeature", {"0", "3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 18, "GlobalFeature", {"0", "3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z7doStuffi", 1, "GlobalFeature", {"0", "3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z7doStuffi", 2, "GlobalFeature", {"0", "3"}));
  doAnalysisAndCompareResults("global_04.o1.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "i", {"5", "6", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "j", {"10"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "k", {"9", "8", "5", "6"}));
  doAnalysisAndCompareResults("KillTest_01.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "A", {"0", "10"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "B", {"2", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "C", {"1", "7", "8", "13"}));
  doAnalysisAndCompareResults("KillTest_02.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleReturnTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "localVar", {"4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "call", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 8, "localVar", {"0", "6", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 8, "call", {"0", "6"}));
  doAnalysisAndCompareResults("return_01.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleHeapTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "i", {"6", "7", "8", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "j", {"6", "7", "8", "17", "10", "9"}));
  doAnalysisAndCompareResults("heap_01.ll", GroundTruth, false);
}

PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleRVOTest_01) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 16, "retval", {"23", "35", "37"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 16, "str", {"70", "65", "72", "74", "77"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 16, "ref.tmp", {"66", "9", "6", "29", "72", "73", "71"}));
  doAnalysisAndCompareResults("rvo_01.ll", GroundTruth, false);
})

TEST_F(IDEInstInteractionAnalysisTest, HandleStruct_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "a", {"1", "4", "5", "6", "7", "8", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "x", {"1", "4", "5", "13"}));
  doAnalysisAndCompareResults("struct_01.ll", GroundTruth, false);
}
