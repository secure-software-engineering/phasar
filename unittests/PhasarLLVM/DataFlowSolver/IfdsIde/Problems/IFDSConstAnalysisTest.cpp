#include <memory>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "gtest/gtest.h"

#include "TestConfig.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSConstAnalysisTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles = unittest::PathToLLTestFiles + "constness/";
  const std::set<std::string> EntryPoints = {"main"};

  unique_ptr<ProjectIRDB> IRDB;
  unique_ptr<LLVMTypeHierarchy> TH;
  unique_ptr<LLVMBasedICFG> ICFG;
  unique_ptr<LLVMPointsToInfo> PT;
  unique_ptr<IFDSConstAnalysis> Constproblem;

  IFDSConstAnalysisTest() = default;
  ~IFDSConstAnalysisTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = make_unique<ProjectIRDB>(IRFiles, IRDBOptions::WPA);
    TH = make_unique<LLVMTypeHierarchy>(*IRDB);
    PT = make_unique<LLVMPointsToSet>(*IRDB);
    ICFG = make_unique<LLVMBasedICFG>(*IRDB, CallGraphAnalysisType::OTF,
                                      EntryPoints, TH.get(), PT.get());
    Constproblem = make_unique<IFDSConstAnalysis>(
        IRDB.get(), TH.get(), ICFG.get(), PT.get(), EntryPoints);
  }

  void SetUp() override {
    initializeLogger(false);
    ValueAnnotationPass::resetValueID();
  }

  void compareResults(const std::set<unsigned long> &GroundTruth,
                      IFDSSolver_P<IFDSConstAnalysis> &Solver) {
    IRDB->emitPreprocessedIR();
    std::set<const llvm::Value *> AllMutableAllocas;
    for (const auto *RR : IRDB->getRetOrResInstructions()) {
      std::set<const llvm::Value *> Facts = Solver.ifdsResultsAt(RR);
      for (const auto *Fact : Facts) {
        if (isAllocaInstOrHeapAllocaFunction(Fact) ||
            (llvm::isa<llvm::GlobalValue>(Fact) &&
             !Constproblem->isZeroValue(Fact))) {

          AllMutableAllocas.insert(Fact);
        }
      }
    }
    std::set<unsigned long> MutableIDs;
    for (const auto *Memloc : AllMutableAllocas) {
      std::cerr << "> Is Mutable: " << llvmIRToShortString(Memloc) << "\n";
      MutableIDs.insert(std::stoul(getMetaDataID(Memloc)));
    }
    EXPECT_EQ(GroundTruth, MutableIDs);
  }
};

/* ============== BASIC TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleBasicTest_01) {
  initialize({PathToLlFiles + "basic/basic_01_cpp_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_02) {
  initialize({PathToLlFiles + "basic/basic_02_cpp_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_03) {
  initialize({PathToLlFiles + "basic/basic_03_cpp_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_04) {
  initialize({PathToLlFiles + "basic/basic_04_cpp_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

/* ============== CONTROL FLOW TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCFForTest_01) {
  initialize({PathToLlFiles + "control_flow/cf_for_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFForTest_02) {
  initialize({PathToLlFiles + "control_flow/cf_for_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_01) {
  initialize({PathToLlFiles + "control_flow/cf_if_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_02) {
  initialize({PathToLlFiles + "control_flow/cf_if_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFWhileTest_01) {
  initialize({PathToLlFiles + "control_flow/cf_while_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

/* ============== POINTER TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandlePointerTest_01) {
  initialize({PathToLlFiles + "pointer/pointer_01_cpp_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_02) {
  initialize({PathToLlFiles + "pointer/pointer_02_cpp_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandlePointerTest_03) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "pointer/pointer_03_cpp_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({2, 3}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_04) {
  initialize({PathToLlFiles + "pointer/pointer_04_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({4}, Llvmconstsolver);
}

/* ============== GLOBAL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_01) {
  initialize({PathToLlFiles + "global/global_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_02) {
  initialize({PathToLlFiles + "global/global_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0, 1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_03) {
  initialize({PathToLlFiles + "global/global_03_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();

  /// The @llvm.global_ctors global variable is never immutable
  compareResults({0, /*1,*/ 2}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleGlobalTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "global/global_04_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0, 4}, Llvmconstsolver);
}

/* ============== CALL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_01) {
  initialize({PathToLlFiles + "call/param/call_param_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({5}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_02) {
  initialize({PathToLlFiles + "call/param/call_param_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({5}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_03) {
  initialize({PathToLlFiles + "call/param/call_param_03_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCallParamTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "call/param/call_param_04_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCallParamTest_05) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "call/param/call_param_05_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({2}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_06) {
  initialize({PathToLlFiles + "call/param/call_param_06_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_07) {
  initialize({PathToLlFiles + "call/param/call_param_07_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({6}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_08) {
  initialize({PathToLlFiles + "call/param/call_param_08_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({4}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_01) {
  initialize({PathToLlFiles + "call/return/call_ret_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_02) {
  initialize({PathToLlFiles + "call/return/call_ret_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_03) {
  initialize({PathToLlFiles + "call/return/call_ret_03_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0}, Llvmconstsolver);
}

/* ============== ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleArrayTest_01) {
  initialize({PathToLlFiles + "array/array_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_02) {
  initialize({PathToLlFiles + "array/array_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_03) {
  initialize({PathToLlFiles + "array/array_03_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleArrayTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/array_04_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_05) {
  initialize({PathToLlFiles + "array/array_05_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_06) {
  initialize({PathToLlFiles + "array/array_06_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  PT->print(std::cerr);
  compareResults({1}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleArrayTest_07) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/array_07_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_08) {
  initialize({PathToLlFiles + "array/array_08_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_09) {
  initialize({PathToLlFiles + "array/array_09_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0}, Llvmconstsolver);
}

/* ============== STL ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_01) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_02) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0, 1}, Llvmconstsolver);
}

PHASAR_SKIP_TEST(TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_03) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  initialize({PathToLlFiles + "array/stl_array/stl_array_03_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({0, 1, 2}, Llvmconstsolver);
})

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleSTLArrayTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/stl_array/stl_array_04_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_05) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_05_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_06) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_06_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({2}, Llvmconstsolver);
}

/* ============== CSTRING TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCStringTest_01) {
  initialize({PathToLlFiles + "array/cstring/cstring_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCStringTest_02) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/cstring/cstring_02_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSConstAnalysis> Llvmconstsolver(*Constproblem);
  Llvmconstsolver.solve();
  compareResults({2}, Llvmconstsolver);
}

/* ============== STRUCTURE TESTS ============== */
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_01) {
//  Initialize({pathToLLFiles + "structs/structs_01_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 5}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_02) {
//  Initialize({pathToLLFiles + "structs/structs_02_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 5}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_03) {
//  Initialize({pathToLLFiles + "structs/structs_03_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 9}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_04) {
//  Initialize({pathToLLFiles + "structs/structs_04_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 10}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_05) {
//  Initialize({pathToLLFiles + "structs/structs_05_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_06) {
//  Initialize({pathToLLFiles + "structs/structs_06_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_07) {
//  Initialize({pathToLLFiles + "structs/structs_07_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_08) {
//  Initialize({pathToLLFiles + "structs/structs_08_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_09) {
//  Initialize({pathToLLFiles + "structs/structs_09_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_10) {
//  Initialize({pathToLLFiles + "structs/structs_10_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_11) {
//  Initialize({pathToLLFiles + "structs/structs_11_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_12) {
//  Initialize({pathToLLFiles + "structs/structs_12_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
