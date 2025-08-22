
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
#include "phasar/Utils/DebugOutput.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

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
      std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
          &GroundTruthEntries) {
    for (const auto &Entry : GroundTruthEntries) {
      const auto *GTInst = std::get<0>(Entry);
      llvm::outs() << "GTInst: " << GTInst << "\n";
      llvm::outs() << "*GTInst: " << *GTInst << "\n";

      const auto *GTValue = std::get<1>(Entry);
      llvm::outs() << "GTValue: " << GTValue << "\n";
      llvm::outs() << "*GTValue: " << *GTValue << "\n\n";
    }

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

    EXPECT_EQ(FoundUninitUses, GroundTruthEntries)
        << "Expected: " << PrettyPrinter{GroundTruthEntries}
        << "; got: " << PrettyPrinter{FoundUninitUses};
  }

  void compareResults(
      const std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>>
          &GroundTruth) {
    auto GroundTruthEntries = getGroundTruthInsts(GroundTruth, true);
    compareResults(GroundTruthEntries);
  }

  void compareResults(
      const std::set<std::tuple<TestingSrcLocation, TestingSrcLocation>>
          &GroundTruth,
      const LLVMProjectIRDB &IRDB) {
    std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
        GroundTruthEntries;

    for (const auto &Entry : GroundTruth) {
      const auto *First = testingLocInIR(std::get<0>(Entry), IRDB);
      const auto *FirstAsInst = llvm::dyn_cast<const llvm::Instruction>(First);
      const auto *Second = testingLocInIR(std::get<1>(Entry), IRDB);
      GroundTruthEntries.insert({FirstAsInst, Second});
    }

    compareResults(GroundTruthEntries);
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

  std::set<std::tuple<TestingSrcLocation, TestingSrcLocation>> GroundTruth;

  const auto IntI =
      SrcCodeLocationEntry(9, 7, HA->getICFG().getFunction("main"));

  // Leaks due to field-insensitivity

  // %1 = load ptr, ptr %x.addr, align 8, !dbg !240, !psr.id !241 | ID: 11
  LineColFun LoadX{4, 12, "_Z3fooRii"};
  // %x.addr = alloca ptr, align 8
  LineColFun ArgAddrX{2, 15, "_Z3fooRii"};
  GroundTruth.insert({LoadX, ArgAddrX});
  // %2 = load ptr, ptr %x.addr
  LineColFun LoadXTwo{5, 14, "_Z3fooRii"};
  GroundTruth.insert({LoadXTwo, ArgAddrX});

  // Load uninitialized variable

  // %1 = load i32, ptr %j, align 4, !dbg !274, !psr.id !275 | ID: 31
  LineColFun LoadJ{11, 18, "main"};
  // %j = alloca i32, align 4
  LineColFun IntJ{10, 7, "main"};
  GroundTruth.insert({LoadJ, IntJ});

  // Load recursive return-value for returning it

  // %4 = load ptr, ptr %retval
  LineColFun FooExit{6, 1, "_Z3fooRii"};
  GroundTruth.insert({FooExit, OperandOf{0, FooExit}});
  // Load return-value of foo in main
  // %0 = load i32, ptr %call, align 4
  LineColFunLambda Load0{10, 11, "main", [](const llvm::Instruction *Inst) {
                           return llvm::isa<llvm::LoadInst>(Inst);
                         }};
  // %call = call noundef nonnull align 4 dereferenceable(4) ptr @_Z3fooRii(ptr
  // noundef nonnull align 4 dereferenceable(4) %i, i32 noundef 10)
  LineColFunLambda CallFooRec{10, 11, "main",
                              [](const llvm::Instruction *Inst) {
                                return llvm::isa<llvm::CallInst>(Inst);
                              }};
  GroundTruth.insert({Load0, CallFooRec});
  // // Load return-value of foo in main
  // GroundTruth.insert({FooExit, IntJ});

  // ***********
  // Load recursive return-value for returning it
  // GroundTruth.insert({RetOfFoo, IntJ});
  // ***********

  // Load return-value of foo in main
  // GroundTruth.insert({IntJ, RetOfFoo});

  compareResults(GroundTruth, *HA->getICFG().getIRDB());

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
  // %x.addr = alloca ptr, align 8
  const auto FooXAddr =
      SrcCodeLocationEntry(3, 15, HA->getICFG().getFunction("_Z3fooRi"));
  // %0 = load ptr, ptr %x.addr, align 8
  const auto FooXLoad =
      SrcCodeLocationEntry(3, 27, HA->getICFG().getFunction("_Z3fooRi"));
  // %x.addr = alloca ptr, align 8
  const auto BarXAddr =
      SrcCodeLocationEntry(4, 15, HA->getICFG().getFunction("_Z3barRi"));
  // %0 = load ptr, ptr %x.addr, align 8
  const auto Load =
      SrcCodeLocationEntry(5, 3, HA->getICFG().getFunction("_Z3barRi"));
  // %1 = load ptr, ptr %x.addr, align 8
  const auto LoadX =
      SrcCodeLocationEntry(6, 10, HA->getICFG().getFunction("_Z3barRi"));
  //
  const auto IntI =
      SrcCodeLocationEntry(9, 7, HA->getICFG().getFunction("main"));
  // %j = alloca i32, align 4
  const auto IntJ =
      SrcCodeLocationEntry(16, 7, HA->getICFG().getFunction("main"));
  // %call = call noundef nonnull align 4 dereferenceable(4) ptr %1
  const auto BazCall =
      SrcCodeLocationEntry(16, 11, HA->getICFG().getFunction("main"),
                           [](const llvm::Instruction *Inst) {
                             return llvm::isa<llvm::CallInst>(Inst);
                           });
  // This implementation to get %2 is bad, but it works
  bool SkippedFirst = false;
  // %2 = load i32, ptr %call, align 4
  const auto SecondLoadInIfEnd = SrcCodeLocationEntry(
      16, 11, HA->getICFG().getFunction("main"),
      [&SkippedFirst](const llvm::Instruction *Inst) mutable {
        if (llvm::isa<llvm::LoadInst>(Inst)) {
          if (!SkippedFirst) {
            SkippedFirst = true;
            return false;
          }
          return true;
        }

        return false;
      });
  // %3 = load i32, ptr %j, align 4
  const auto LoadJ =
      SrcCodeLocationEntry(17, 10, HA->getICFG().getFunction("main"));
  // is passed as a reference, so I isn't being loaded here
  // const auto LoadI =
  //     SrcCodeLocationEntry(16, 15, HA->getICFG().getFunction("main"));

  // TODO: remove this comment: This GT is correct!
  // 3  => {0}; due to field-insensitivity
  GroundTruth.insert({FooXLoad, FooXAddr});

  // TODO: remove this comment: This GT is correct!
  // 8  => {5}; due to field-insensitivity
  GroundTruth.insert({Load, BarXAddr});

  // TODO: remove this comment: This GT is correct!
  // 10 => {5}; due to alias-unawareness
  GroundTruth.insert({LoadX, BarXAddr});

  // TODO: remove this comment: GT is wrong.
  // 35 => {34}; actual leak
  GroundTruth.insert({SecondLoadInIfEnd, BazCall});
  // 37 => {17}; actual leak
  GroundTruth.insert({LoadJ, IntJ});

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
