#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

namespace psr {
using MapTy = llvm::DenseMap<const llvm::Function *,
                             llvm::SmallPtrSet<const llvm::Instruction *, 2>>;

class LLVMBasedICFGExportTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = unittest::PathToLLTestFiles;
  static constexpr auto PathToJSONFiles = unittest::PathToJSONTestFiles;

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  nlohmann::json exportICFG(const std::string &TestFile,
                            bool AsSrcCode = false) {
    LLVMProjectIRDB IRDB(PathToLLFiles + TestFile);
    DIBasedTypeHierarchy TH(IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH,
                       nullptr, Soundness::Soundy, /*IncludeGlobals*/ false);

    auto Ret = ICFG.exportICFGAsJson(AsSrcCode);

    // llvm::errs() << "Result: " << Ret.dump(4) << '\n';

    return Ret;
  }

  nlohmann::json exportCFGFor(const std::string &TestFile,
                              const std::string &FunctionName,
                              bool AsSrcCode = false) {
    LLVMProjectIRDB IRDB(PathToLLFiles + TestFile);
    LLVMBasedCFG CFG;

    const auto *F = IRDB.getFunction(FunctionName);
    assert(F != nullptr && "Invalid function");
    // ASSERT_NE(nullptr, F);

    auto Ret =
        AsSrcCode ? CFG.exportCFGAsSourceCodeJson(F) : CFG.exportCFGAsJson(F);

    // llvm::errs() << "Result: " << Ret.dump(4) << '\n';

    return Ret;
  }

  MapTy getAllRetSites(const LLVMBasedICFG &ICFG) {
    MapTy RetSitesOf;

    for (const auto *F : ICFG.getAllVertexFunctions()) {
      for (const auto &Inst : llvm::instructions(F)) {
        if (llvm::isa<llvm::CallBase>(&Inst)) {
          for (const auto *Callee : ICFG.getCalleesOfCallAt(&Inst)) {
            for (const auto *Succ : ICFG.getSuccsOf(&Inst)) {
              RetSitesOf[Callee].insert(Succ);
            }
          }
        }
      }
    }

    return RetSitesOf;
  }

  void verifyIRJson(const nlohmann::json &ExportedICFG,
                    const LLVMBasedICFG &GroundTruth,
                    bool WithDebugOutput = false) {
    auto RetSitesOf = getAllRetSites(GroundTruth);

    auto expectEdge // NOLINT
        = [&](const std::string &From, const std::string &To) {
            EXPECT_TRUE(std::any_of(
                ExportedICFG.begin(), ExportedICFG.end(),
                [&](auto &&Elem) {
                  return Elem == nlohmann::json{{"from", From}, {"to", To}};
                }))
                << "No edge from " << From << " to " << To << " in "
                << ExportedICFG.dump(4);
          };

    auto print // NOLINT
        = [WithDebugOutput](auto &&...Args) {
            if (WithDebugOutput) {
              ((llvm::errs() << Args), ...);
              llvm::errs() << "\n";
            }
          };

    for (const auto *F : GroundTruth.getAllVertexFunctions()) {
      for (const auto &Inst : llvm::instructions(F)) {
        auto InstStr = llvmIRToStableString(&Inst);
        if (llvm::isa<llvm::CallBase>(&Inst)) {
          for (const auto *Callee : GroundTruth.getCalleesOfCallAt(&Inst)) {
            if (!Callee->isDeclaration()) {
              if (WithDebugOutput) {
                print("Callee: ", *Callee);
              }

              expectEdge(InstStr,
                         llvmIRToStableString(&Callee->front().front()));
              if (WithDebugOutput) {
                print("> end");
              }
            }
          }
        } else if (llvm::isa<llvm::ReturnInst>(&Inst) ||
                   llvm::isa<llvm::ResumeInst>(&Inst)) {
          const auto &RetSites = RetSitesOf[Inst.getFunction()];

          for (const auto *Ret : RetSites) {
            expectEdge(InstStr, llvmIRToStableString(Ret));
          }
        } else {
          for (const auto *Succ : GroundTruth.getSuccsOf(&Inst)) {
            expectEdge(InstStr, llvmIRToStableString(Succ));
          }
        }
      }
    }
  }

  void verifySourceCodeJSON(const nlohmann::json &ExportedICFG,
                            const nlohmann::json &GroundTruth) {
    ASSERT_TRUE(ExportedICFG.is_array());

    EXPECT_EQ(GroundTruth.size(), ExportedICFG.size());
    bool HasError = false;
    for (const auto &GTJson : GroundTruth) {
      auto GTFrom = GTJson.at("from").get<SourceCodeInfo>();
      auto GTTo = GTJson.at("to").get<SourceCodeInfo>();

      bool HasMatch = std::any_of(ExportedICFG.begin(), ExportedICFG.end(),
                                  [&](const nlohmann::json &ExportedInfoJson) {
                                    return ExportedInfoJson.at("from")
                                               .get<SourceCodeInfo>()
                                               .equivalentWith(GTFrom) &&
                                           ExportedInfoJson.at("to")
                                               .get<SourceCodeInfo>()
                                               .equivalentWith(GTTo);
                                  });

      EXPECT_TRUE(HasMatch) << "No exported equivalent to " << GTJson.dump(4);
      HasError |= !HasMatch;
    }

    if (HasError) {
      llvm::errs() << "Errorneous json: " << ExportedICFG.dump(4) << '\n';
    }
  }

  nlohmann::json readJson(const llvm::Twine &JsonFilename) {
    return psr::readJsonFile(PathToJSONFiles + JsonFilename);
  }

  void verifyExportICFG(const llvm::Twine &TestFile,
                        bool WithDebugOutput = false) {
    LLVMProjectIRDB IRDB(PathToLLFiles + TestFile);
    DIBasedTypeHierarchy TH(IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH,
                       nullptr, Soundness::Soundy, /*IncludeGlobals*/ false);

    verifyIRJson(ICFG.exportICFGAsJson(/*WithSourceCodeInfo*/ false), ICFG,
                 WithDebugOutput);

    if (WithDebugOutput) {
      struct AAWriter : public llvm::AssemblyAnnotationWriter {
        /// emitInstructionAnnot - This may be implemented to emit a string
        /// right before an instruction is emitted.
        void emitInstructionAnnot(const llvm::Instruction *Inst,
                                  llvm::formatted_raw_ostream &OS) override {
          OS << "  ; | Id: " << getMetaDataID(Inst) << '\n';
        }
      } AW{};
      IRDB.getModule()->print(llvm::errs(), &AW);
      // llvm::errs() << "ModuleRef: " << *IRDB.getWPAModule() << "\n";
      llvm::errs()
          << ICFG.exportICFGAsJson(/*WithSourceCodeInfo*/ false).dump(4)
          << '\n';
    }
  }
};

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR01) {
  verifyExportICFG("call_graphs/static_callsite_1_c.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR02) {
  verifyExportICFG("call_graphs/static_callsite_2_c.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR03) {
  verifyExportICFG("call_graphs/static_callsite_3_c.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR04) {
  verifyExportICFG("call_graphs/static_callsite_4_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR05) {
  verifyExportICFG("call_graphs/static_callsite_5_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR06) {
  verifyExportICFG("call_graphs/static_callsite_6_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR07) {
  verifyExportICFG("call_graphs/static_callsite_7_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR08) {
  verifyExportICFG("call_graphs/static_callsite_8_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR09) {
  verifyExportICFG("call_graphs/static_callsite_9_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR10) {
  verifyExportICFG("call_graphs/static_callsite_10_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR11) {
  verifyExportICFG("call_graphs/static_callsite_11_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR12) {
  verifyExportICFG("call_graphs/static_callsite_12_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIR13) {
  verifyExportICFG("call_graphs/static_callsite_13_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV1) {
  verifyExportICFG("call_graphs/virtual_call_1_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV2) {
  verifyExportICFG("call_graphs/virtual_call_2_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV3) {
  verifyExportICFG("call_graphs/virtual_call_3_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV4) {
  verifyExportICFG("call_graphs/virtual_call_4_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV5) {
  verifyExportICFG("call_graphs/virtual_call_5_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV6) {
  verifyExportICFG("call_graphs/virtual_call_6_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV7) {
  verifyExportICFG("call_graphs/virtual_call_7_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV8) {
  verifyExportICFG("call_graphs/virtual_call_8_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGIRV9) {
  verifyExportICFG("call_graphs/virtual_call_9_cpp.ll");
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGSource01) {
  auto Results =
      exportICFG("linear_constant/call_01_cpp_dbg.ll", /*asSrcCode*/ true);

  verifySourceCodeJSON(Results,
                       readJson("linear_constant/call_01_cpp_icfg.json"));
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGSource02) {
  auto Results =
      exportICFG("linear_constant/call_07_cpp_dbg.ll", /*asSrcCode*/ true);
  // llvm::errs() << Results.dump(4) << '\n';
  verifySourceCodeJSON(Results,
                       readJson("linear_constant/call_07_cpp_icfg.json"));
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGSource03) {
  auto Results =
      exportICFG("exceptions/exceptions_01_cpp_dbg.ll", /*asSrcCode*/ true);
  llvm::errs() << Results.dump(4) << '\n';
  verifySourceCodeJSON(Results,
                       readJson("exceptions/exceptions_01_cpp_icfg.json"));
}

TEST_F(LLVMBasedICFGExportTest, ExportCFG01) {
  auto Results = exportCFGFor("linear_constant/branch_07_cpp_dbg.ll", "main",
                              /*asSrcCode*/ true);
  // llvm::errs() << Results.dump(4) << '\n';
  verifySourceCodeJSON(Results,
                       readJson("linear_constant/branch_07_cpp_main_cfg.json"));
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
