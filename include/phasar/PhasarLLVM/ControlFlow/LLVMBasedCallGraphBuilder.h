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

/// Constructs a call-graph using the given CGResolver to resolve indirect
/// calls.
///
/// Uses a fixpoint iteration, if
/// `CGResolver.mutatesHelperAnalysisInformation()` returns true and the
/// soundness S is not Soundness::Unsound.
///
/// \param IRDB The IR code where the call-graph should be based on
/// \param CGResolver The resolver to use for resolving indirect calls.
/// \param EntryPoints The functions, where the call-graph construction should
/// start. The resulting call-graph will only contain functions that are
/// (transitively) reachable from the entry-points.
/// \param S The soundness level. May be used to trade soundness for
/// performance.
[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(const LLVMProjectIRDB &IRDB, Resolver &CGResolver,
                        llvm::ArrayRef<const llvm::Function *> EntryPoints,
                        Soundness S = Soundness::Soundy);

/// Constructs a call-graph using the given CGResolver to resolve indirect
/// calls.
///
/// Uses a fixpoint iteration, if
/// `CGResolver.mutatesHelperAnalysisInformation()` returns true and the
/// soundness S is not Soundness::Unsound.
///
/// \param IRDB The IR code where the call-graph should be based on
/// \param CGResolver The resolver to use for resolving indirect calls.
/// \param EntryPoints Names of the functions, where the call-graph construction
/// should start. The resulting call-graph will only contain functions that are
/// (transitively) reachable from the entry-points.
/// \param S The soundness level. May be used to trade soundness for
/// performance.
[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(const LLVMProjectIRDB &IRDB, Resolver &CGResolver,
                        llvm::ArrayRef<std::string> EntryPoints,
                        Soundness S = Soundness::Soundy);

/// Kept for compatibility with LLVMBasedICFG. See the constructor of
/// LLVMBasedICFG::LLVMBasedICFG(LLVMProjectIRDB *, CallGraphAnalysisType,
/// llvm::ArrayRef<std::string>, DIBasedTypeHierarchy *, LLVMAliasInfoRef,
/// Soundness, bool) for more information.
[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(LLVMProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                        llvm::ArrayRef<const llvm::Function *> EntryPoints,
                        DIBasedTypeHierarchy &TH, LLVMVFTableProvider &VTP,
                        LLVMAliasInfoRef PT = nullptr,
                        Soundness S = Soundness::Soundy);

/// Kept for compatibility with LLVMBasedICFG. See the constructor of
/// LLVMBasedICFG::LLVMBasedICFG(LLVMProjectIRDB *, CallGraphAnalysisType,
/// llvm::ArrayRef<std::string>, DIBasedTypeHierarchy *, LLVMAliasInfoRef,
/// Soundness, bool) for more information.
[[nodiscard]] LLVMBasedCallGraph
buildLLVMBasedCallGraph(LLVMProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                        llvm::ArrayRef<std::string> EntryPoints,
                        DIBasedTypeHierarchy &TH, LLVMVFTableProvider &VTP,
                        LLVMAliasInfoRef PT = nullptr,
                        Soundness S = Soundness::Soundy);

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCALLGRAPHBUILDER_H
