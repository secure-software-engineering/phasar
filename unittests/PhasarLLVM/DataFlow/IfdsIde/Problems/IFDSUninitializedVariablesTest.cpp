
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"

#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/Utils/DebugOutput.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <optional>

using namespace psr;
using namespace psr::unittest;

/* ============== TEST FIXTURE ============== */

class IFDSUninitializedVariablesTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("uninitialized_variables/");
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;

  std::optional<IFDSUninitializedVariables> UninitProblem;

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(IRFile, EntryPoints);
    UninitProblem =
        createAnalysisProblem<IFDSUninitializedVariables>(*HA, EntryPoints);
  }

  using GroundTruthTy =
      std::map<TestingSrcLocation, std::set<TestingSrcLocation>>;

  void compareResults(const GroundTruthTy &GroundTruthEntries) {
    auto ConvGroundTruth = convertTestingLocationSetMapInIR(
        GroundTruthEntries, HA->getProjectIRDB());

    EXPECT_EQ(UninitProblem->getAllUndefUses(), ConvGroundTruth)
        << "UndefUses do not match:\n  Expected: "
        << PrettyPrinter{ConvGroundTruth}
        << "\n  Got: " << PrettyPrinter{UninitProblem->getAllUndefUses()};
  }

}; // Test Fixture

TEST_F(IFDSUninitializedVariablesTest, UninitTest_01_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "all_uninit_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // all_uninit.cpp does not contain undef-uses
  GroundTruthTy GroundTruth;
  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_02_SHOULD_LEAK) {
  initialize({PathToLlFiles + "binop_uninit_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // binop_uninit uses uninitialized variable i in 'int j = i + 10;'
  GroundTruthTy GroundTruth;

  // %4 = load i32, i32* %2, ID: 6 ;  %2 is the uninitialized variable i
  // %5 = add nsw i32 %4, 10 ;        %4 is undef, since it is loaded from
  // undefined alloca; not sure if it is necessary to report again
  const auto Entry = LineColFun{2, 0, "main"};
  const auto EntryTwo = LineColFun{3, 11, "main"};
  const auto EntryThree = LineColFun{3, 13, "main"};
  GroundTruth.insert({EntryTwo, {Entry}});
  GroundTruth.insert({EntryThree, {EntryTwo}});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_03_SHOULD_LEAK) {
  initialize({PathToLlFiles + "callnoret_c_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // callnoret uses uninitialized variable a in 'return a + 10;' of addTen(int)
  GroundTruthTy GroundTruth;

  const auto IntA = LineColFun{7, 7, "main"};
  const auto CopyA = LineColFun{9, 10, "main"};
  const auto ArgA = LineColFun{1, 16, "addTen"};
  const auto LoadA = LineColFun{3, 10, "addTen"};
  const auto Add = LineColFun{3, 12, "addTen"};
  GroundTruth.insert({CopyA, {IntA}});
  GroundTruth.insert({Add, {LoadA}});
  GroundTruth.insert({LoadA, {ArgA}});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_04_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "ctor_default_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // ctor.cpp does not contain undef-uses
  GroundTruthTy GroundTruth;

  compareResults(GroundTruth);
}
TEST_F(IFDSUninitializedVariablesTest, UninitTest_05_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "struct_member_init_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // struct_member_init.cpp does not contain undef-uses
  GroundTruthTy GroundTruth;

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_06_SHOULD_NOT_LEAK) {
  initialize({PathToLlFiles + "struct_member_uninit_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  // struct_member_uninit.cpp does not contain undef-uses
  GroundTruthTy GroundTruth;

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
  GroundTruthTy GroundTruth;

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

  GroundTruthTy GroundTruth;

  const auto IntI = LineColFun{2, 7, "_Z3foov"};
  const auto UseOfI = LineColFun{3, 10, "_Z3foov"};

  GroundTruth.insert({UseOfI, {IntI}});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_11_SHOULD_NOT_LEAK) {

  initialize({PathToLlFiles + "sanitizer_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  GroundTruthTy GroundTruth;
  // all undef-uses are sanitized;
  // However, the uninitialized variable j is read, which causes the analysis to
  // report an undef-use
  // 6 => {2}

  const auto IntI = LineColFun{3, 7, "main"};
  const auto UseOfI = LineColFun{4, 7, "main"};

  GroundTruth.insert({UseOfI, {IntI}});

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
  GroundTruthTy GroundTruth;

  const auto IntJ = LineColFun{3, 7, "main"};
  const auto LoadJ = LineColFun{5, 7, "main"};

  GroundTruth.insert({LoadJ, {IntJ}});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_14_SHOULD_LEAK) {

  initialize({PathToLlFiles + "uninit_c_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  GroundTruthTy GroundTruth;

  const auto IntA = LineColFun{2, 7, "main"};
  const auto IntB = LineColFun{3, 7, "main"};
  const auto LoadA = LineColFun{6, 11, "main"};
  const auto Multiply = LineColFun{6, 13, "main"};
  const auto LoadB = LineColFun{6, 15, "main"};

  GroundTruth.insert({LoadA, {IntA}});
  GroundTruth.insert({LoadB, {IntB}});
  GroundTruth.insert({Multiply, {LoadA, LoadB}});

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

  GroundTruthTy GroundTruth;

  const auto ArgX = LineColFun{1, 18, "_Z8functionii"};
  const auto IntI = LineColFun{2, 7, "_Z8functionii"};
  const auto LoadX = LineColFun{3, 11, "_Z8functionii"};
  const auto LoadI = LineColFun{5, 10, "_Z8functionii"};
  const auto Add = LineColFun{5, 12, "_Z8functionii"};
  const auto IntJ = LineColFun{10, 7, "main"};
  const auto LoadJ = LineColFun{12, 16, "main"};

  // TODO remove GroundTruth.insert({LoadX, ArgX}) below
  GroundTruth.insert({LoadX, {ArgX}});
  GroundTruth.insert({LoadI, {IntI}});
  GroundTruth.insert({Add, {LoadI}});
  GroundTruth.insert({LoadJ, {IntJ}});
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

  GroundTruthTy GroundTruth;

  // Leaks due to field-insensitivity

  // %1 = load ptr, ptr %x.addr, align 8, !dbg !240, !psr.id !241 | ID: 11
  LineColFun LoadX{4, 12, "_Z3fooRii"};
  // %x.addr = alloca ptr, align 8
  LineColFun ArgAddrX{2, 15, "_Z3fooRii"};
  GroundTruth.insert({LoadX, {ArgAddrX}});
  // %2 = load ptr, ptr %x.addr
  LineColFun LoadXTwo{5, 14, "_Z3fooRii"};
  GroundTruth.insert({LoadXTwo, {ArgAddrX}});

  // Load uninitialized variable

  // %1 = load i32, ptr %j, align 4, !dbg !274, !psr.id !275 | ID: 31
  LineColFun LoadJ{11, 18, "main"};
  // %j = alloca i32, align 4
  LineColFun IntJ{10, 7, "main"};
  GroundTruth.insert({LoadJ, {IntJ}});

  // Load recursive return-value for returning it

  // %4 = load ptr, ptr %retval
  LineColFun FooExit{6, 1, "_Z3fooRii"};
  GroundTruth.insert({FooExit, {OperandOf{0, FooExit}}});
  // Load return-value of foo in main
  // %0 = load i32, ptr %call, align 4
  LineColFunOp Load0{10, 11, "main", llvm::Instruction::Load};
  // %call = call noundef nonnull align 4 dereferenceable(4) ptr @_Z3fooRii(ptr
  // noundef nonnull align 4 dereferenceable(4) %i, i32 noundef 10)
  LineColFunOp CallFooRec{10, 11, "main", llvm::Instruction::Call};
  GroundTruth.insert({Load0, {CallFooRec}});
  // // Load return-value of foo in main
  // GroundTruth.insert({FooExit, IntJ});

  // ***********
  // Load recursive return-value for returning it
  // GroundTruth.insert({RetOfFoo, IntJ});
  // ***********

  // Load return-value of foo in main
  // GroundTruth.insert({IntJ, RetOfFoo});

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_21_SHOULD_LEAK) {

  initialize({PathToLlFiles + "virtual_call_cpp_dbg.ll"});
  IFDSSolver Solver(*UninitProblem, &HA->getICFG());
  Solver.solve();

  GroundTruthTy GroundTruth;
  // %x.addr = alloca ptr, align 8
  const auto FooXAddr = LineColFun{3, 15, "_Z3fooRi"};
  // %0 = load ptr, ptr %x.addr, align 8
  const auto FooXLoad = LineColFun{3, 27, "_Z3fooRi"};
  // %x.addr = alloca ptr, align 8
  const auto BarXAddr = LineColFun{4, 15, "_Z3barRi"};
  // %0 = load ptr, ptr %x.addr, align 8
  const auto Load = LineColFun{5, 3, "_Z3barRi"};
  // %1 = load ptr, ptr %x.addr, align 8
  const auto LoadX = LineColFun{6, 10, "_Z3barRi"};

  // %j = alloca i32, align 4
  const auto IntJ = LineColFun{16, 7, "main"};
  // %call = call noundef nonnull align 4 dereferenceable(4) ptr %1
  const auto BazCall = LineColFunOp{16, 11, "main", llvm::Instruction::Call};

  // %2 = load i32, ptr %call, align 4
  const auto SecondLoadInIfEnd = OperandOf{
      // The value operand of the store
      1 - llvm::StoreInst::getPointerOperandIndex(),
      LineColFunOp{16, 7, "main", llvm::Instruction::Store},
  };
  // %3 = load i32, ptr %j, align 4
  const auto LoadJ = LineColFun{17, 10, "main"};
  // is passed as a reference, so I isn't being loaded here
  // const auto LoadI =
  //     LineColFun{16, 15, "main"};

  // 3  => {0}; due to field-insensitivity
  GroundTruth.insert({FooXLoad, {FooXAddr}});
  // 8  => {5}; due to field-insensitivity
  GroundTruth.insert({Load, {BarXAddr}});
  // 10 => {5}; due to alias-unawareness
  GroundTruth.insert({LoadX, {BarXAddr}});
  // 35 => {34}; actual leak
  GroundTruth.insert({SecondLoadInIfEnd, {BazCall}});
  // 37 => {17}; actual leak
  GroundTruth.insert({LoadJ, {IntJ}});

  compareResults(GroundTruth);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
