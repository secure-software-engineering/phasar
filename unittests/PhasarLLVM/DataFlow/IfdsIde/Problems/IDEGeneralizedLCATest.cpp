/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/SrcCodeLocationEntry.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

#include "SourceMapping.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <vector>

using namespace psr;
using namespace psr::glca;

using groundTruth_t = std::tuple<const IDEGeneralizedLCA::l_t,
                                 SrcCodeLocationEntry, SrcCodeLocationEntry>;

/* ============== TEST FIXTURE ============== */

class IDEGeneralizedLCATest : public ::testing::Test {

protected:
  static constexpr auto PathToLLFiles =
      PHASAR_BUILD_SUBFOLDER("general_linear_constant/");

  std::optional<HelperAnalyses> HA;
  std::optional<IDEGeneralizedLCA> LCAProblem;
  std::unique_ptr<IDESolver<IDEGeneralizedLCADomain>> LCASolver;

  static constexpr size_t MaxSetSize = 2;

  IDEGeneralizedLCATest() = default;

  void initialize(llvm::StringRef LLFile, size_t MaxSetSize = 2) {
    using namespace std::literals;
    HA.emplace(PathToLLFiles + LLFile, std::vector{"main"s});
    LCAProblem = createAnalysisProblem<IDEGeneralizedLCA>(
        *HA, std::vector{"main"s}, MaxSetSize);
    LCASolver = std::make_unique<IDESolver<IDEGeneralizedLCADomain>>(
        *LCAProblem, &HA->getICFG());

    LCASolver->solve();
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  //  compare results
  /// \brief compares the computed results with every given tuple (value,
  /// alloca, inst)
  void compareResults(std::vector<groundTruth_t> &Expected) {
    for (const auto &Entry : Expected) {
      const auto &EVal = std::get<0>(Entry);
      const auto &SCLVr = std::get<1>(Entry);
      const auto &SCLInst = std::get<2>(Entry);

      const auto *Vr = getInstLambdaOrNot(SCLVr);
      const auto *Inst = getInstLambdaOrNot(SCLInst);

      bool Flag = false;

      if (Vr) {
        llvm::outs() << "VrId Inst:   " << *Vr << "\n";
      } else {
        llvm::outs() << "VrId is nullptr\n";
        Flag = true;
      }

      if (Inst) {
        llvm::outs() << "InstID Inst: " << *Inst << "\n";
      } else {
        llvm::outs() << "Inst is nullptr\n";
        Flag = true;
      }

      if (Flag) {
        EXPECT_TRUE(false);
        continue;
      }

      llvm::outs() << "Result: " << LCASolver->resultAt(Inst, Vr) << "\n";

      ASSERT_NE(nullptr, Vr);
      ASSERT_NE(nullptr, Inst);

      auto Result = LCASolver->resultAt(Inst, Vr);
      EXPECT_EQ(EVal, Result)
          << "vr:" << Vr->getValueID() << " inst:" << Inst->getValueID()
          << " Expected: " << EVal << " Got:" << Result;
    }

    /*for (const auto &[EVal, VrId, InstId] : Expected) {
      const auto *Vr = HA->getProjectIRDB().getInstruction(VrId);
      const auto *Inst = HA->getProjectIRDB().getInstruction(InstId);
      llvm::outs() << "VrId Inst:   " << *Vr << "\n";
      llvm::outs() << "InstID Inst: " << *Inst << "\n";
      ASSERT_NE(nullptr, Vr);
      ASSERT_NE(nullptr, Inst);
      auto Result = LCASolver->resultAt(Inst, Vr);

      EXPECT_EQ(EVal, Result) << "vr:" << VrId << " inst:" << InstId
                              << " Expected: " << EVal << " Got:" << Result;
    }*/
  }

private:
  const llvm::Instruction *
  getInstLambdaOrNot(const SrcCodeLocationEntry &Entry) {
    if (Entry.LambdaFunc) {
      return unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(Entry.Context), Entry.Line,
          Entry.Column, Entry.LambdaFunc);
    }

    return unittest::getInstAtOrNull(
        std::get<const llvm::Function *>(Entry.Context), Entry.Line,
        Entry.Column);
  }
}; // class Fixture

