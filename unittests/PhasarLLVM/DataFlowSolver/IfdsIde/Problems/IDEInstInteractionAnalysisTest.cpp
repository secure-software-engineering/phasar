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

#include <gtest/gtest.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/BitVectorSet.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEInstInteractionAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/inst_interaction/";
  const std::set<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Values
  using IIACompactResult_t = std::tuple<std::string, std::size_t, std::string,
                                        IDEInstInteractionAnalysis::l_t>;
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  //   IDEInstInteractionAnalysis::lca_restults_t
  void doAnalysisAndCompareResults(const std::string &llvmFilePath,
                                   std::set<IIACompactResult_t> GroundTruth,
                                   bool printDump = false) {
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    if (printDump) {
      IRDB->emitPreprocessedIR(std::cout, false);
    }
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToInfo PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::CHA, EntryPoints, &TH,
                       &PT);
    IDEInstInteractionAnalysis IIAProblem(IRDB, &TH, &ICFG, &PT, EntryPoints);
    // use Phasar's instruction ids as testing labels
    auto Generator = [](const llvm::Instruction *I, const llvm::Value *SrcNode,
                        const llvm::Value *DestNode) -> std::set<std::string> {
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
    IDESolver_P<IDEInstInteractionAnalysis> IIASolver(IIAProblem);
    IIASolver.solve();
    if (printDump) {
      IIASolver.dumpResults();
    }
    // IIASolver.emitESGasDot();
    // do the comparison
    for (auto &Truth : GroundTruth) {
      auto Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      auto Line = getNthInstruction(Fun, std::get<1>(Truth));
      auto ResultMap = IIASolver.resultsAt(Line);
      for (auto &[Fact, Value] : ResultMap) {
        std::string FactStr = llvmIRToString(Fact);
        llvm::StringRef FactRef(FactStr);
        if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
          std::cout << "Checking variable: " << FactStr << std::endl;
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
  }

  void TearDown() override { delete IRDB; }

}; // Test Fixture

/* ============== BASIC TESTS for int ====== */

// have a minimal individual test that instantiates the
// IDEInstInteractionAnalysisT for a non-string template parameter
// IDEInstInteractionAnalysisT<int>
TEST(IDEInstInteractionAnalysisTTest, HandleInterger) {
  bool printDump = false;
  boost::log::core::get()->set_logging_enabled(false);
  ProjectIRDB IRDB(
      {PhasarConfig::getPhasarConfig().PhasarDirectory() +
       "build/test/llvm_test_code/inst_interaction/basic_01_cpp.ll"},
      IRDBOptions::WPA);
  if (printDump) {
    IRDB.emitPreprocessedIR(std::cout, false);
  }
  ValueAnnotationPass::resetValueID();
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToInfo PT(IRDB);
  std::set<std::string> EntryPoints({"main"});
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, EntryPoints, &TH, &PT);
  IDEInstInteractionAnalysisT<int> IIAProblem(&IRDB, &TH, &ICFG, &PT,
                                              EntryPoints);
  // use Phasar's instruction ids as testing labels
  auto Generator = [](const llvm::Instruction *I, const llvm::Value *SrcNode,
                      const llvm::Value *DestNode) -> std::set<int> {
    std::set<int> Labels;
    if (I->hasMetadata()) {
      std::string Label =
          llvm::cast<llvm::MDString>(
              I->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
              ->getString()
              .str();
      Labels.insert(std::stoi(Label));
    }
    return Labels;
  };
  // register the above generator function
  IIAProblem.registerEdgeFactGenerator(Generator);
  IDESolver_P<IDEInstInteractionAnalysisT<int>> IIASolver(IIAProblem);
  IIASolver.solve();
  if (printDump) {
    IIASolver.dumpResults();
  }
  // IIASolver.emitESGasDot();
  // specify the ground truth
  using IIACompactResultInteger_t =
      std::tuple<std::string, std::size_t, std::string,
                 IDEInstInteractionAnalysisT<int>::l_t>;
  std::set<IIACompactResultInteger_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<int>>(
          "main", 9, "i", {1, 4, 5}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<int>>(
          "main", 9, "j", {1, 2, 4, 7}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<int>>(
          "main", 9, "retval", {0, 3}));
  // do the comparison
  for (auto &Truth : GroundTruth) {
    auto Fun = IRDB.getFunctionDefinition(std::get<0>(Truth));
    auto Line = getNthInstruction(Fun, std::get<1>(Truth));
    auto ResultMap = IIASolver.resultsAt(Line);
    for (auto &[Fact, Value] : ResultMap) {
      std::string FactStr = llvmIRToString(Fact);
      llvm::StringRef FactRef(FactStr);
      if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
        std::cout << "Checking variable: " << FactStr << std::endl;
        EXPECT_EQ(std::get<3>(Truth), Value);
      }
    }
  }
}

/* ============== BASIC TESTS ============== */
TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "i", {"1", "4", "5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "j", {"1", "2", "4", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "retval", {"0", "3"}));
  doAnalysisAndCompareResults("basic_01_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i", {"10", "16", "18", "20", "3", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"12", "3", "4", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"10", "16", "18", "21", "22", "3", "5", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "retval", {"0", "6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argc.addr", {"1", "7", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argv.addr", {"2", "8"}));
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
          "main", 19, "1", {"1", "11", "15", "2", "3", "4", "6", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "i", {"1", "3", "6", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "j", {"2", "3", "6", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "p", {"1", "2", "4", "9", "11", "14", "16"}));
  doAnalysisAndCompareResults("basic_06_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  // GroundTruth.emplace(
  // std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
  // "main", 11, "i", {"1", "5", "7", "9"}));
  doAnalysisAndCompareResults("call_01_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  doAnalysisAndCompareResults("call_02_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  doAnalysisAndCompareResults("call_03_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  doAnalysisAndCompareResults("call_04_cpp.ll", GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "i", {"1", "11", "6", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "j", {"1", "7", "10", "12", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "retval", {"5", "8"}));
  doAnalysisAndCompareResults("call_05_cpp.ll", GroundTruth, false);
}

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
