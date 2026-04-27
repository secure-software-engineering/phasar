
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSConstAnalysis.h"

#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <initializer_list>

using namespace psr;
using namespace psr::unittest;

/* ============== TEST FIXTURE ============== */

class IFDSConstAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles = PHASAR_BUILD_SUBFOLDER("constness/");
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;

  std::optional<IFDSConstAnalysis> Constproblem;
  std::vector<const llvm::Instruction *> RetOrResInstructions;

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(IRFile, EntryPoints);
    Constproblem = createAnalysisProblem<IFDSConstAnalysis>(*HA, EntryPoints);
  }

  llvm::ArrayRef<const llvm::Instruction *> getRetOrResInstructions() {
    if (!RetOrResInstructions.empty()) {
      return RetOrResInstructions;
    }

    for (const auto *Fun : HA->getProjectIRDB().getAllFunctions()) {
      for (const auto &Inst : llvm::instructions(Fun)) {
        if (llvm::isa<llvm::ReturnInst>(&Inst) ||
            llvm::isa<llvm::ResumeInst>(&Inst)) {
          RetOrResInstructions.push_back(&Inst);
        }
      }
    }
    return RetOrResInstructions;
  }

  void compareResultsImpl(const std::set<const llvm::Value *> &GroundTruth,
                          auto &&SR) {
    std::set<const llvm::Value *> AllMutableAllocas;

    for (const auto *RR : getRetOrResInstructions()) {
      std::set<const llvm::Value *> Facts = SR.ifdsResultsAt(RR);
      for (const auto *Fact : Facts) {
        if (isAllocaInstOrHeapAllocaFunction(Fact) ||
            (llvm::isa<llvm::GlobalValue>(Fact) &&
             !Constproblem->isZeroValue(Fact))) {
          llvm::outs() << "Found *Fact: " << *Fact << "\n";
          AllMutableAllocas.insert(Fact);
        }
      }
    }

    EXPECT_EQ(GroundTruth, AllMutableAllocas);
  }

  void compareResults(const std::set<TestingSrcLocation> &GroundTruth,
                      auto &Solver) {
    auto GroundTruthEntries =
        convertTestingLocationSetInIR(GroundTruth, HA->getProjectIRDB());

    compareResultsImpl(GroundTruthEntries, Solver.getSolverResults());
  }
  void compareResults(std::initializer_list<TestingSrcLocation> GroundTruth,
                      auto &Solver) {
    auto GroundTruthEntries =
        convertTestingLocationSetInIR(GroundTruth, HA->getProjectIRDB());

    compareResultsImpl(GroundTruthEntries, Solver.getSolverResults());
  }
};

