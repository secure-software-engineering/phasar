/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/ExternCallbackModel.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

namespace {

const llvm::Function *getGeneratedModel(const LLVMProjectIRDB &IRDB,
                                        llvm::StringRef BrokerName) {
  std::string Prefix = (ExternCallbackModel::ModelPrefix + llvm::Twine('.') +
                        BrokerName + llvm::Twine('.'))
                           .str();
  for (const auto *F : IRDB.getAllFunctions()) {
    if (F->getName().starts_with(Prefix)) {
      return F;
    }
  }

  return nullptr;
}

llvm::SmallVector<const llvm::Function *, 4>
getGeneratedModels(const LLVMProjectIRDB &IRDB, llvm::StringRef BrokerName) {
  llvm::SmallVector<const llvm::Function *, 4> Models;
  std::string Prefix = (ExternCallbackModel::ModelPrefix + llvm::Twine('.') +
                        BrokerName + llvm::Twine('.'))
                           .str();
  for (const auto *F : IRDB.getAllFunctions()) {
    if (F->getName().starts_with(Prefix)) {
      Models.push_back(F);
    }
  }

  return Models;
}

const llvm::CallBase *getCallbackCall(const llvm::Function &Model,
                                      const LLVMBasedICFG &ICFG,
                                      const llvm::Function &Callback) {
  for (const auto &Inst : llvm::instructions(Model)) {
    const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst);
    if (!Call) {
      continue;
    }

    auto Callees = ICFG.getCalleesOfCallAt(Call);
    if (Callees == llvm::ArrayRef{&Callback}) {
      return Call;
    }
  }

  return nullptr;
}

const llvm::CallBase *getCallbackCall(const llvm::Function &Model,
                                      const LLVMBasedCallGraph &CG,
                                      const llvm::Function &Callback) {
  for (const auto &Inst : llvm::instructions(Model)) {
    const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst);
    if (!Call) {
      continue;
    }

    auto Callees = CG.getCalleesOfCallAt(Call);
    if (Callees == llvm::ArrayRef{&Callback}) {
      return Call;
    }
  }

  return nullptr;
}

void doExternCallbackTest(llvm::StringRef IRFile, llvm::StringRef BrokerName,
                          llvm::StringRef CallbackName,
                          bool ExpectIndirectCallbackCall = false,
                          unsigned ExpectedCallbackArgs = 1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles + IRFile);
  DIBasedTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, nullptr,
                     Soundness::Soundy, /*IncludeGlobals*/ false);

  const auto *Model = getGeneratedModel(IRDB, BrokerName);
  ASSERT_NE(nullptr, Model);

  const auto *Callback = IRDB.getFunctionDefinition(CallbackName);
  ASSERT_NE(nullptr, Callback);

  const auto *CallbackCall = getCallbackCall(*Model, ICFG, *Callback);
  ASSERT_NE(nullptr, CallbackCall);
  EXPECT_EQ(ExpectIndirectCallbackCall,
            ICFG.isIndirectFunctionCall(CallbackCall));
  EXPECT_EQ(ExpectedCallbackArgs, CallbackCall->arg_size());

  auto Callees = ICFG.getCalleesOfCallAt(CallbackCall);
  EXPECT_EQ(llvm::ArrayRef{Callback}, Callees);
}

void doExternCallbackCallGraphBuilderTest(llvm::StringRef IRFile,
                                          llvm::StringRef BrokerName,
                                          llvm::StringRef CallbackName,
                                          unsigned ExpectedCallbackArgs = 1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles + IRFile);
  DIBasedTypeHierarchy TH(IRDB);
  LLVMVFTableProvider VTP(IRDB);
  RTAResolver Resolver(&IRDB, &VTP, &TH);

  auto CG =
      buildLLVMBasedCallGraphWithExternCallbackModels(IRDB, Resolver, {"main"});

  const auto *Model = getGeneratedModel(IRDB, BrokerName);
  ASSERT_NE(nullptr, Model);

  const auto *Callback = IRDB.getFunctionDefinition(CallbackName);
  ASSERT_NE(nullptr, Callback);

  const auto *CallbackCall = getCallbackCall(*Model, CG, *Callback);
  ASSERT_NE(nullptr, CallbackCall);
  EXPECT_EQ(ExpectedCallbackArgs, CallbackCall->arg_size());

  auto Callees = CG.getCalleesOfCallAt(CallbackCall);
  EXPECT_EQ(llvm::ArrayRef{Callback}, Callees);
}

unsigned countCallbackModels(llvm::ArrayRef<const llvm::Function *> Models,
                             const LLVMBasedICFG &ICFG,
                             const llvm::Function &Callback) {
  unsigned Count = 0;
  for (const auto *Model : Models) {
    const auto *CallbackCall = getCallbackCall(*Model, ICFG, Callback);
    if (!CallbackCall) {
      continue;
    }

    EXPECT_EQ(1U, CallbackCall->arg_size());
    ++Count;
  }

  return Count;
}

