/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPH_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPH_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/StableVector.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"

#include "nlohmann/json.hpp"

#include <functional>
#include <vector>

namespace psr {
template <typename N, typename F> class CallGraphBuilder;
template <typename N, typename F> class CallGraph {
  friend class CallGraphBuilder<N, F>;

public:
  using FunctionVertexTy = llvm::SmallVector<N>;
  using InstructionVertexTy = llvm::SmallVector<F>;

  CallGraph() noexcept = default;

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

  [[nodiscard]] llvm::ArrayRef<F>
  getCalleesOfCallAt(ByConstRef<N> Inst) const noexcept {
    const auto *CalleesPtr = CalleesAt.lookup(Inst);
    return CalleesPtr ? *CalleesPtr : llvm::ArrayRef<F>();
  }

  [[nodiscard]] llvm::ArrayRef<N>
  getCallersOf(ByConstRef<F> Fun) const noexcept {
    const auto *CallersPtr = CallersOf.lookup(Fun);
    return CallersPtr ? *CallersPtr : llvm::ArrayRef<F>();
  }

  [[nodiscard]] auto getAllVertexFunctions() const noexcept {
    return llvm::make_first_range(CallersOf);
  }
  [[nodiscard]] size_t size() const noexcept { return CallersOf.size(); }
  [[nodiscard]] bool empty() const noexcept { return CallersOf.empty(); }

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
  StableVector<InstructionVertexTy> InstVertexOwner{};
  std::vector<FunctionVertexTy> FunVertexOwner{};

  llvm::DenseMap<N, InstructionVertexTy *> CalleesAt{};
  llvm::DenseMap<F, FunctionVertexTy *> CallersOf{};
};

template <typename N, typename F> class CallGraphBuilder {
public:
  using FunctionVertexTy = typename CallGraph<N, F>::FunctionVertexTy;
  using InstructionVertexTy = typename CallGraph<N, F>::InstructionVertexTy;

  explicit CallGraphBuilder(size_t MaxNumFunctions) {
    CG.FunVertexOwner.reserve(MaxNumFunctions);
    CG.CalleesAt.reserve(MaxNumFunctions);
    CG.CallersOf.reserve(MaxNumFunctions);
  }

  [[nodiscard]] FunctionVertexTy *addFunctionVertex(F Fun) {
    auto [It, Inserted] = CG.CallersOf.try_emplace(std::move(Fun), nullptr);
    if (Inserted) {
      auto Cap = CG.FunVertexOwner.capacity();
      assert(CG.FunVertexOwner.size() < Cap &&
             "Trying to add more than MaxNumFunctions Function Vertices");
      It->second = &CG.FunVertexOwner.emplace_back();
    }
    return It->second;
  }

  [[nodiscard]] InstructionVertexTy *addInstructionVertex(N Inst) {
    auto [It, Inserted] = CG.CalleesAt.try_emplace(std::move(Inst), nullptr);
    if (Inserted) {
      It->second = &CG.InstVertexOwner.emplace_back();
    }
    return It->second;
  }

  void addCallEdge(N CS, F Callee) {
    auto Vtx = addInstructionVertex(CS);
    addCallEdge(std::move(CS), Vtx, std::move(Callee));
  }

  void addCallEdge(N CS, InstructionVertexTy *Callees, F Callee) {
    auto *Callers = addFunctionVertex(Callee);

    Callees->push_back(std::move(Callee));
    Callers->push_back(std::move(CS));
  }

  [[nodiscard]] CallGraph<N, F> consumeCallGraph() noexcept {
    return std::move(CG);
  }

  [[nodiscard]] const CallGraph<N, F> &viewCallGraph() const noexcept {
    return CG;
  }

private:
  CallGraph<N, F> CG{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPH_H