/* ============== BASIC TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleBasicTest_01) {
  initialize({PathToLlFiles + "basic/basic_01_cpp_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  std::set<TestingSrcLocation> GroundTruth;
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_02) {
  initialize({PathToLlFiles + "basic/basic_02_cpp_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_03) {
  initialize({PathToLlFiles + "basic/basic_03_cpp_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_04) {
  initialize({PathToLlFiles + "basic/basic_04_cpp_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

/* ============== CONTROL FLOW TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCFForTest_01) {
  initialize({PathToLlFiles + "control_flow/cf_for_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 12, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFForTest_02) {
  initialize({PathToLlFiles + "control_flow/cf_for_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{4, 12, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_01) {
  initialize({PathToLlFiles + "control_flow/cf_if_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{4, 12, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_02) {
  initialize({PathToLlFiles + "control_flow/cf_if_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  std::set<TestingSrcLocation> GroundTruth{};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFWhileTest_01) {
  initialize({PathToLlFiles + "control_flow/cf_while_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{5, 12, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

/* ============== POINTER TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandlePointerTest_01) {
  initialize({PathToLlFiles + "pointer/pointer_01_cpp_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_02) {
  initialize({PathToLlFiles + "pointer/pointer_02_cpp_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandlePointerTest_03) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "pointer/pointer_03_cpp_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{4, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_04) {
  initialize({PathToLlFiles + "pointer/pointer_04_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{5, 7, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

/* ============== GLOBAL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_01) {
  initialize({PathToLlFiles + "global/global_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = GlobalVar{"g1"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_02) {
  initialize({PathToLlFiles + "global/global_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = GlobalVar{"g"};
  auto EntryTwo = LineColFun{4, 7, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry, EntryTwo};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_03) {
  initialize({PathToLlFiles + "global/global_03_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();

  auto Entry = LineColFun{6, 10, "__cxx_global_var_init"};
  auto EntryTwo = GlobalVar{"g"};

  std::set<TestingSrcLocation> GroundTruth{Entry, EntryTwo};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleGlobalTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "global/global_04_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = GlobalVar{"g1"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

/* ============== CALL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_01) {
  initialize({PathToLlFiles + "call/param/call_param_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{5, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_02) {
  initialize({PathToLlFiles + "call/param/call_param_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{5, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_03) {
  initialize({PathToLlFiles + "call/param/call_param_03_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  // auto Entry = LineColFun{, , "main"};
  // std::set<TestingSrcLocation> GroundTruth{Entry};
  std::set<TestingSrcLocation> GroundTruth{};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCallParamTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "call/param/call_param_04_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCallParamTest_05) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "call/param/call_param_05_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_06) {
  initialize({PathToLlFiles + "call/param/call_param_06_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_07) {
  initialize({PathToLlFiles + "call/param/call_param_07_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{6, 12, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_08) {
  initialize({PathToLlFiles + "call/param/call_param_08_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{9, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_01) {
  initialize({PathToLlFiles + "call/return/call_ret_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_02) {
  initialize({PathToLlFiles + "call/return/call_ret_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 12, "_Z3foov"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_03) {
  initialize({PathToLlFiles + "call/return/call_ret_03_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 12, "_Z3foov"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

/* ============== ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleArrayTest_01) {
  initialize({PathToLlFiles + "array/array_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_02) {
  initialize({PathToLlFiles + "array/array_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_03) {
  initialize({PathToLlFiles + "array/array_03_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleArrayTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/array_04_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_05) {
  initialize({PathToLlFiles + "array/array_05_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_06) {
  initialize({PathToLlFiles + "array/array_06_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  HA->getAliasInfo().print(llvm::errs());
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleArrayTest_07) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/array_07_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_08) {
  initialize({PathToLlFiles + "array/array_08_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_09) {
  initialize({PathToLlFiles + "array/array_09_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{3, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

/* ============== STL ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_01) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_02) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  std::set<TestingSrcLocation> GroundTruth = {
      LineColFun{4, 0, "main"},
      GlobalVar{"__const.main.a"},
  };

  compareResults(GroundTruth, Llvmconstsolver);
}

PHASAR_SKIP_TEST(TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_03) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  initialize({PathToLlFiles + "array/stl_array/stl_array_03_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults(
      {
          GlobalVar{"__const.main.a"},
          GlobalVar{".str"},
          LineColFun{4, 0, "main"},
      },
      Llvmconstsolver);
})

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleSTLArrayTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/stl_array/stl_array_04_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_05) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_05_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_06) {
  initialize({PathToLlFiles + "array/stl_array/stl_array_06_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{4, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
}

/* ============== CSTRING TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCStringTest_01) {
  initialize({PathToLlFiles + "array/cstring/cstring_01_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  compareResults({}, Llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCStringTest_02) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  initialize({PathToLlFiles + "array/cstring/cstring_02_cpp_m2r_dbg.ll"});
  IFDSSolver Llvmconstsolver(*Constproblem, &HA->getICFG());
  Llvmconstsolver.solve();
  auto Entry = LineColFun{4, 0, "main"};
  std::set<TestingSrcLocation> GroundTruth{Entry};
  compareResults(GroundTruth, Llvmconstsolver);
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
