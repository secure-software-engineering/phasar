/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPH_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPH_H

#include "phasar/ControlFlow/CallGraphBase.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/StableVector.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"

#include "nlohmann/json.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace psr {
template <typename N, typename F> class CallGraphBuilder;
template <typename N, typename F> class CallGraph;

template <typename N, typename F> struct CGTraits<CallGraph<N, F>> {
  using n_t = N;
  using f_t = F;
};

/// An explicit graph-representation of a call-graph. Only represents the data,
/// not the call-graph analysis that creates it.
///
/// This type is immutable. To incrementally build it from your call-graph
/// analysis, use the CallGraphBuilder
template <typename N, typename F>
class CallGraph : public CallGraphBase<CallGraph<N, F>> {
  using base_t = CallGraphBase<CallGraph<N, F>>;
  friend base_t;
  friend class CallGraphBuilder<N, F>;

public:
  using typename base_t::f_t;
  using typename base_t::n_t;
  using FunctionVertexTy = llvm::SmallVector<n_t>;
  using InstructionVertexTy = llvm::SmallVector<f_t>;

  /// Creates a new, empty call-graph
  CallGraph() noexcept = default;

  /// Deserializes a previously computed call-graph
  template <typename FunctionGetter, typename InstructionGetter>
  explicit CallGraph(const nlohmann::json &PrecomputedCG,
                     FunctionGetter GetFunctionFromName,
                     InstructionGetter GetInstructionFromId) {

    if (!PrecomputedCG.is_object()) {
      PHASAR_LOG_LEVEL_CAT(ERROR, "CallGraph", "Invalid Json. Expected object");
      return;
    }

    CallersOf.reserve(PrecomputedCG.size());
    CalleesAt.reserve(PrecomputedCG.size());
    FunVertexOwner.reserve(PrecomputedCG.size());

    for (const auto &[FunName, CallerIDs] : PrecomputedCG.items()) {
      const auto &Fun = std::invoke(GetFunctionFromName, FunName);
      if (!Fun) {
        PHASAR_LOG_LEVEL_CAT(WARNING, "CallGraph",
                             "Invalid function name: " << FunName);
        continue;
      }

      auto *CEdges = addFunctionVertex(Fun);
      CEdges->reserve(CallerIDs.size());

      for (const auto &JId : CallerIDs) {
        auto Id = JId.get<size_t>();
        const auto &CS = std::invoke(GetInstructionFromId, Id);
        if (!CS) {
          PHASAR_LOG_LEVEL_CAT(WARNING, "CallGraph",
                               "Invalid CAll-Instruction Id: " << Id);
        }

        addCallEdge(CS, Fun);
      }
    }
  }

  /// A range of all functions that are vertices in thie call-graph
  [[nodiscard]] auto getAllVertexFunctions() const noexcept {
    return llvm::make_first_range(CallersOf);
  }

  /// The number of functions within this call-graph
  [[nodiscard]] size_t size() const noexcept { return CallersOf.size(); }

  [[nodiscard]] bool empty() const noexcept { return CallersOf.empty(); }

  /// Creates a JSON representation of this call-graph suitable for presistent
  /// storage.
  /// Use the ctor taking a json object for deserialization
  template <typename FunctionIdGetter, typename InstIdGetter>
  [[nodiscard]] nlohmann::json getAsJson(FunctionIdGetter GetFunctionId,
                                         InstIdGetter GetInstructionId) const {
    nlohmann::json J;

    for (const auto &[Fun, Callers] : CallersOf) {
      auto &JCallers = J[std::invoke(GetFunctionId, Fun)];

      for (const auto &CS : *Callers) {
        JCallers.push_back(std::invoke(GetInstructionId, CS));
      }
    }

    return J;
  }

private:
  [[nodiscard]] llvm::ArrayRef<f_t>
  getCalleesOfCallAtImpl(ByConstRef<n_t> Inst) const noexcept {
    if (const auto *CalleesPtr = CalleesAt.lookup(Inst)) {
      return *CalleesPtr;
    }
    return {};
  }

