/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <memory>

#include "gtest/gtest.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisFileIOTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      unittest::PathToLLTestFiles + "typestate_analysis_fileio/";
  const std::set<std::string> EntryPoints = {"main"};

  unique_ptr<ProjectIRDB> IRDB;
  unique_ptr<LLVMTypeHierarchy> TH;
  unique_ptr<LLVMBasedICFG> ICFG;
  unique_ptr<LLVMPointsToInfo> PT;
  unique_ptr<CSTDFILEIOTypeStateDescription> CSTDFILEIODesc;
  unique_ptr<IDETypeStateAnalysis> TSProblem;
  enum IOSTATE {
    TOP = 42,
    UNINIT = 0,
    OPENED = 1,
    CLOSED = 2,
    ERROR = 3,
    BOT = 4
  };

  IDETSAnalysisFileIOTest() = default;
  ~IDETSAnalysisFileIOTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = make_unique<ProjectIRDB>(IRFiles, IRDBOptions::WPA);
    TH = make_unique<LLVMTypeHierarchy>(*IRDB);
    PT = make_unique<LLVMPointsToSet>(*IRDB);
    ICFG = make_unique<LLVMBasedICFG>(*IRDB, CallGraphAnalysisType::OTF,
                                      EntryPoints, TH.get(), PT.get());
    CSTDFILEIODesc = make_unique<CSTDFILEIOTypeStateDescription>();
    TSProblem = make_unique<IDETypeStateAnalysis>(IRDB.get(), TH.get(),
                                                  ICFG.get(), PT.get(),
                                                  *CSTDFILEIODesc, EntryPoints);
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(
      const std::map<std::size_t, std::map<std::string, int>> &GroundTruth,
      IDESolver_P<IDETypeStateAnalysis> &Solver) {
    for (const auto &InstToGroundTruth : GroundTruth) {
      auto *Inst = IRDB->getInstruction(InstToGroundTruth.first);
      // std::cout << "Handle results at " << InstToGroundTruth.first <<
      // std::endl;
      auto GT = InstToGroundTruth.second;
      std::map<std::string, int> Results;
      for (auto Result : Solver.resultsAt(Inst, true)) {
        if (GT.find(getMetaDataID(Result.first)) != GT.end()) {
          Results.insert(std::pair<std::string, int>(
              getMetaDataID(Result.first), Result.second));
        }
      }
      EXPECT_EQ(Results, GT) << "At " << llvmIRToShortString(Inst);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_01) {
  initialize({PathToLlFiles + "typestate_01_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);
  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      {5, {{"3", IOSTATE::UNINIT}}},
      {9, {{"3", IOSTATE::CLOSED}}},
      {7, {{"3", IOSTATE::OPENED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_02) {
  initialize({PathToLlFiles + "typestate_02_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      {7, {{"3", IOSTATE::OPENED}, {"5", IOSTATE::OPENED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_03) {
  initialize({PathToLlFiles + "typestate_03_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  // llvmtssolver.printReport();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Entry in foo()
      {2, {{"foo.0", IOSTATE::OPENED}}},
      // Exit in foo()
      {6,
       {
           {"foo.0", IOSTATE::CLOSED},
           {"2", IOSTATE::CLOSED},
           {"4", IOSTATE::CLOSED},
           //{"8", IOSTATE::CLOSED} // 6 is before 8; so no info avaliable
           // before ret FF
       }},
      // Exit in main()
      {14,
       {{"2", IOSTATE::CLOSED},
        {"8", IOSTATE::CLOSED},
        {"12", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_04) {
  initialize({PathToLlFiles + "typestate_04_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();

  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in foo()
      {6,
       {
           {"2", IOSTATE::OPENED},
           //{"8", IOSTATE::OPENED} // 6 is before 8, so no info available
           // before retFF
       }},
      // Before closing in main()
      {12, {{"2", IOSTATE::UNINIT}, {"8", IOSTATE::UNINIT}}},
      // At exit in main()
      {14, {{"2", IOSTATE::ERROR}, {"8", IOSTATE::ERROR}}}};

  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_05) {
  initialize({PathToLlFiles + "typestate_05_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Before if statement
      {10, {{"4", IOSTATE::OPENED}, {"6", IOSTATE::OPENED}}},
      // Inside if statement at last instruction
      {13,
       {{"4", IOSTATE::CLOSED},
        {"6", IOSTATE::CLOSED},
        {"11", IOSTATE::CLOSED}}},
      // After if statement
      {14, {{"4", IOSTATE::BOT}, {"6", IOSTATE::BOT}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, DISABLED_HandleTypeState_06) {
  // This test fails due to imprecise points-to information
  initialize({PathToLlFiles + "typestate_06_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Before first fopen()
      {8, {{"5", IOSTATE::UNINIT}, {"6", IOSTATE::UNINIT}}},
      // Before storing the result of the first fopen()
      {9,
       {{"5", IOSTATE::UNINIT},
        {"6", IOSTATE::UNINIT},
        // Return value of first fopen()
        {"8", IOSTATE::OPENED}}},
      // Before second fopen()
      {10,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::UNINIT},
        {"8", IOSTATE::OPENED}}},
      // Before storing the result of the second fopen()
      {11,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::UNINIT},
        // Return value of second fopen()
        {"10", IOSTATE::OPENED}}},
      // Before fclose()
      {13,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::OPENED},
        {"12", IOSTATE::OPENED}}},
      // After if statement
      {14, {{"5", IOSTATE::CLOSED}, {"6", IOSTATE::OPENED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_07) {
  initialize({PathToLlFiles + "typestate_07_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // In foo()
      {6,
       {
           {"foo.0", IOSTATE::CLOSED}, {"2", IOSTATE::CLOSED},
           //{"8", IOSTATE::CLOSED}// 6 is before 8, so no info available
           // before retFF
       }},
      // At fclose()
      {11, {{"8", IOSTATE::UNINIT}, {"10", IOSTATE::UNINIT}}},
      // After fclose()
      {12, {{"8", IOSTATE::ERROR}, {"10", IOSTATE::ERROR}}},
      // After fopen()
      {13,
       {{"8", IOSTATE::ERROR},
        {"10", IOSTATE::ERROR},
        {"12", IOSTATE::OPENED}}},
      // After store
      {14,
       {{"8", IOSTATE::OPENED},
        {"10", IOSTATE::ERROR},
        {"12", IOSTATE::OPENED}}},
      // At exit in main()
      {16, {{"2", IOSTATE::CLOSED}, {"8", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_08) {
  initialize({PathToLlFiles + "typestate_08_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in foo()
      {6, {{"2", IOSTATE::OPENED}}},
      // At exit in main()
      {11, {{"2", IOSTATE::OPENED}, {"8", IOSTATE::UNINIT}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_09) {
  initialize({PathToLlFiles + "typestate_09_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in foo()
      {8,
       {
           {"4", IOSTATE::OPENED},
           //{"10", IOSTATE::OPENED}// 8 is before 10, so no info available
           // before retFF
       }},
      // At exit in main()
      {18, {{"4", IOSTATE::CLOSED}, {"10", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_10) {
  initialize({PathToLlFiles + "typestate_10_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in bar()
      {4, {{"2", IOSTATE::UNINIT}}},
      // At exit in foo()
      {11,
       {//{"2", IOSTATE::OPENED},
        //{"13", IOSTATE::OPENED}, // 2 and 13 are in different functions, so
        // results are not available before retFF
        {"5", IOSTATE::OPENED}}},
      // At exit in main()
      {19,
       {{"2", IOSTATE::CLOSED},
        {"5", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_11) {
  initialize({PathToLlFiles + "typestate_11_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in bar(): closing uninitialized file-handle gives error-state
      {6,
       {
           {"2", IOSTATE::ERROR},
           //{"7", IOSTATE::ERROR},
           //{"13", IOSTATE::ERROR} // 7 and 13 not yet reached
       }},
      // At exit in foo()
      {11,
       {
           //{"2", IOSTATE::OPENED}, // 2 is in different function
           {"7", IOSTATE::OPENED},
           //{"13", IOSTATE::OPENED}  // 13 is after 11
       }},
      // At exit in main(): due to aliasing the error-state from bar is
      // propagated back to main
      {19,
       {{"2", IOSTATE::ERROR}, {"7", IOSTATE::ERROR}, {"13", IOSTATE::ERROR}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_12) {
  initialize({PathToLlFiles + "typestate_12_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in bar()
      {6,
       {
           {"2", IOSTATE::OPENED},
           //{"10", IOSTATE::OPENED} // 6 has no information about 10, as it
           // always completes before
       }},
      // At exit in foo()
      {8, {{"2", IOSTATE::OPENED}, {"10", IOSTATE::OPENED}}},
      // At exit in main()
      {16, {{"2", IOSTATE::CLOSED}, {"10", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_13) {
  initialize({PathToLlFiles + "typestate_13_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Before first fclose()
      {8, {{"3", IOSTATE::OPENED}}},
      // Before second fclose()
      {10, {{"3", IOSTATE::CLOSED}}},
      // At exit in main()
      {11, {{"3", IOSTATE::ERROR}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_14) {
  initialize({PathToLlFiles + "typestate_14_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Before first fopen()
      {7, {{"5", IOSTATE::UNINIT}}},
      // Before second fopen()
      {9, {{"5", IOSTATE::OPENED}}},
      // After second store
      {11,
       {{"5", IOSTATE::OPENED},
        {"7", IOSTATE::OPENED},
        {"9", IOSTATE::OPENED}}},
      // At exit in main()
      {11,
       {{"5", IOSTATE::CLOSED},
        {"7", IOSTATE::CLOSED},
        {"9", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_15) {
  initialize({PathToLlFiles + "typestate_15_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // After store of ret val of first fopen()
      {9,
       {
           {"5", IOSTATE::OPENED}, {"7", IOSTATE::OPENED},
           // for 9, 11, 13 state is top
           // {"9", IOSTATE::OPENED},
           // {"11", IOSTATE::OPENED},
           // {"13", IOSTATE::OPENED}
       }},
      // After first fclose()
      {11,
       {
           {"5", IOSTATE::CLOSED},
           {"7", IOSTATE::CLOSED},
           {"9", IOSTATE::CLOSED},
           // for 11 and 13 state is top
           // {"11", IOSTATE::CLOSED},
           // {"13", IOSTATE::CLOSED}
       }},
      // After second fopen() but before storing ret val
      {12,
       {
           {"5", IOSTATE::CLOSED},
           {"7", IOSTATE::CLOSED},
           {"9", IOSTATE::CLOSED},
           {"11", IOSTATE::OPENED},
           // for 13 state is top
           //{"13", IOSTATE::CLOSED}
       }},
      // After storing ret val of second fopen()
      {13,
       {
           {"5", IOSTATE::OPENED},
           {"7", IOSTATE::CLOSED}, // 7 and 9 do not alias 11
           {"9", IOSTATE::CLOSED},
           {"11", IOSTATE::OPENED},
           // for 13 state is top
           //{"13", IOSTATE::OPENED}
       }},
      // At exit in main()
      {15,
       {{"5", IOSTATE::CLOSED},
        // Due to flow-insensitive alias information, the
        // closed file-handle (which has ID 13) may alias
        // the closed file handles 7 and 9. Hence closed
        // + closed gives error for 7 and 9 => false positive
        {"7", IOSTATE::ERROR},
        {"9", IOSTATE::ERROR},
        {"11", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_16) {
  initialize({PathToLlFiles + "typestate_16_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  // Llvmtssolver.dumpResults();

  // auto Pts = PT->getPointsToSet(IRDB->getInstruction(2));
  // std::cout << "PointsTo(2) = {";
  // bool Frst = true;
  // for (const auto *P : *Pts) {
  //   if (Frst) {
  //     Frst = false;
  //   } else {
  //     std::cout << ", ";
  //   }
  //   std::cout << llvmIRToShortString(P);
  // }
  // std::cout << "}" << std::endl;

  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in foo()
      {16,
       {
           {"2", IOSTATE::CLOSED},
           // {"18", IOSTATE::CLOSED} // pointsTo information is not sufficient
       }},
      // At exit in main()
      {24, {{"2", IOSTATE::CLOSED}, {"18", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

// TODO: Check this case again!
TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_17) {
  initialize({PathToLlFiles + "typestate_17_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();

  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Before loop
      {15,
       {{"2", IOSTATE::CLOSED},
        {"9", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}},
      // Before fgetc() // fgetc(CLOSED)=ERROR   join  CLOSED  = BOT
      {17,
       {
           {"2", IOSTATE::BOT}, {"9", IOSTATE::BOT}, {"13", IOSTATE::BOT},
           // {"16", IOSTATE::BOT} // at 16 we now have ERROR (actually, this is
           // correct as well as BOT)
       }},
      // At exit in main()
      {22,
       {
           {"2", IOSTATE::BOT}, {"9", IOSTATE::BOT}, {"13", IOSTATE::BOT},
           //{"16", IOSTATE::BOT} // at 16 we now have ERROR (actually, this is
           // correct as well as BOT)
       }}};
  compareResults(Gt, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_18) {
  initialize({PathToLlFiles + "typestate_18_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // At exit in foo()
      {17,
       {
           {"2", IOSTATE::CLOSED},
           // {"19", IOSTATE::CLOSED} // pointsTo information not sufficient
       }},
      // At exit in main()
      {25, {{"2", IOSTATE::CLOSED}, {"19", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

// TODO: Check this case again!
TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_19) {
  initialize({PathToLlFiles + "typestate_19_c.ll"});
  IDESolver_P<IDETypeStateAnalysis> Llvmtssolver(*TSProblem);

  Llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      {11, {{"8", IOSTATE::UNINIT}}},
      {14, {{"8", IOSTATE::BOT}}},
      // At exit in main()
      {25, {{"2", IOSTATE::CLOSED}, {"8", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
