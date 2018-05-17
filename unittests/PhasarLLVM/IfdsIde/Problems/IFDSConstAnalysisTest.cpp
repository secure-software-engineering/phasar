#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSConstAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>

/* ============== TEST FIXTURE ============== */

class IFDSConstAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToTests =
      /*PhasarDirectory + */ "../../../../../test/llvm_test_code/constness/";
  const std::vector<std::string> EntryPoints = {"main"};
  const std::set<std::string> IgnoredGlobalNames = {"llvm.used",
                                                    "llvm.compiler.used",
                                                    "llvm.global_ctors",
                                                    "llvm.global_dtors",
                                                    "vtable",
                                                    "typeinfo"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  IFDSConstAnalysis *constproblem;

  IFDSConstAnalysisTest() {}
  virtual ~IFDSConstAnalysisTest() {}

  void SetUp(const std::vector<std::string> &IRFiles) {
    initializeLogger(false);
    ValueAnnotationPass::resetValueID();
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG = new LLVMBasedICFG(*TH, *IRDB, WalkerStrategy::Pointer,
                             ResolveStrategy::OTF, EntryPoints);
    constproblem = new IFDSConstAnalysis(*ICFG, EntryPoints);
  }

  virtual void TearDown() override {
    PAMM_FACTORY;
    delete IRDB;
    delete TH;
    delete ICFG;
    delete constproblem;
    PAMM_RESET;
  }

  void
  compareResults(const std::set<unsigned long> &groundTruth,
                 LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> &solver) {
    // get all stack and heap alloca instructions
    // std::set<const llvm::Value *> allMemoryLoc =
    // IRDB->getAllocaInstructions();
    // add global varibales to the memory location set, except the llvm
    // intrinsic global variables
    // for (auto M : IRDB->getAllModules()) {
    //  for (auto &GV : M->globals()) {
    //    if (GV.hasName()) {
    //      string GVName = cxx_demangle(GV.getName().str());
    //      if (!IgnoredGlobalNames.count(GVName.substr(0, GVName.find(' ')))) {
    //        allMemoryLoc.insert(&GV);
    //      }
    //    }
    //  }
    //}
    std::set<const llvm::Value *> allMutableAllocas;
    for (auto RR : IRDB->getRetResInstructions()) {
      std::set<const llvm::Value *> facts = solver.ifdsResultsAt(RR);
      for (auto fact : facts) {
        if (isAllocaInstOrHeapAllocaFunction(fact) ||
            (llvm::isa<llvm::GlobalValue>(fact) &&
             !constproblem->isZeroValue(fact))) {
          allMutableAllocas.insert(fact);
        }
      }
      // Empty facts means the return/resume statement is part of not
      // analyzed function - remove all allocas of that function
      // if (facts.empty()) {
      //  const llvm::Function *F = RR->getParent()->getParent();
      //  for (auto mem_itr = allMemoryLoc.begin();
      //       mem_itr != allMemoryLoc.end();) {
      //    if (auto Inst = llvm::dyn_cast<llvm::Instruction>(*mem_itr)) {
      //      if (Inst->getParent()->getParent() == F) {
      //        mem_itr = allMemoryLoc.erase(mem_itr);
      //      } else {
      //        ++mem_itr;
      //      }
      //    } else {
      //      ++mem_itr;
      //    }
      //  }
      /*} else {
        for (auto fact : solver.ifdsResultsAt(RR)) {
          if (isAllocaInstOrHeapAllocaFunction(fact) ||
              llvm::isa<llvm::GlobalValue>(fact)) {
            allMemoryLoc.erase(fact);
          }
        }
      }*/
    }
    std::set<unsigned long> mutableIDs;
    for (auto memloc : allMutableAllocas) {
      memloc->print(llvm::outs());
      mutableIDs.insert(std::stoul(getMetaDataID(memloc)));
    }
    EXPECT_EQ(groundTruth, mutableIDs);
  }
};

