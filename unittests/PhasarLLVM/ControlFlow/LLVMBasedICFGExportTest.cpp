#include "gtest/gtest.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarPass/Options.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

namespace psr {
using MapTy = llvm::DenseMap<const llvm::Function *,
                             llvm::SmallPtrSet<const llvm::Instruction *, 2>>;

class LLVMBasedICFGExportTest : public ::testing::Test {
protected:
  const std::string &PathToLLFiles = unittest::PathToLLTestFiles;
  const std::string &PathToJSONFiles = unittest::PathToJSONTestFiles;

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  nlohmann::json exportICFG(const std::string &TestFile,
                            bool AsSrcCode = false) {
    ProjectIRDB IRDB({PathToLLFiles + TestFile}, IRDBOptions::WPA);
    LLVMTypeHierarchy TH(IRDB);
    LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH);

    auto Ret =
        AsSrcCode ? ICFG.exportICFGAsSourceCodeJson() : ICFG.exportICFGAsJson();

    // llvm::errs() << "Result: " << Ret.dump(4) << '\n';

    return Ret;
  }

  nlohmann::json exportCFGFor(const std::string &TestFile,
                              const std::string &FunctionName,
                              bool AsSrcCode = false) {
    ProjectIRDB IRDB({PathToLLFiles + TestFile}, IRDBOptions::WPA);
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

    for (const auto *F : ICFG.getAllFunctions()) {
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

    for (const auto *F : GroundTruth.getAllFunctions()) {
      for (const auto &Inst : llvm::instructions(F)) {
        auto InstStr = llvmIRToStableString(&Inst);
        if (llvm::isa<llvm::CallBase>(&Inst)) {
          for (const auto *Callee : GroundTruth.getCalleesOfCallAt(&Inst)) {
            if (!Callee->isDeclaration()) {
              print("Callee: ", *Callee);
              expectEdge(InstStr,
                         llvmIRToStableString(&Callee->front().front()));

              print("> end");
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

    for (const auto &GTJson : GroundTruth) {
      auto GTFrom = GTJson.at("from").get<SourceCodeInfo>();
      auto GTTo = GTJson.at("to").get<SourceCodeInfo>();
      EXPECT_TRUE(std::any_of(ExportedICFG.begin(), ExportedICFG.end(),
                              [&](const nlohmann::json &ExportedInfoJson) {
                                return ExportedInfoJson.at("from")
                                           .get<SourceCodeInfo>()
                                           .equivalentWith(GTFrom) &&
                                       ExportedInfoJson.at("to")
                                           .get<SourceCodeInfo>()
                                           .equivalentWith(GTTo);
                              }))
          << "No exported equivalent to " << GTJson.dump(4);
    }
  }

  nlohmann::json readJson(const std::string &JsonFilename) {
    std::ifstream IfIn(PathToJSONFiles + JsonFilename);
    assert(IfIn.good() && "Invalid JSON file");

    nlohmann::json J;
    IfIn >> J;

    assert(IfIn.good() && "Error reading JSON file");
    return J;
  }

  void verifyExportICFG(const std::string &TestFile,
                        bool WithDebugOutput = false) {
    ProjectIRDB IRDB({PathToLLFiles + TestFile}, IRDBOptions::WPA);
    LLVMTypeHierarchy TH(IRDB);
    LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH);

    std::cerr << "ModuleRef: " << IRDB.getWPAModule() << "\n";

    verifyIRJson(ICFG.exportICFGAsJson(), ICFG, WithDebugOutput);
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
  // std::cerr << Results.dump(4) << std::endl;
  verifySourceCodeJSON(Results,
                       readJson("linear_constant/call_07_cpp_icfg.json"));
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGSource03) {
  auto Results =
      exportICFG("exceptions/exceptions_01_cpp_dbg.ll", /*asSrcCode*/ true);
  // std::cerr << Results.dump(4) << std::endl;
  verifySourceCodeJSON(Results,
                       readJson("exceptions/exceptions_01_cpp_icfg.json"));
}

TEST_F(LLVMBasedICFGExportTest, ExportCFG01) {
  auto Results = exportCFGFor("linear_constant/branch_07_cpp_dbg.ll", "main",
                              /*asSrcCode*/ true);
  // std::cerr << Results.dump(4) << std::endl;
  verifySourceCodeJSON(Results,
                       readJson("linear_constant/branch_07_cpp_main_cfg.json"));
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
