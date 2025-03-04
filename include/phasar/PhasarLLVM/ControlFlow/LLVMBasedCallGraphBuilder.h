/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCALLGRAPHBUILDER_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCALLGRAPHBUILDER_H

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/Soundness.h"

namespace psr {
class LLVMProjectIRDB;
enum class CallGraphAnalysisType;
class DIBasedTypeHierarchy;
class LLVMVFTableProvider;
class Resolver;

[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(LLVMProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                        llvm::ArrayRef<const llvm::Function *> EntryPoints,
                        DIBasedTypeHierarchy &TH, LLVMVFTableProvider &VTP,
                        LLVMAliasInfoRef PT = nullptr,
                        Soundness S = Soundness::Soundy);

[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(const LLVMProjectIRDB &IRDB, Resolver &CGResolver,
                        llvm::ArrayRef<const llvm::Function *> EntryPoints,
                        Soundness S = Soundness::Soundy);

[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(LLVMProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                        llvm::ArrayRef<std::string> EntryPoints,
                        DIBasedTypeHierarchy &TH, LLVMVFTableProvider &VTP,
                        LLVMAliasInfoRef PT = nullptr,
                        Soundness S = Soundness::Soundy);

[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(const LLVMProjectIRDB &IRDB, Resolver &CGResolver,
                        llvm::ArrayRef<std::string> EntryPoints,
                        Soundness S = Soundness::Soundy);
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCALLGRAPHBUILDER_H
