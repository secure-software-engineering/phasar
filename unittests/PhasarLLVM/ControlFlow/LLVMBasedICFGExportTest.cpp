#include "gtest/gtest.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Twine.h"
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
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

using nlohmann::json;

template <typename TKey, typename TValue>
using MapTy = llvm::DenseMap<TKey, TValue>;

class LLVMBasedICFGExportTest : public ::testing::Test {
protected:
  PSR_CONST_CONSTEXPR std::string pathToLLFiles = unittest::PathToLLTestFiles;
  PSR_CONST_CONSTEXPR std::string pathToJSONFiles =
      unittest::PathToJSONTestFiles;

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  json exportICFG(const std::string &testFile, bool asSrcCode = false) {
    ProjectIRDB IRDB({pathToLLFiles + testFile}, IRDBOptions::WPA);
    LLVMTypeHierarchy TH(IRDB);
    LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);

    return asSrcCode ? ICFG.exportICFGAsSourceCodeJson()
                     : ICFG.exportICFGAsJson();
  }

  MapTy<const llvm::Function *, llvm::SmallPtrSet<const llvm::Instruction *, 2>>
  getAllRetSites(const LLVMBasedICFG &ICFG) {
    MapTy<const llvm::Function *,
          llvm::SmallPtrSet<const llvm::Instruction *, 2>>
        RetSitesOf;

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

  template <bool WithDebugOutput = false>
  void verifyIRJson(const json &ExportedICFG,
                    const LLVMBasedICFG &GroundTruth) {
    auto RetSitesOf = getAllRetSites(GroundTruth);

    auto expectEdge = [&](const std::string &From, const std::string &To) {
      EXPECT_TRUE(std::any_of(ExportedICFG.begin(), ExportedICFG.end(),
                              [&](auto &&Elem) {
                                return Elem == json{{"from", From}, {"to", To}};
                              }))
          << "No edge from " << From << " to " << To;
    };

    auto print = [](auto &&...Args) {
      if constexpr (WithDebugOutput) {
        ((llvm::errs() << Args), ...);
        llvm::errs() << "\n";
      }
    };

    for (const auto *F : GroundTruth.getAllFunctions()) {
      for (const auto &Inst : llvm::instructions(F)) {
        auto InstStr = llvmIRToString(&Inst);
        if (llvm::isa<llvm::CallBase>(&Inst)) {
          for (const auto *Callee : GroundTruth.getCalleesOfCallAt(&Inst)) {
            if (!Callee->isDeclaration()) {
              print("Callee: ", *Callee);
              expectEdge(InstStr, llvmIRToString(&Callee->front().front()));

              print("> end");
            }
          }
        } else if (llvm::isa<llvm::ReturnInst>(&Inst) ||
                   llvm::isa<llvm::ResumeInst>(&Inst)) {
          const auto &RetSites = RetSitesOf[Inst.getFunction()];

          for (const auto *Ret : RetSites) {
            expectEdge(InstStr, llvmIRToString(Ret));
          }
        } else {
          for (const auto *Succ : GroundTruth.getSuccsOf(&Inst)) {
            expectEdge(InstStr, llvmIRToString(Succ));
          }
        }
      }
    }
  }

  void verifySourceCodeJSON(const json &ExportedICFG, const json &GroundTruth) {
    ASSERT_TRUE(ExportedICFG.is_array());

    EXPECT_EQ(GroundTruth.size(), ExportedICFG.size());

    for (const auto &GTJson : GroundTruth) {
      auto GTFrom = GTJson.at("from").get<SourceCodeInfo>();
      auto GTTo = GTJson.at("to").get<SourceCodeInfo>();
      EXPECT_TRUE(std::any_of(ExportedICFG.begin(), ExportedICFG.end(),
                              [&](const json &ExportedInfoJson) {
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

  json readJson(const std::string &jsonfileName) {
    std::ifstream fIn(pathToJSONFiles + jsonfileName);
    assert(fIn.good() && "Invalid JSON file");

    json J;
    fIn >> J;

    assert(fIn.good() && "Error reading JSON file");
    return J;
  }

  template <bool WithDebugOutput = false>
  void verifyExportICFG(const std::string &testFile) {
    ProjectIRDB IRDB({pathToLLFiles + testFile}, IRDBOptions::WPA);
    LLVMTypeHierarchy TH(IRDB);
    LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);

    verifyIRJson<WithDebugOutput>(ICFG.exportICFGAsJson(), ICFG);
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
  auto results =
      exportICFG("linear_constant/call_01_cpp_dbg.ll", /*asSrcCode*/ true);
  verifySourceCodeJSON(results,
                       readJson("linear_constant/call_01_cpp_icfg.json"));
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGSource02) {
  auto results =
      exportICFG("linear_constant/call_07_cpp_dbg.ll", /*asSrcCode*/ true);
  // std::cerr << results.dump(4) << std::endl;
  verifySourceCodeJSON(results,
                       readJson("linear_constant/call_07_cpp_icfg.json"));
}

TEST_F(LLVMBasedICFGExportTest, ExportICFGSource03) {
  auto results =
      exportICFG("exceptions/exceptions_01_cpp_dbg.ll", /*asSrcCode*/ true);
  // std::cerr << results.dump(4) << std::endl;
  verifySourceCodeJSON(results,
                       readJson("exceotions/exceptions_01_cpp_icfg.json"));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}