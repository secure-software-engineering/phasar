/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedICFG.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDICFG_H_

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/ControlFlow/ICFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/MemoryResource.h"
#include "phasar/Utils/Soundness.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/OperandTraits.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <memory>

namespace psr {
class LLVMTypeHierarchy;
class LLVMProjectIRDB;

class LLVMBasedICFG;
template <> struct CFGTraits<LLVMBasedICFG> : CFGTraits<LLVMBasedCFG> {};

class LLVMBasedICFG : public LLVMBasedCFG, public ICFGBase<LLVMBasedICFG> {
  friend ICFGBase;

  struct Builder;

public:
  class FakeTerminatorInst : public llvm::Instruction {
  public:
    [[nodiscard]] const auto *getOriginalTerminator() const noexcept {
      return OrigTerminator;
    }

    static bool classof(const llvm::Instruction * /*Inst*/) noexcept {
      llvm::report_fatal_error(
          "The FakeTerminatorInst is not embedded within "
          "the llvm::Instruction hierarchy! To perform a runtime typecheck "
          "regarding FakeTerminatorInst, use the isFakeNode() and "
          "getFakeNodeOrNull() APIs from your respective LLVMBasedICFG "
          "instead!");
      return false;
    }

    using Use = llvm::Use;

    DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);

    FakeTerminatorInst(llvm::LLVMContext &Ctx,
                       const llvm::Instruction *OrigTerminator);

  private:
    // friend LLVMBasedICFG;

    const llvm::Instruction *OrigTerminator{};
  };

  static constexpr llvm::StringLiteral GlobalCRuntimeModelName =
      "__psrCRuntimeGlobalCtorsModel";

  /// Constructs the ICFG based on the given IRDB and the entry-points using a
  /// fixpoint iteration. This may take a long time.
  ///
  /// \param IRDB The IR code where the ICFG should be based on. Must not be
  /// nullptr.
  /// \param CGType The analysis kind to use for call-graph resolution
  /// \param EntryPoints The names of the functions to start with when
  /// incrementally building up the ICFG. For whole-program analysis of an
  /// executable use {"main"}.
  /// \param TH The type-hierarchy implementation to use. Will be constructed
  /// on-the-fly if nullptr, but required
  /// \param PT The points-to implementation to use. Will be constructed
  /// on-the-fly if nullptr, but required
  /// \param S The soundness level to expect from the analysis. Currently unused
  /// \param IncludeGlobals Properly include global constructors/destructors
  /// into the ICFG, if true. Requires to generate artificial functions into the
  /// IRDB. True by default
  explicit LLVMBasedICFG(LLVMProjectIRDB *IRDB, CallGraphAnalysisType CGType,
                         llvm::ArrayRef<std::string> EntryPoints = {},
                         LLVMTypeHierarchy *TH = nullptr,
                         LLVMAliasInfoRef PT = nullptr,
                         Soundness S = Soundness::Soundy,
                         bool IncludeGlobals = true);

  /// Creates an ICFG with an already given call-graph
  explicit LLVMBasedICFG(CallGraph<n_t, f_t> CG, LLVMProjectIRDB *IRDB,
                         LLVMTypeHierarchy *TH = nullptr);

  explicit LLVMBasedICFG(LLVMProjectIRDB *IRDB,
                         const nlohmann::json &SerializedCG,
                         LLVMTypeHierarchy *TH = nullptr);

  ~LLVMBasedICFG();

  LLVMBasedICFG(const LLVMBasedICFG &) = delete;
  LLVMBasedICFG &operator=(const LLVMBasedICFG &) = delete;

  LLVMBasedICFG(LLVMBasedICFG &&) noexcept = default;
  LLVMBasedICFG &operator=(LLVMBasedICFG &&) noexcept = default;

