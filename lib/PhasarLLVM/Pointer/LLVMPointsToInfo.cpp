/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Value.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>

namespace psr {

LLVMPointsToInfo::LLVMPointsToInfo(ProjectIRDB &IRDB) {
  //     // TODO Have a look at this stuff from the future at some point in time
  //   /// PassManagerBuilder - This class is used to set up a standard
  //   /// optimization
  //   /// sequence for languages like C and C++, allowing some APIs to
  //   customize
  //   /// the
  //   /// pass sequence in various ways. A simple example of using it would be:
  //   ///
  //   ///  PassManagerBuilder Builder;
  //   ///  Builder.OptLevel = 2;
  //   ///  Builder.populateFunctionPassManager(FPM);
  //   ///  Builder.populateModulePassManager(MPM);
  //   ///
  //   /// In addition to setting up the basic passes, PassManagerBuilder allows
  //   /// frontends to vend a plugin API, where plugins are allowed to add
  //   /// extensions
  //   /// to the default pass manager.  They do this by specifying where in the
  //   /// pass
  //   /// pipeline they want to be added, along with a callback function that
  //   adds
  //   /// the pass(es).  For example, a plugin that wanted to add a loop
  //   /// optimization
  //   /// could do something like this:
  //   ///
  //   /// static void addMyLoopPass(const PMBuilder &Builder, PassManagerBase
  //   &PM)
  //   /// {
  //   ///   if (Builder.getOptLevel() > 2 && Builder.getOptSizeLevel() == 0)
  //   ///     PM.add(createMyAwesomePass());
  //   /// }
  //   ///   ...
  //   ///   Builder.addExtension(PassManagerBuilder::EP_LoopOptimizerEnd,
  //   ///                        addMyLoopPass);
  //   ///   ...
  //   // But for now, stick to what is well debugged
  //   llvm::legacy::PassManager PM;
  //   if (Options & IRDBOptions::MEM2REG) {
  //     llvm::FunctionPass *Mem2Reg =
  //     llvm::createPromoteMemoryToRegisterPass(); PM.add(Mem2Reg);
  //   }
  //   GeneralStatisticsPass *GSP = new GeneralStatisticsPass();
  //   ValueAnnotationPass *VAP = new ValueAnnotationPass(M->getContext());
  //   // Mandatory passed for the alias analysis
  //   auto BasicAAWP = llvm::createBasicAAWrapperPass();
  //   auto TargetLibraryWP = new llvm::TargetLibraryInfoWrapperPass();
  //   // Optional, more precise alias analysis
  //   // auto ScopedNoAliasAAWP = llvm::createScopedNoAliasAAWrapperPass();
  //   // auto TBAAWP = llvm::createTypeBasedAAWrapperPass();
  //   // auto ObjCARCAAWP = llvm::createObjCARCAAWrapperPass();
  //   // auto SCEVAAWP = llvm::createSCEVAAWrapperPass();
  //   auto CFLAndersAAWP = llvm::createCFLAndersAAWrapperPass();
  //   // auto CFLSteensAAWP = llvm::createCFLSteensAAWrapperPass();
  //   // Add the passes
  //   PM.add(GSP);
  //   PM.add(VAP);
  //   PM.add(BasicAAWP);
  //   PM.add(TargetLibraryWP);
  //   // PM.add(ScopedNoAliasAAWP);
  //   // PM.add(TBAAWP);
  //   // PM.add(ObjCARCAAWP);
  //   // PM.add(SCEVAAWP);
  //   PM.add(CFLAndersAAWP);
  //   // PM.add(CFLSteensAAWP);
  //   PM.run(*M);
  //   // just to be sure that none of the passes has messed up the module!
  //   bool broken_debug_info = false;
  //   if (M == nullptr ||
  //       llvm::verifyModule(*M, &llvm::errs(), &broken_debug_info)) {
  //     LOG_IF_ENABLE(BOOST_LOG_SEV(lg, CRITICAL)
  //                   << "AnalysisController: module is broken!");
  //   }
  //   if (broken_debug_info) {
  //     LOG_IF_ENABLE(BOOST_LOG_SEV(lg, WARNING)
  //                   << "AnalysisController: debug info is broken.");
  //   }
  //   for (auto RR : GSP->getRetResInstructions()) {
  //     ret_res_instructions.insert(RR);
  //   }
  //   for (auto A : GSP->getAllocaInstructions()) {
  //     alloca_instructions.insert(A);
  //   }
  //   // Obtain the allocated types found in the module
  //   allocated_types = GSP->getAllocatedTypes();
  //   // Obtain the very important alias analysis results
  //   // and construct the intra-procedural points-to graphs.
  //   for (auto &F : *M) {
  //     // When module-wise analysis is performed, declarations might occure
  //     // causing meaningless points-to graphs to be produced.
  //     if (!F.isDeclaration()) {
  //       llvm::BasicAAResult BAAResult =
  //           createLegacyPMBasicAAResult(*BasicAAWP, F);
  //       llvm::AAResults AARes =
  //           llvm::createLegacyPMAAResults(*BasicAAWP, F, BAAResult);
  //       // This line is a major slowdown
  //       // The problem comes from the generation of PtG which is far too slow
  //       // due to the use of llvmIRToString (without it, the generation of
  //       PtG is
  //       // very acceptable)
  //       insertPointsToGraph(F.getName().str(), new PointsToGraph(AARes, &F));
  //     }
  //   }
}

AliasResult LLVMPointsToInfo::alias(const llvm::Value *V1,
                                    const llvm::Value *V2) const {
  return AliasResult::MayAlias;
}

std::set<const llvm::Value *>
LLVMPointsToInfo::getPointsToSet(const llvm::Value *V1) const {
  return {};
}

nlohmann::json LLVMPointsToInfo::getAsJson() const { return ""_json; }

PointsToGraph *
LLVMPointsToInfo::getPointsToGraph(const std::string &FunctionName) const {
  return nullptr;
}

} // namespace psr
