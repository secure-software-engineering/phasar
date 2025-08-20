
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "SourceMapping.h"
#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <optional>
#include <tuple>

using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSUninitializedVariablesTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("uninitialized_variables/");
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;

  std::optional<IFDSUninitializedVariables> UninitProblem;

  IFDSUninitializedVariablesTest() = default;
  ~IFDSUninitializedVariablesTest() override = default;

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(IRFile, EntryPoints);
    UninitProblem =
        createAnalysisProblem<IFDSUninitializedVariables>(*HA, EntryPoints);
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  void compareResults(
      const std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>>
          &GroundTruth) {
    auto GroundTruthEntries = getGroundTruthInsts(GroundTruth, true);

    std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
        FoundUninitUses;
    for (const auto &Kvp : UninitProblem->getAllUndefUses()) {
      const auto *SourceInst = Kvp.first;
      llvm::outs() << "SourceInst: " << SourceInst << "\n";
      llvm::outs() << "*SourceInst: " << *SourceInst << "\n";

      for (const auto *UV : Kvp.second) {
        llvm::outs() << "UV: " << UV << "\n";
        llvm::outs() << "*UV: " << *UV << "\n\n";
        FoundUninitUses.insert({SourceInst, UV});
      }
    }

    EXPECT_EQ(FoundUninitUses, GroundTruthEntries);
  }
}; // Test Fixture