/* ============== BASIC TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleBasicTest_01) {
  SetUp({pathToTests + "basic/basic_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_02) {
  SetUp({pathToTests + "basic/basic_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_03) {
  SetUp({pathToTests + "basic/basic_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_04) {
  SetUp({pathToTests + "basic/basic_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

/* ============== CONTROL FLOW TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCFForTest_01) {
  SetUp({pathToTests + "control_flow/cf_for_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFForTest_02) {
  SetUp({pathToTests + "control_flow/cf_for_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_01) {
  SetUp({pathToTests + "control_flow/cf_if_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_02) {
  SetUp({pathToTests + "control_flow/cf_if_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFWhileTest_01) {
  SetUp({pathToTests + "control_flow/cf_while_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

/* ============== POINTER TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandlePointerTest_01) {
  SetUp({pathToTests + "pointer/pointer_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_02) {
  SetUp({pathToTests + "pointer/pointer_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_03) {
  SetUp({pathToTests + "pointer/pointer_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({2, 3}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_04) {
  SetUp({pathToTests + "pointer/pointer_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({3}, llvmconstsolver);
}

/* ============== GLOBAL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_01) {
  SetUp({pathToTests + "global/global_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_02) {
  SetUp({pathToTests + "global/global_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0, 1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_03) {
  SetUp({pathToTests + "global/global_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_04) {
  SetUp({pathToTests + "global/global_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0, 4}, llvmconstsolver);
}

/* ============== CALL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_01) {
  SetUp({pathToTests + "call/param/call_param_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({4}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_02) {
  SetUp({pathToTests + "call/param/call_param_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({4}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_03) {
  SetUp({pathToTests + "call/param/call_param_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_04) {
  SetUp({pathToTests + "call/param/call_param_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_05) {
  SetUp({pathToTests + "call/param/call_param_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_06) {
  SetUp({pathToTests + "call/param/call_param_06.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_07) {
  SetUp({pathToTests + "call/param/call_param_07.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({4}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_08) {
  SetUp({pathToTests + "call/param/call_param_08.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({3}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_01) {
  SetUp({pathToTests + "call/return/call_ret_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_02) {
  SetUp({pathToTests + "call/return/call_ret_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_03) {
  SetUp({pathToTests + "call/return/call_ret_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

/* ============== ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleArrayTest_01) {
  SetUp({pathToTests + "array/array_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_02) {
  SetUp({pathToTests + "array/array_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_03) {
  SetUp({pathToTests + "array/array_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_04) {
  SetUp({pathToTests + "array/array_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_05) {
  SetUp({pathToTests + "array/array_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_06) {
  SetUp({pathToTests + "array/array_06.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_07) {
  SetUp({pathToTests + "array/array_07.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_08) {
  SetUp({pathToTests + "array/array_08.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_09) {
  SetUp({pathToTests + "array/array_09.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

/* ============== STL ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_01) {
  SetUp({pathToTests + "array/stl_array/stl_array_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_02) {
  SetUp({pathToTests + "array/stl_array/stl_array_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_03) {
  SetUp({pathToTests + "array/stl_array/stl_array_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_04) {
  SetUp({pathToTests + "array/stl_array/stl_array_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_05) {
  SetUp({pathToTests + "array/stl_array/stl_array_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_06) {
  SetUp({pathToTests + "array/stl_array/stl_array_06.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

/* ============== CSTRING TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCStringTest_01) {
  SetUp({pathToTests + "array/cstring/cstring_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCStringTest_02) {
  SetUp({pathToTests + "array/cstring/cstring_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

/* ============== STRUCTURE TESTS ============== */
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_01) {
//  SetUp({pathToTests + "structs/structs_01.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 5}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_02) {
//  SetUp({pathToTests + "structs/structs_02.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 5}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_03) {
//  SetUp({pathToTests + "structs/structs_03.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 9}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_04) {
//  SetUp({pathToTests + "structs/structs_04.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 10}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_05) {
//  SetUp({pathToTests + "structs/structs_05.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0, 1}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_06) {
//  SetUp({pathToTests + "structs/structs_06.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_07) {
//  SetUp({pathToTests + "structs/structs_07.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_08) {
//  SetUp({pathToTests + "structs/structs_08.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_09) {
//  SetUp({pathToTests + "structs/structs_09.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_10) {
//  SetUp({pathToTests + "structs/structs_10.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_11) {
//  SetUp({pathToTests + "structs/structs_11.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_12) {
//  SetUp({pathToTests + "structs/structs_12.ll"});
//  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
//      *constproblem, false);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