  /// Exports the whole ICFG (not only the call-graph) as DOT.
  ///
  /// \param WithSourceCodeInfo If true, not only contains the LLVM instructions
  /// as labels, but source-code information as well (e.g. function name, line
  /// no, col no, src-line).
  [[nodiscard]] std::string
  exportICFGAsDot(bool WithSourceCodeInfo = true) const;
  /// Similar to exportICFGAsDot, but exports the ICFG as JSON instead
  [[nodiscard]] nlohmann::json
  exportICFGAsJson(bool WithSourceCodeInfo = true) const;

  [[nodiscard]] size_t getNumVertexFunctions() const noexcept {
    return CG.getNumVertexFunctions();
  }

  /// Returns all functions from the underlying IRDB that are part of the ICFG,
  /// i.e. that are reachable from the entry-points
  [[nodiscard]] auto getAllVertexFunctions() const noexcept {
    return CG.getAllVertexFunctions();
  }

  /// Gets the underlying IRDB
  [[nodiscard]] LLVMProjectIRDB *getIRDB() const noexcept { return IRDB; }

  [[nodiscard]] bool isFakeNode(n_t Node) const noexcept {
    return FakeInstructions.member(Node);
  }

  [[nodiscard]] const FakeTerminatorInst *
  getFakeNodeOrNull(n_t Node) const noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return isFakeNode(Node) ? static_cast<const FakeTerminatorInst *>(Node)
                            : nullptr;
  }

  using CFGBase::print;
  using ICFGBase::print;

  using CFGBase::getAsJson;
  using ICFGBase::getAsJson;

private:
  struct FakeTerminatorInstHolder {
    // Note: Really wish being able to store a vector<FakeTerminatorInst>, but
    // LLVM Instructions are not copyable/moveable and we need them to be
    // contiguous for an efficient member() check

    struct Deleter {
      size_t Sz{};

      Deleter() noexcept = default;
      Deleter(size_t Sz) noexcept : Sz(Sz){};

      void operator()(FakeTerminatorInst *Begin) const noexcept;
    };

    using UniquePtrTy = std::unique_ptr<FakeTerminatorInst, Deleter>;

    UniquePtrTy Data{};

    bool member(const llvm::Instruction *Inst) const noexcept {
      if (!Inst) {
        return false;
      }
      // Just to be sure, we are operating in the right domain...
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      const auto *Cast = static_cast<const FakeTerminatorInst *>(Inst);
      const auto *Begin = Data.get();
      const auto *End = Begin + Data.get_deleter().Sz;
      return Cast >= Begin && Cast < End;
    }
  };

  [[nodiscard]] FunctionRange getAllFunctionsImpl() const;
  [[nodiscard]] f_t getFunctionImpl(llvm::StringRef Fun) const;

  [[nodiscard]] bool isIndirectFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] bool isVirtualFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] std::vector<n_t> allNonCallStartNodesImpl() const;
  [[nodiscard]] llvm::SmallVector<n_t> getCallsFromWithinImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2>
  getReturnSitesOfCallAtImpl(n_t Inst) const;
  void printImpl(llvm::raw_ostream &OS) const;
  [[nodiscard]] nlohmann::json getAsJsonImpl() const;
  [[nodiscard]] const CallGraph<n_t, f_t> &getCallGraphImpl() const noexcept {
    return CG;
  }

  [[nodiscard]] llvm::Function *buildCRuntimeGlobalCtorsDtorsModel(
      llvm::Module &M, llvm::ArrayRef<llvm::Function *> UserEntryPoints);

  [[nodiscard]] FakeTerminatorInstHolder
  createFakeInstructions(const CallGraph<n_t, f_t> &CG);

  // ---

  CallGraph<const llvm::Instruction *, const llvm::Function *> CG;
  LLVMProjectIRDB *IRDB = nullptr;
  MaybeUniquePtr<LLVMTypeHierarchy, true> TH;
  FakeTerminatorInstHolder FakeInstructions;
};
} // namespace psr

#endif