TEST_F(IFDSUninitializedVariablesTest, UninitTest_01_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "all_uninit_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // all_uninit.cpp does not contain undef-uses
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;
  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_02_SHOULD_LEAK) {
  initialize({PathToLlFiles + "binop_uninit_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // binop_uninit uses uninitialized variable i in 'int j = i + 10;'
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  // %4 = load i32, i32* %2, ID: 6 ;  %2 is the uninitialized variable i
  // %5 = add nsw i32 %4, 10 ;        %4 is undef, since it is loaded from
  // undefined alloca; not sure if it is necessary to report again
  const auto Entry =
      SrcCodeLocationEntry(2, 0, HA->getICFG().getFunction("main"));
  const auto EntryTwo =
      SrcCodeLocationEntry(3, 11, HA->getICFG().getFunction("main"));
  const auto EntryThree =
      SrcCodeLocationEntry(3, 13, HA->getICFG().getFunction("main"));
  GroundTruth.insert({EntryTwo, Entry});
  GroundTruth.insert({EntryThree, EntryTwo});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_03_SHOULD_LEAK) {
  initialize({PathToLlFiles + "callnoret_c_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // callnoret uses uninitialized variable a in 'return a + 10;' of addTen(int)
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  const auto IntA =
      SrcCodeLocationEntry(7, 7, HA->getICFG().getFunction("main"));
  const auto CopyA =
      SrcCodeLocationEntry(9, 10, HA->getICFG().getFunction("main"));
  const auto ArgA =
      SrcCodeLocationEntry(1, 16, HA->getICFG().getFunction("addTen"));
  const auto LoadA =
      SrcCodeLocationEntry(3, 10, HA->getICFG().getFunction("addTen"));
  const auto Add =
      SrcCodeLocationEntry(3, 12, HA->getICFG().getFunction("addTen"));
  GroundTruth.insert({CopyA, IntA});
  GroundTruth.insert({Add, LoadA});
  GroundTruth.insert({LoadA, ArgA});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_04_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "ctor_default_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // ctor.cpp does not contain undef-uses
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  compareResults(GroundTruth);
}
TEST_F(IFDSUninitializedVariablesTest, UninitTest_05_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "struct_member_init_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // struct_member_init.cpp does not contain undef-uses
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_06_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "struct_member_uninit_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // struct_member_uninit.cpp does not contain undef-uses
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  compareResults(GroundTruth);
}
/****************************************************************************************
 * fails due to field-insensitivity + struct ignorance + clang compiler hacks
 *
*****************************************************************************************
TEST_F(IFDSUninitializedVariablesTest, UninitTest_07_SHOULD_LEAK) {
  Initialize({pathToLLFiles + "struct_member_uninit2_cpp_dbg.ll"});
  IFDSSolver<IFDSUninitializedVariables::n_t,
IFDSUninitializedVariables::d_t,IFDSUninitializedVariables::f_t,IFDSUninitializedVariables::t_t,IFDSUninitializedVariables::v_t,IFDSUninitializedVariables::i_t>
Solver(*UninitProblem, false); Solver.solve();
  // struct_member_uninit2.cpp contains a use of the uninitialized field _x.b
  map<int, set<string>> GroundTruth;
  // %5 = load i16, i16* %4; %4 is the uninitialized struct-member _x.b
  GroundTruth[4] = {"3"};



  compareResults(GroundTruth);
}
*****************************************************************************************/

TEST_F(IFDSUninitializedVariablesTest, UninitTest_08_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "global_variable_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // global_variable.cpp does not contain undef-uses
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  compareResults(GroundTruth);
}
/****************************************************************************************
 * failssince @i is uninitialized in the c++ code, but initialized in the
 * LLVM-IR
 *
*****************************************************************************************
TEST_F(IFDSUninitializedVariablesTest, UninitTest_09_SHOULD_LEAK) {
  Initialize({pathToLLFiles + "global_variable_cpp_dbg.ll"});
  IFDSSolver<IFDSUninitializedVariables::n_t,
IFDSUninitializedVariables::d_t,IFDSUninitializedVariables::f_t,IFDSUninitializedVariables::t_t,IFDSUninitializedVariables::v_t,IFDSUninitializedVariables::i_t>
Solver(*UninitProblem, false); Solver.solve();
  // global_variable.cpp does not contain undef-uses
  map<int, set<string>> GroundTruth;
  // load i32, i32* @i
  GroundTruth[5] = {"0"};
  compareResults(GroundTruth);
}
*****************************************************************************************/

TEST_F(IFDSUninitializedVariablesTest, UninitTest_10_SHOULD_LEAK) {
  initialize({PathToLlFiles + "return_uninit_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  const auto IntI =
      SrcCodeLocationEntry(2, 7, HA->getICFG().getFunction("_Z3foov"));
  const auto UseOfI =
      SrcCodeLocationEntry(3, 10, HA->getICFG().getFunction("_Z3foov"));

  GroundTruth.insert({UseOfI, IntI});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_11_SHOULD_NOT_LEAK) {

  initialize({PathToLlFiles + "sanitizer_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;
  // all undef-uses are sanitized;
  // However, the uninitialized variable j is read, which causes the analysis to
  // report an undef-use
  // 6 => {2}

  const auto IntI =
      SrcCodeLocationEntry(3, 7, HA->getICFG().getFunction("main"));
  const auto UseOfI =
      SrcCodeLocationEntry(4, 7, HA->getICFG().getFunction("main"));

  GroundTruth.insert({UseOfI, IntI});

  compareResults(GroundTruth);
}

//---------------------------------------------------------------------
// Not relevant any more; Test case covered by UninitTest_11
//---------------------------------------------------------------------
/* TEST_F(IFDSUninitializedVariablesTest, UninitTest_12_SHOULD_LEAK) {

  Initialize({pathToLLFiles + "sanitizer_uninit_cpp_dbg.ll"});
  IFDSSolver<IFDSUninitializedVariables::n_t,
IFDSUninitializedVariables::d_t,IFDSUninitializedVariables::f_t,IFDSUninitializedVariables::t_t,IFDSUninitializedVariables::v_t,IFDSUninitializedVariables::i_t>
Solver(*UninitProblem, true); Solver.solve();
  // The sanitized value is not used always => the phi-node is "tainted"
  map<int, set<string>> GroundTruth;
  GroundTruth[6] = {"2"};
  GroundTruth[13] = {"2"};
  compareResults(GroundTruth);
}
*/
TEST_F(IFDSUninitializedVariablesTest, UninitTest_13_SHOULD_NOT_LEAK) {

  initialize({PathToLlFiles + "sanitizer2_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // The undef-uses do not affect the program behaviour, but are of course still
  // found and reported
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  const auto IntJ =
      SrcCodeLocationEntry(3, 7, HA->getICFG().getFunction("main"));
  const auto LoadJ =
      SrcCodeLocationEntry(5, 7, HA->getICFG().getFunction("main"));

  GroundTruth.insert({LoadJ, IntJ});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_14_SHOULD_LEAK) {

  initialize({PathToLlFiles + "uninit_c_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  const auto IntA =
      SrcCodeLocationEntry(2, 7, HA->getICFG().getFunction("main"));
  const auto IntB =
      SrcCodeLocationEntry(3, 7, HA->getICFG().getFunction("main"));
  const auto LoadA =
      SrcCodeLocationEntry(6, 11, HA->getICFG().getFunction("main"));
  const auto Multiply =
      SrcCodeLocationEntry(6, 13, HA->getICFG().getFunction("main"));
  const auto LoadB =
      SrcCodeLocationEntry(6, 15, HA->getICFG().getFunction("main"));

  GroundTruth.insert({LoadA, IntA});
  GroundTruth.insert({LoadB, IntB});
  GroundTruth.insert({Multiply, LoadA});
  GroundTruth.insert({Multiply, LoadB});

  compareResults(GroundTruth);
}

/****************************************************************************************
 * Fails probably due to field-insensitivity
 *
*****************************************************************************************
TEST_F(IFDSUninitializedVariablesTest, UninitTest_15_SHOULD_LEAK) {
  Initialize({pathToLLFiles + "dyn_mem_cpp_dbg.ll"});
  IFDSSolver<IFDSUninitializedVariables::n_t,
IFDSUninitializedVariables::d_t,IFDSUninitializedVariables::f_t,IFDSUninitializedVariables::t_t,IFDSUninitializedVariables::v_t,IFDSUninitializedVariables::i_t>
Solver(*UninitProblem, false); Solver.solve(); map<int, set<string>>
GroundTruth;
  // TODO remove GT[14] and GT[13]
  GroundTruth[14] = {"3"};
  GroundTruth[13] = {"2"};
  GroundTruth[15] = {"13", "14"};

  GroundTruth[35] = {"4"};
  GroundTruth[38] = {"35"};

  GroundTruth[28] = {"2"};
  GroundTruth[29] = {"3"};
  GroundTruth[30] = {"28", "29"};

  GroundTruth[33] = {"30"};

  // Analysis detects false positive at %12:

  // store i32* %3, i32** %6, align 8, !dbg !28
  // %12 = load i32*, i32** %6, align 8, !dbg !29


  compareResults(GroundTruth);
}
*****************************************************************************************/

TEST_F(IFDSUninitializedVariablesTest, UninitTest_16_SHOULD_LEAK) {

  initialize({PathToLlFiles + "growing_example_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  const auto ArgX =
      SrcCodeLocationEntry(1, 18, HA->getICFG().getFunction("_Z8functionii"));
  const auto IntI =
      SrcCodeLocationEntry(2, 7, HA->getICFG().getFunction("_Z8functionii"));
  const auto LoadX =
      SrcCodeLocationEntry(3, 11, HA->getICFG().getFunction("_Z8functionii"));
  const auto LoadI =
      SrcCodeLocationEntry(5, 10, HA->getICFG().getFunction("_Z8functionii"));
  const auto Add =
      SrcCodeLocationEntry(5, 12, HA->getICFG().getFunction("_Z8functionii"));
  const auto IntJ =
      SrcCodeLocationEntry(10, 7, HA->getICFG().getFunction("main"));
  const auto LoadJ =
      SrcCodeLocationEntry(12, 16, HA->getICFG().getFunction("main"));

  // TODO: rewrite comment below
  // TODO remove GT[11]
  GroundTruth.insert({LoadX, ArgX});
  GroundTruth.insert({LoadI, IntI});
  GroundTruth.insert({Add, LoadI});
  GroundTruth.insert({LoadJ, IntJ});
#if false
  // TODO remove GT[11]
  GroundTruth[11] = {"0"};

  GroundTruth[16] = {"2"};
  GroundTruth[18] = {"16"};
  GroundTruth[34] = {"24"};
#endif

  compareResults(GroundTruth);
}

/****************************************************************************************
 * Fails due to struct ignorance; general problem with field sensitivity: when
 * all structs would be treated as uninitialized per default, the analysis would
 * not be able to detect correct constructor calls
 *
*****************************************************************************************
TEST_F(IFDSUninitializedVariablesTest, UninitTest_17_SHOULD_LEAK) {

  Initialize({pathToLLFiles + "struct_test_cpp.ll"});
  IFDSSolver<IFDSUninitializedVariables::n_t,
IFDSUninitializedVariables::d_t,IFDSUninitializedVariables::f_t,IFDSUninitializedVariables::t_t,IFDSUninitializedVariables::v_t,IFDSUninitializedVariables::i_t>
Solver(*UninitProblem, false); Solver.solve();

  map<int, set<string>> GroundTruth;
  // printf should leak both parameters => fails

  GroundTruth[8] = {"5", "7"};
  compareResults(GroundTruth);
}
*****************************************************************************************/
/****************************************************************************************
 * Fails, since the analysis is not able to detect memcpy calls
 *
*****************************************************************************************
TEST_F(IFDSUninitializedVariablesTest, UninitTest_18_SHOULD_NOT_LEAK) {

  Initialize({pathToLLFiles + "array_init_cpp.ll"});
  IFDSSolver<IFDSUninitializedVariables::n_t,
IFDSUninitializedVariables::d_t,IFDSUninitializedVariables::f_t,IFDSUninitializedVariables::t_t,IFDSUninitializedVariables::v_t,IFDSUninitializedVariables::i_t>
Solver(*UninitProblem, false); Solver.solve();

  map<int, set<string>> GroundTruth;
  //

  compareResults(GroundTruth);
}
*****************************************************************************************/
/****************************************************************************************
 * fails due to missing alias information (and missing field/array element
 *information)
 *
*****************************************************************************************
TEST_F(IFDSUninitializedVariablesTest, UninitTest_19_SHOULD_NOT_LEAK) {

  Initialize({pathToLLFiles + "array_init_simple_cpp.ll"});
  IFDSSolver<IFDSUninitializedVariables::n_t,
IFDSUninitializedVariables::d_t,IFDSUninitializedVariables::f_t,IFDSUninitializedVariables::t_t,IFDSUninitializedVariables::v_t,IFDSUninitializedVariables::i_t>
Solver(*UninitProblem, false); Solver.solve();

  map<int, set<string>> GroundTruth;
  compareResults(GroundTruth);
}
*****************************************************************************************/

TEST_F(IFDSUninitializedVariablesTest, UninitTest_20_SHOULD_LEAK) {

  initialize({PathToLlFiles + "recursion_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  const auto ArgAddrX =
      SrcCodeLocationEntry(2, 15, HA->getICFG().getFunction("_Z3fooRii"));
  const auto LoadX =
      SrcCodeLocationEntry(4, 12, HA->getICFG().getFunction("_Z3fooRii"));
  const auto LoadXTwo =
      SrcCodeLocationEntry(5, 14, HA->getICFG().getFunction("_Z3fooRii"));
  const auto FooExit =
      SrcCodeLocationEntry(6, 1, HA->getICFG().getFunction("_Z3fooRii"));
  // const auto IntN =
  //     SrcCodeLocationEntry(3, 7, HA->getICFG().getFunction("_Z3fooRii"));
  const auto IntI =
      SrcCodeLocationEntry(9, 7, HA->getICFG().getFunction("main"));
  const auto IntJ =
      SrcCodeLocationEntry(10, 7, HA->getICFG().getFunction("main"));
  const auto RetOfFoo =
      SrcCodeLocationEntry(10, 11, HA->getICFG().getFunction("main"));
  const auto LoadJ =
      SrcCodeLocationEntry(11, 18, HA->getICFG().getFunction("main"));

  // TODO: ask fabian why the original unittest ground truth has 5 elements
  // TODO: Also, why doesn't this crash if I say getFunction("main") when
  // looking at insts in foo?

  // GroundTruth.insert({LoadXTwo, IntI});

  // Leaks due to field-insensitivity
  GroundTruth.insert({LoadX, ArgAddrX});
  GroundTruth.insert({LoadXTwo, ArgAddrX});

  // Load uninitialized variable i
  // GroundTruth.insert({LoadI, IntI});

  // Load recursive return-value for returning it
  GroundTruth.insert({LoadJ, IntJ});
  //
  // // Load return-value of foo in main
  // GroundTruth.insert({FooExit, IntJ});

  // ***********
  // Load recursive return-value for returning it
  // GroundTruth.insert({RetOfFoo, IntJ});
  // ***********

  // Load return-value of foo in main
  // GroundTruth.insert({IntJ, RetOfFoo});

  compareResults(GroundTruth);

#if false

  // Leaks at 11 and 14 due to field-insensitivity
  GroundTruth[11] = {"2"};
  GroundTruth[14] = {"2"};

  // Load uninitialized variable i
  GroundTruth[31] = {"24"};
  // Load recursive return-value for returning it
  GroundTruth[20] = {"1"};
  // Load return-value of foo in main
  GroundTruth[29] = {"28"};
  // Analysis does not check uninit on actualparameters
  // GroundTruth[32] = {"31"};
  compareResults(GroundTruth);

#endif
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_21_SHOULD_LEAK) {

  initialize({PathToLlFiles + "virtual_call_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  const auto FooXAddr =
      SrcCodeLocationEntry(3, 15, HA->getICFG().getFunction("_Z3fooRi"));
  const auto FooXLoad =
      SrcCodeLocationEntry(3, 27, HA->getICFG().getFunction("_Z3fooRi"));

  const auto BarXAddr =
      SrcCodeLocationEntry(4, 15, HA->getICFG().getFunction("_Z3barRi"));
  const auto Load =
      SrcCodeLocationEntry(5, 3, HA->getICFG().getFunction("_Z3barRi"));
  const auto LoadX =
      SrcCodeLocationEntry(6, 10, HA->getICFG().getFunction("_Z3barRi"));
  const auto IntI =
      SrcCodeLocationEntry(9, 7, HA->getICFG().getFunction("main"));
  const auto IntJ =
      SrcCodeLocationEntry(16, 7, HA->getICFG().getFunction("main"));
  const auto BazCall =
      SrcCodeLocationEntry(16, 11, HA->getICFG().getFunction("main"));
  const auto BazCall2 =
      SrcCodeLocationEntry(16, 11, HA->getICFG().getFunction("main"));
  const auto LoadJ =
      SrcCodeLocationEntry(17, 10, HA->getICFG().getFunction("main"));
  // is passed as a reference, so I isn't being loaded here
  // const auto LoadI =
  //     SrcCodeLocationEntry(16, 15, HA->getICFG().getFunction("main"));

  // 3  => {0}; due to field-insensitivity
  GroundTruth.insert({FooXLoad, FooXAddr});

  // 8  => {5}; due to field-insensitivity
  GroundTruth.insert({Load, BarXAddr});

  // 10 => {5}; due to alias-unawareness
  GroundTruth.insert({LoadX, BarXAddr});
  // 35 => {34}; actual leak
  GroundTruth.insert({BazCall, IntJ});
  // 37 => {17}; actual leak
  GroundTruth.insert({BazCall2, IntJ});

#if false
  map<int, set<string>> GroundTruth = {
      {3, {"0"}}, {8, {"5"}}, {10, {"5"}}, {35, {"34"}}, {37, {"17"}}};
  // 3  => {0}; due to field-insensitivity
  // 8  => {5}; due to field-insensitivity
  // 10 => {5}; due to alias-unawareness
  // 35 => {34}; actual leak
  // 37 => {17}; actual leak
#endif
  compareResults(GroundTruth);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