TEST_F(IDEGeneralizedLCATest, SimpleTest) {
  initialize("SimpleTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(10)},
       SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(15)},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}
TEST_F(IDEGeneralizedLCATest, BranchTest) {
  initialize("BranchTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(25)},
       SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::AddOperator>(Inst);
                            }),
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(24)},
       SrcCodeLocationEntry(3, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FPtest) {
  initialize("FPtest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(4.5)},
       SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs()
                                  << "4.5: In lambda *Inst: " << *Inst << "\n";
                              llvm::outs() << "4.5: In lambda *Inst.getType(): "
                                           << *Inst->getType() << "\n";
                              llvm::outs()
                                  << "4.5: In lambda *Inst.getOpcode(): "
                                  << Inst->getOpcode() << "\n";
                              /* Floating-point types are handled by FAdd, FSub
                                 and FMul, instead of Add, Sub and Mul, which
                                 are used by Integer types.
                                 Opcode 18 is operator FMul */
                              // TODO: ask Fabian if the opcode is a valid
                              // approach to this issue.
                              return Inst->getOpcode() == 18;
                            }),
       SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(2.0)},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs()
                                  << "2.0: In lambda *Inst: " << *Inst << "\n";
                              llvm::outs() << "2.0: In lambda *Inst.getType(): "
                                           << *Inst->getType() << "\n";
                              llvm::outs()
                                  << "2.0: In lambda *Inst.getOpcode(): "
                                  << Inst->getOpcode() << "\n";
                              /* Floating-point types are handled by FAdd, FSub
                                 and FMul, instead of Add, Sub and Mul, which
                                 are used by Integer types.
                                 Opcode 16 is operator FSub */
                              // TODO: ask Fabian if the opcode is a valid
                              // approach to this issue.
                              return Inst->getOpcode() == 16;
                            }),
       SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTest) {
  initialize("StringTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("Hello, World")},
       SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue("Hello, World")},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringBranchTest) {
  initialize("StringBranchTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("Hello Hello"), EdgeValue("Hello, World")},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(11, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  // TODO: ask Fabian how to reach:
  // %1 = load ptr, ptr %str1, align 8, !dbg !40
  // in the if.end branch
  GroundTruth.push_back(
      {{EdgeValue("Hello, World")},
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) mutable {
                              return llvm::isa<llvm::StoreInst>(Inst);
                            }),
       SrcCodeLocationEntry(11, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::LoadInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTestCpp) {
  initialize("StringTest_cpp_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  // TODO: ask Fabian, why ReturnInst doesn't work here and how to fix that
  // test.
  GroundTruth.push_back(
      {{EdgeValue("Hello, World")},
       SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst" << *Inst << "\n";
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FloatDivisionTest) {
  initialize("FloatDivision_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(1.0)},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs()
                                  << "In lambda *Inst: " << *Inst << "\n";
                              llvm::outs() << "2.0: In lambda *Inst.getType(): "
                                           << *Inst->getType() << "\n";
                              llvm::outs() << "n lambda *Inst.getOpcode(): "
                                           << Inst->getOpcode() << "\n";
                              /* Floating-point types are handled by FAdd, FSub
                                 and FMul, instead of Add, Sub and Mul, which
                                 are used by Integer types.
                                 Opcode 18 is operator fptosi */
                              // TODO: ask Fabian if the opcode is a valid
                              // approach to this issue.
                              return Inst->getOpcode() == 45;
                            }),
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst" << *Inst << "\n";
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(nullptr)},
       SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs()
                                  << "In lambda *Inst: " << *Inst << "\n";
                              llvm::outs() << "In lambda *Inst.getType(): "
                                           << *Inst->getType() << "\n";
                              llvm::outs() << "In lambda *Inst.getOpcode(): "
                                           << Inst->getOpcode() << "\n";
                              /* Floating-point types are handled by FAdd, FSub
                                 and FMul, instead of Add, Sub and Mul, which
                                 are used by Integer types.
                                 Opcode 18 is operator FMul */
                              // TODO: ask Fabian if the opcode is a valid
                              // approach to this issue.
                              return Inst->getOpcode() == 18;
                            }),
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(-7.0)},
       SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs()
                                  << "2.0: In lambda *Inst: " << *Inst << "\n";
                              llvm::outs() << "2.0: In lambda *Inst.getType(): "
                                           << *Inst->getType() << "\n";
                              llvm::outs()
                                  << "2.0: In lambda *Inst.getOpcode(): "
                                  << Inst->getOpcode() << "\n";
                              /* Floating-point types are handled by FAdd, FSub
                                 and FMul, instead of Add, Sub and Mul, which
                                 are used by Integer types.
                                 Opcode 16 is operator FSub */
                              // TODO: ask Fabian if the opcode is a valid
                              // approach to this issue.
                              return Inst->getOpcode() == 16;
                            }),
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst" << *Inst << "\n";
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, SimpleFunctionTest) {
  initialize("SimpleFunctionTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  // TODO: ask Fabian if this impl is fine, or if there is a better way to do
  // it.
  /* We want the second call inst here:
    not this -> %call2 = call i32 @foo(i32 noundef %2), !dbg !42, !psr.id !43
    this -----> %call3 = call i32 @bar(i32 noundef %call2), !dbg !44, !psr.id
    !45
  */
  bool FoundFirstCall = false;
  GroundTruth.push_back(
      {{EdgeValue(48)},
       SrcCodeLocationEntry(
           8, 0, HA->getProjectIRDB().getFunction("main"),
           [&FoundFirstCall](const llvm::Instruction *Inst) mutable {
             llvm::outs() << "*Inst: " << *Inst << "\n";
             if (llvm::isa<llvm::CallInst>(Inst) && !FoundFirstCall) {
               FoundFirstCall = true;
               return false;
             }
             return llvm::isa<llvm::CallInst>(Inst);
           }),
       SrcCodeLocationEntry(10, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(nullptr)},
       SrcCodeLocationEntry(9, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst: " << *Inst << "\n";
                              return llvm::isa<llvm::AddOperator>(Inst);
                            }),
       SrcCodeLocationEntry(10, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, GlobalVariableTest) {
  initialize("GlobalVariableTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(50)},
       SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst: " << *Inst << "\n";
                              return llvm::isa<llvm::AddOperator>(Inst);
                            }),
       SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(8)},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst: " << *Inst << "\n";
                              return llvm::isa<llvm::AddOperator>(Inst);
                            }),
       SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, Imprecision) {
  initialize("Imprecision_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  // TODO: ask Fabian how to handle this test. I do not know how to handle that
  // source file. I tried the calls in main, line 3 with function foo and line 1
  // with function bar.

  GroundTruth.push_back(
      {{EdgeValue(1), EdgeValue(2)},
       SrcCodeLocationEntry(3, 0, HA->getProjectIRDB().getFunction("foo"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst: " << *Inst << "\n";
                              // return false;
                              return llvm::isa<llvm::StoreInst>(Inst);
                            }),
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});
  GroundTruth.push_back(
      {{EdgeValue(2), EdgeValue(3)},
       SrcCodeLocationEntry(3, 0, HA->getProjectIRDB().getFunction("foo"),
                            [](const llvm::Instruction *Inst) {
                              llvm::outs() << "*Inst: " << *Inst << "\n";
                              return false;
                              // return llvm::isa<llvm::LoadInst>(Inst);
                            }),
       SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, ReturnConstTest) {
  initialize("ReturnConstTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(43)},
       SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::AddOperator>(Inst);
                            }),
       SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, NullTest) {
  initialize("NullTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("")},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main"),
                            [](const llvm::Instruction *Inst) {
                              return llvm::isa<llvm::ReturnInst>(Inst);
                            })});

  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
