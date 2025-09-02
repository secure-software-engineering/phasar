/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

// #include "phasar/AnalysisConfig.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/call_graph.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include <string>

namespace {
[[nodiscard]] std::string printStringSet(const std::set<llvm::StringRef> &Set) {
  std::string Ret;
  llvm::raw_string_ostream OS(Ret);
  llvm::interleaveComma(Set, OS << "{ ");
  OS << " }";
  return Ret;
}
/////////////////////////////
psr::LLVMBasedICFG createBaseCG(psr::LLVMProjectIRDB &IRDB,
                                const psr::LLVMVFTableProvider &VTP,
                                const psr::DIBasedTypeHierarchy &TH,
                                psr::LLVMAliasInfoRef /*PT*/) {
  psr::RTAResolver Res(&IRDB, &VTP, &TH);

  std::vector<std::string> EntryPoints;
  ///////////////////////////////////
  if (IRDB.getFunctionDefinition("main")) {
    EntryPoints.emplace_back("main");
  } else {
    for (const auto *F : IRDB.getAllFunctions()) {
      if (!F->isDeclaration() && F->hasExternalLinkage()) {
        EntryPoints.emplace_back(F->getName());
      }
    }
  }
  ///////////////////////////////////
  return psr::LLVMBasedICFG(&IRDB, Res, EntryPoints, psr::Soundness::Soundy);
}
//////////////////////////////
class VTACallGraphTest : public ::testing::Test {
protected:
  static constexpr llvm::StringLiteral PathToLLFiles =
      "/build/phasar/test/llvm_test_code/";

  struct GroundTruthEntry {
    size_t CSId;
    std::set<llvm::StringRef> Callees;
  };

