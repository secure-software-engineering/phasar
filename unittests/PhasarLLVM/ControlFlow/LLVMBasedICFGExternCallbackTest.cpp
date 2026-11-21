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

TEST(LLVMBasedICFGExternCallbackTest, Qsort) {
  doExternCallbackTest("call_graphs/extern_callback_qsort_c.ll", "qsort", "cmp",
                       /*ExpectIndirectCallbackCall*/ false,
                       /*ExpectedCallbackArgs*/ 2);
}

TEST(LLVMBasedICFGExternCallbackTest, QsortPassesArrayElements) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/extern_callback_qsort_c.ll");
  ASSERT_EQ(1U, ExternCallbackModel::rewriteCalls(IRDB));
  const auto *Model = getGeneratedModel(IRDB, "qsort");
  ASSERT_NE(nullptr, Model);
  const auto *Cmp = IRDB.getFunctionDefinition("cmp");
  ASSERT_NE(nullptr, Cmp);

  const llvm::CallBase *CallbackCall = nullptr;
  for (const auto &Inst : llvm::instructions(Model)) {
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst);
        Call && Call->getCalledFunction() == Cmp) {
      CallbackCall = Call;
    }
  }
  ASSERT_NE(nullptr, CallbackCall);
  // Each argument is base + nmemb * sizeof(int): any element of the array
  for (const auto &Arg : CallbackCall->args()) {
    const auto *Elem = llvm::dyn_cast<llvm::GetElementPtrInst>(Arg.get());
    ASSERT_NE(nullptr, Elem);
    EXPECT_EQ(Model->getArg(0), Elem->getPointerOperand());
    EXPECT_EQ(Model->getArg(1), Elem->getOperand(1));
    const auto *ElemTy =
        llvm::dyn_cast<llvm::ArrayType>(Elem->getSourceElementType());
    ASSERT_NE(nullptr, ElemTy);
    EXPECT_EQ(sizeof(int), ElemTy->getNumElements());
  }
}

TEST(LLVMBasedICFGExternCallbackTest, SearchBrokers) {
  constexpr llvm::StringLiteral File =
      "call_graphs/extern_callback_search_c.ll";
  doExternCallbackTest(File, "bsearch", "cmpB", false, 2);
  doExternCallbackTest(File, "lfind", "cmpL", false, 2);
  doExternCallbackTest(File, "qsort_r", "cmpR", false, 3);
}

TEST(LLVMBasedICFGExternCallbackTest, LfindLoadsTheCount) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/extern_callback_search_c.ll");
  ASSERT_EQ(3U, ExternCallbackModel::rewriteCalls(IRDB));
  const auto *Model = getGeneratedModel(IRDB, "lfind");
  ASSERT_NE(nullptr, Model);
  const auto *Cmp = IRDB.getFunctionDefinition("cmpL");
  ASSERT_NE(nullptr, Cmp);
  const llvm::CallBase *CallbackCall = nullptr;
  for (const auto &Inst : llvm::instructions(Model)) {
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst);
        Call && Call->getCalledFunction() == Cmp) {
      CallbackCall = Call;
    }
  }
  ASSERT_NE(nullptr, CallbackCall);
  // cmpL(key, base + *nmemp * size)
  EXPECT_EQ(Model->getArg(0), CallbackCall->getArgOperand(0));
  const auto *Elem =
      llvm::dyn_cast<llvm::GetElementPtrInst>(CallbackCall->getArgOperand(1));
  ASSERT_NE(nullptr, Elem);
  EXPECT_EQ(Model->getArg(1), Elem->getPointerOperand());
  const auto *Count = llvm::dyn_cast<llvm::LoadInst>(Elem->getOperand(1));
  ASSERT_NE(nullptr, Count);
  EXPECT_EQ(Model->getArg(2), Count->getPointerOperand());
}

TEST(LLVMBasedICFGExternCallbackTest, Scandir) {
  constexpr llvm::StringLiteral File =
      "call_graphs/extern_callback_scandir_c.ll";
  doExternCallbackTest(File, "scandir", "keep");
  doExternCallbackTest(File, "scandir", "byName", false, 2);
}

TEST(LLVMBasedICFGExternCallbackTest, ScandirPassesListEntries) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/extern_callback_scandir_c.ll");
  ASSERT_EQ(1U, ExternCallbackModel::rewriteCalls(IRDB));
  const auto *Model = getGeneratedModel(IRDB, "scandir");
  ASSERT_NE(nullptr, Model);
  const auto *Keep = IRDB.getFunctionDefinition("keep");
  const auto *ByName = IRDB.getFunctionDefinition("byName");
  ASSERT_NE(nullptr, Keep);
  ASSERT_NE(nullptr, ByName);

  const llvm::CallBase *Broker = nullptr;
  const llvm::CallBase *KeepCall = nullptr;
  const llvm::CallBase *ByNameCall = nullptr;
  for (const auto &Inst : llvm::instructions(Model)) {
    const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst);
    if (!Call) {
      continue;
    }
    const auto *Callee = Call->getCalledFunction();
    if (Callee && Callee->getName() == "scandir") {
      Broker = Call;
    } else if (Callee == Keep) {
      KeepCall = Call;
    } else if (Callee == ByName) {
      ByNameCall = Call;
    }
  }
  ASSERT_NE(nullptr, Broker);
  ASSERT_NE(nullptr, KeepCall);
  ASSERT_NE(nullptr, ByNameCall);

  // An element of *namelist, indexed by scandir's result
  const auto IsEntrySlot = [&](const llvm::Value *V) {
    const auto *Elem = llvm::dyn_cast<llvm::GetElementPtrInst>(V);
    if (!Elem || Elem->getOperand(1) != Broker) {
      return false;
    }
    const auto *List =
        llvm::dyn_cast<llvm::LoadInst>(Elem->getPointerOperand());
    return List && List->getPointerOperand() == Model->getArg(1);
  };
  // keep(entry), byName(&entry, &entry)
  const auto *Entry =
      llvm::dyn_cast<llvm::LoadInst>(KeepCall->getArgOperand(0));
  ASSERT_NE(nullptr, Entry);
  EXPECT_TRUE(IsEntrySlot(Entry->getPointerOperand()));
  for (const auto &Arg : ByNameCall->args()) {
    EXPECT_TRUE(IsEntrySlot(Arg.get()));
  }
}

TEST(LLVMBasedICFGExternCallbackTest, MiscBrokers) {
  constexpr llvm::StringLiteral File = "call_graphs/extern_callback_misc_c.ll";
  doExternCallbackTest(File, "thrd_create", "thrdWorker");
  doExternCallbackTest(File, "dl_iterate_phdr", "visitObj", false, 3);
  doExternCallbackTest(File, "nftw", "visitFile", false, 4);
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