void expectNoRewrite(llvm::StringRef IRFile, llvm::StringRef BrokerName) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles + IRFile);
  ASSERT_TRUE(static_cast<bool>(IRDB));

  EXPECT_EQ(0U, ExternCallbackModel::rewriteCalls(IRDB));
  EXPECT_TRUE(getGeneratedModels(IRDB, BrokerName).empty());
}

} // namespace

TEST(LLVMBasedICFGExternCallbackTest, PthreadCreate) {
  doExternCallbackTest("call_graphs/extern_callback_pthread_c.ll",
                       "pthread_create", "worker");
}

TEST(LLVMBasedICFGExternCallbackTest, PthreadCreateIndirect) {
  doExternCallbackTest("call_graphs/extern_callback_pthread_indirect_c.ll",
                       "pthread_create", "worker",
                       /*ExpectIndirectCallbackCall*/ true);
}

TEST(LLVMBasedICFGExternCallbackTest, PthreadCreateMultipleCallSites) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/extern_callback_pthread_multiple_c.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, nullptr,
                     Soundness::Soundy, /*IncludeGlobals*/ false);

  auto Models = getGeneratedModels(IRDB, "pthread_create");
  EXPECT_EQ(2U, Models.size());

  const auto *FirstCallback = IRDB.getFunctionDefinition("first_worker");
  ASSERT_NE(nullptr, FirstCallback);
  const auto *SecondCallback = IRDB.getFunctionDefinition("second_worker");
  ASSERT_NE(nullptr, SecondCallback);

  EXPECT_EQ(1U, countCallbackModels(Models, ICFG, *FirstCallback));
  EXPECT_EQ(1U, countCallbackModels(Models, ICFG, *SecondCallback));
}

TEST(LLVMBasedICFGExternCallbackTest, RewriteCallsIsIdempotent) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/extern_callback_pthread_multiple_c.ll");

  EXPECT_EQ(2U, ExternCallbackModel::rewriteCalls(IRDB));
  EXPECT_EQ(0U, ExternCallbackModel::rewriteCalls(IRDB));
  EXPECT_EQ(2U, getGeneratedModels(IRDB, "pthread_create").size());
}

TEST(LLVMBasedICFGExternCallbackTest, KmpcForkCall) {
  doExternCallbackTest("call_graphs/extern_callback_kmpc_fork_call_c.ll",
                       "__kmpc_fork_call", "microtask",
                       /*ExpectIndirectCallbackCall*/ false,
                       /*ExpectedCallbackArgs*/ 3);
}

TEST(LLVMBasedICFGExternCallbackTest, KmpcForkTeams) {
  doExternCallbackTest("call_graphs/extern_callback_kmpc_fork_teams_c.ll",
                       "__kmpc_fork_teams", "team_microtask",
                       /*ExpectIndirectCallbackCall*/ false,
                       /*ExpectedCallbackArgs*/ 3);
}

TEST(LLVMBasedICFGExternCallbackTest, CallbackAttribute) {
  doExternCallbackTest("call_graphs/extern_callback_attribute_c.ll",
                       "callback_broker", "sink");
}

TEST(LLVMBasedICFGExternCallbackTest, CallbackAttributeIndirect) {
  doExternCallbackTest("call_graphs/extern_callback_attribute_indirect_c.ll",
                       "callback_broker", "sink",
                       /*ExpectIndirectCallbackCall*/ true);
}

TEST(LLVMBasedICFGExternCallbackTest, CallGraphBuilderResolverOverload) {
  doExternCallbackCallGraphBuilderTest(
      "call_graphs/extern_callback_pthread_c.ll", "pthread_create", "worker");
}

TEST(LLVMBasedICFGExternCallbackTest, CallGraphBuilderCGTypeOverloadIndirect) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/extern_callback_pthread_indirect_c.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMVFTableProvider VTP(IRDB);

  auto CG = buildLLVMBasedCallGraph(IRDB, CallGraphAnalysisType::OTF, {"main"},
                                    TH, VTP);

  const auto *Model = getGeneratedModel(IRDB, "pthread_create");
  ASSERT_NE(nullptr, Model);

  const auto *Callback = IRDB.getFunctionDefinition("worker");
  ASSERT_NE(nullptr, Callback);

  const auto *CallbackCall = getCallbackCall(*Model, CG, *Callback);
  ASSERT_NE(nullptr, CallbackCall);
  EXPECT_TRUE(CallbackCall->isIndirectCall());
}

TEST(LLVMBasedICFGExternCallbackTest, InvalidMetadataCallbackArg) {
  expectNoRewrite("call_graphs/extern_callback_invalid_metadata_arg_c.ll",
                  "callback_broker");
}

TEST(LLVMBasedICFGExternCallbackTest, TooFewBrokerArgs) {
  expectNoRewrite("call_graphs/extern_callback_too_few_broker_args_c.ll",
                  "pthread_create");
}

TEST(LLVMBasedICFGExternCallbackTest, NonPointerCallbackCalleeArg) {
  expectNoRewrite("call_graphs/extern_callback_non_pointer_callee_arg_c.ll",
                  "callback_broker");
}

TEST(LLVMBasedICFGExternCallbackTest, MismatchedCallbackArity) {
  expectNoRewrite("call_graphs/extern_callback_mismatched_callback_arity_c.ll",
                  "callback_broker");
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