  void doAnalysisAndCompareResults(const llvm::Twine &IRFile,
                                   llvm::ArrayRef<GroundTruthEntry> GT) {
    ASSERT_FALSE(GT.empty()) << "No Ground-Truth provided!";

    auto IRDB = std::make_unique<psr::LLVMProjectIRDB>(PathToLLFiles + IRFile);
    ASSERT_TRUE(IRDB->isValid());

    psr::LLVMVFTableProvider VTP(*IRDB);
    psr::DIBasedTypeHierarchy TH(*IRDB);
    psr::LLVMAliasSet AS(IRDB.get());
    // implement function locally
    auto BaseCG = createBaseCG(*IRDB, VTP, TH, &AS);

    auto CG = psr::computeVTACallgraph(*IRDB->getModule(),
                                       BaseCG.getCallGraph(), &AS, VTP);

    for (const auto &Entry : GT) {
      const auto *CS = IRDB->getInstruction(Entry.CSId);
      ASSERT_NE(nullptr, CS);
      ASSERT_TRUE(llvm::isa<llvm::CallBase>(CS));
      auto &&Callees = CG.getCalleesOfCallAt(CS);

      EXPECT_EQ(Entry.Callees.size(), Callees.size());

      auto GTCallees = Entry.Callees;
      for (const auto *Callee : Callees) {
        auto CalleeName = Callee->getName();
        EXPECT_TRUE(Entry.Callees.count(CalleeName))
            << "Did not expect function '" << CalleeName.str()
            << "' being called at " << psr::llvmIRToString(CS);
        GTCallees.erase(CalleeName);
      }

      EXPECT_TRUE(GTCallees.empty())
          << "Expected callees not found at " << psr::llvmIRToString(CS) << ": "
          << printStringSet(GTCallees);
    }
  }
};

TEST_F(VTACallGraphTest, VirtualCallSite_InterProcCallSite) {
  doAnalysisAndCompareResults("virtual_callsites/interproc_callsite_cpp.ll",
                              {
                                  {16, {"_ZN7Derived3barEv"}},
                              });
}

TEST_F(VTACallGraphTest, UninitializedVariables_VirtualCall) {
  doAnalysisAndCompareResults("uninitialized_variables/virtual_call_cpp_dbg.ll",
                              {
                                  {34, {"_Z3barRi", "_Z3fooRi"}},
                              });
}

TEST_F(VTACallGraphTest, PathTracing_Inter12) {
  // Note: The VTA analysis is not flow-sensitive
  doAnalysisAndCompareResults(
      "path_tracing/inter_12_cpp_dbg.ll",
      {
          {33, {"_ZN3TwoD0Ev", "_ZN5ThreeD0Ev"}},
          {45, {"_ZN5Three11assignValueEi", "_ZN3Two11assignValueEi"}},
      });
}

TEST_F(VTACallGraphTest, CallGraphs_FunctionPointer1) {
  doAnalysisAndCompareResults("call_graphs/function_pointer_1_c.ll",
                              {
                                  {5, {"bar"}},
                              });
}
TEST_F(VTACallGraphTest, CallGraphs_FunctionPointer2) {
  doAnalysisAndCompareResults("call_graphs/function_pointer_2_cpp.ll",
                              {
                                  {8, {"_Z3barv"}},
                              });
}
TEST_F(VTACallGraphTest, CallGraphs_FunctionPointer3) {
  // Note: Although bar is assigned (and part of the TAG), is does not qualify
  // as psr::isConsistentCall()
  doAnalysisAndCompareResults("call_graphs/function_pointer_3_cpp.ll",
                              {
                                  {11, {/*"_Z3bari",*/ "_Z3foov"}},
                              });
}
TEST_F(VTACallGraphTest, CallGraphs_VirtualCall2) {
  doAnalysisAndCompareResults("call_graphs/virtual_call_2_cpp.ll",
                              {
                                  {20, {"_ZN1B3fooEv"}},
                              });
}
TEST_F(VTACallGraphTest, CallGraphs_VirtualCall3) {
  // Use the dbg version, because VTA relies on !heapallocsite metadata
  doAnalysisAndCompareResults("call_graphs/virtual_call_3_cpp_dbg.ll",
                              {
                                  {22, {"_ZN5AImpl3fooEv"}},
                                  {30, {"_ZN5AImplD0Ev"}},
                              });
}
TEST_F(VTACallGraphTest, CallGraphs_VirtualCall4) {
  doAnalysisAndCompareResults("call_graphs/virtual_call_4_cpp.ll",
                              {
                                  {20, {"_ZN1B3fooEv"}},
                              });
}
TEST_F(VTACallGraphTest, CallGraphs_VirtualCall5) {
  // Use the dbg version, because VTA relies on !heapallocsite metadata
  doAnalysisAndCompareResults("call_graphs/virtual_call_5_cpp_dbg.ll",
                              {
                                  {24, {"_ZN1B5VfuncEv"}},
                                  {32, {"_ZN1BD0Ev"}},
                              });
}
TEST_F(VTACallGraphTest, CallGraphs_VirtualCall7) {
  // Use the dbg version, because VTA relies on !heapallocsite metadata
  doAnalysisAndCompareResults("call_graphs/virtual_call_7_cpp_dbg.ll",
                              {
                                  {28, {"_ZN1A5VfuncEv"}},
                                  {34, {"_ZN1B5VfuncEv"}},
                                  {42, {"_ZN1AD0Ev"}},
                              });
}
TEST_F(VTACallGraphTest, DISABLED_CallGraphs_VirtualCall8) {

  // Use the dbg version, because VTA relies on !heapallocsite metadata
  // Note: The VTA analysis is neither flow-, nor context-sensitive
  doAnalysisAndCompareResults(
      "call_graphs/virtual_call_8_cpp_dbg.ll",
      {
          {26, {"_ZZ4mainEN1B3fooEv", "_ZZ4mainEN1C3fooEv"}},
          {32, {"_ZZ4mainEN1B3fooEv", "_ZZ4mainEN1C3fooEv"}},
      });
}
TEST_F(VTACallGraphTest, CallGraphs_VirtualCall9) {
  // Use the dbg version, because VTA relies on !heapallocsite metadata
  // Note: The VTA analysis is neither flow-, nor context-sensitive
  doAnalysisAndCompareResults(
      "call_graphs/virtual_call_9_cpp_dbg.ll",
      {
          {85, {"_ZN1B3fooEv", "_ZN1C3fooEv", "_ZN1D3fooEv"}},
          {93, {"_ZN1BD0Ev", "_ZN1CD0Ev", "_ZN1DD0Ev"}},
      });
}
// TODO: More tests!

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
