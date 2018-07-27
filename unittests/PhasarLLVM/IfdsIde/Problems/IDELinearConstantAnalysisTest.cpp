#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */

class IDELinearConstantAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
    PhasarDirectory + "build/test/llvm_test_code/linear_constant/";
  const std::vector<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  IDELinearConstantAnalysis *LCAProblem;

  IDELinearConstantAnalysisTest() = default;
  virtual ~IDELinearConstantAnalysisTest() = default;

  void SetUp(const std::vector<std::string> &IRFiles) {
    initializeLogger(false);
    ValueAnnotationPass::resetValueID();
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG = new LLVMBasedICFG(*TH, *IRDB, WalkerStrategy::Pointer,
                             ResolveStrategy::OTF, EntryPoints);
    LCAProblem = new IDELinearConstantAnalysis(*ICFG, EntryPoints);
  }

  virtual void TearDown() override {
    PAMM_FACTORY;
    delete IRDB;
    delete TH;
    delete ICFG;
    delete LCAProblem;
    PAMM_RESET;
  }

  void
  compareResults(const std::set<unsigned long> &groundTruth,
                 LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> &solver) {
    // std::set<const llvm::Value *> allMutableAllocas;
    // for (auto RR : IRDB->getRetResInstructions()) {
    //   std::set<const llvm::Value *> facts = solver.ifdsResultsAt(RR);
    //   for (auto fact : facts) {
    //     if (isAllocaInstOrHeapAllocaFunction(fact) ||
    //         (llvm::isa<llvm::GlobalValue>(fact) &&
    //          !constproblem->isZeroValue(fact))) {
    //       allMutableAllocas.insert(fact);
    //     }
    //   }
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
    // }
    // std::set<unsigned long> mutableIDs;
    // for (auto memloc : allMutableAllocas) {
    //   memloc->print(llvm::outs());
    //   mutableIDs.insert(std::stoul(getMetaDataID(memloc)));
    // }
    // EXPECT_EQ(groundTruth, mutableIDs);
    std::cout << '\n';
  }
};

/* ============== BASIC TESTS ============== */
TEST_F(IDELinearConstantAnalysisTest, HandleBasicTest_01) {
  SetUp({pathToLLFiles + "basic_01.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
    *LCAProblem, true);
  llvmlcasolver.solve();
  compareResults({}, llvmlcasolver);
}


// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