  [[nodiscard]] llvm::ArrayRef<n_t>
  getCallersOfImpl(ByConstRef<f_t> Fun) const noexcept {
    if (const auto *CallersPtr = CallersOf.lookup(Fun)) {
      return *CallersPtr;
    }
    return {};
  }

  // ---

  StableVector<InstructionVertexTy> InstVertexOwner;
  std::vector<FunctionVertexTy> FunVertexOwner;

  llvm::DenseMap<N, InstructionVertexTy *> CalleesAt{};
  llvm::DenseMap<F, FunctionVertexTy *> CallersOf{};
};

/// A mutable wrapper over a CallGraph. Use this to build a call-graph from
/// within your call-graph ananlysis.
template <typename N, typename F> class CallGraphBuilder {
public:
  using n_t = typename CallGraph<N, F>::n_t;
  using f_t = typename CallGraph<N, F>::f_t;
  using FunctionVertexTy = typename CallGraph<n_t, f_t>::FunctionVertexTy;
  using InstructionVertexTy = typename CallGraph<n_t, f_t>::InstructionVertexTy;

  void reserve(size_t MaxNumFunctions) {
    CG.FunVertexOwner.reserve(MaxNumFunctions);
    CG.CalleesAt.reserve(MaxNumFunctions);
    CG.CallersOf.reserve(MaxNumFunctions);
  }

  /// Registeres a new function in the call-graph. Returns a list of all
  /// call-sites that are known so far to potentially call this function.
  /// Do not manually add elements to this vector -- use addCallEdge instead.
  [[nodiscard]] FunctionVertexTy *addFunctionVertex(f_t Fun) {
    auto [It, Inserted] = CG.CallersOf.try_emplace(std::move(Fun), nullptr);
    if (Inserted) {
      auto Cap = CG.FunVertexOwner.capacity();
      assert(CG.FunVertexOwner.size() < Cap &&
             "Trying to add more than MaxNumFunctions Function Vertices");
      It->second = &CG.FunVertexOwner.emplace_back();
    }
    return It->second;
  }

  /// Registeres a new call-site in the call-graph. Returns a list of all
  /// callee functions that are known so far to potentially be called by this
  /// function.
  /// Do not manually add elements to this vector -- use addCallEdge instead.
  [[nodiscard]] InstructionVertexTy *addInstructionVertex(n_t Inst) {
    auto [It, Inserted] = CG.CalleesAt.try_emplace(std::move(Inst), nullptr);
    if (Inserted) {
      It->second = &CG.InstVertexOwner.emplace_back();
    }
    return It->second;
  }

  /// Tries to lookup the InstructionVertex for the given call-site. Returns
  /// nullptr on failure.
  [[nodiscard]] InstructionVertexTy *
  getInstVertexOrNull(ByConstRef<n_t> Inst) const noexcept {
    return CG.CalleesAt.lookup(Inst);
  }

  /// Adds a new directional edge to the call-graph indicating that CS may call
  /// Callee
  void addCallEdge(n_t CS, f_t Callee) {
    auto Vtx = addInstructionVertex(CS);
    addCallEdge(std::move(CS), Vtx, std::move(Callee));
  }

  /// Same as addCallEdge(n_t, f_t), but uses an already known
  /// InstructionVertexTy to save a lookup
  void addCallEdge(n_t CS, InstructionVertexTy *Callees, f_t Callee) {
    auto *Callers = addFunctionVertex(Callee);

    Callees->push_back(std::move(Callee));
    Callers->push_back(std::move(CS));
  }

  /// Moves the completely built call-graph out of this builder for further use.
  /// Do not use the builder after it anymore.
  [[nodiscard]] CallGraph<n_t, f_t> consumeCallGraph() noexcept {
    return std::move(CG);
  }

  /// Returns a view on the current (partial) call-graph that has already been
  /// constructed
  [[nodiscard]] const CallGraph<n_t, f_t> &viewCallGraph() const noexcept {
    return CG;
  }

private:
  CallGraph<n_t, f_t> CG{};
};
} // namespace psr

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

extern template class psr::CallGraph<const llvm::Instruction *,
                                     const llvm::Function *>;
extern template class psr::CallGraphBuilder<const llvm::Instruction *,
                                            const llvm::Function *>;

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPH_H
