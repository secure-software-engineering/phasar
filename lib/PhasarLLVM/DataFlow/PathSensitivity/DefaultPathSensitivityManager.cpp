#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/DefaultPathSensitivityManager.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include "PathFilter.h"

#include <cstdint>

namespace psr {

namespace {

struct PathLengthFilter {
  using n_t = const llvm::Instruction *;

  void saveState() { Lengths.push_back(CurrLength); }
  void restoreState() { CurrLength = Lengths.pop_back_val(); }
  void saveEdge(n_t /*Prev*/, n_t /*Inst*/) { CurrLength++; }
  [[nodiscard]] bool isValid() const { return CurrLength <= MaxLength; }
  bool saveFinalEdge(n_t /*Prev*/, n_t /*FinalInst*/) {
    CurrLength++;
    return isValid();
  }

  size_t MaxLength{};
  size_t CurrLength{};
  llvm::SmallVector<size_t> Lengths{};
};

struct UnrollFilter {
  using n_t = const llvm::Instruction *;

  struct State {
    llvm::DenseMap<n_t, size_t> BrCount{};
    bool Valid = true;
  };

  void saveState() {
    if (MaxUnroll == SIZE_MAX) {
      return;
    }
    // Note: Beware the reallocation!
    auto Init = RestoreStack.back();
    RestoreStack.push_back(std ::move(Init));
  }

  void restoreState() {
    if (MaxUnroll == SIZE_MAX) {
      return;
    }
    RestoreStack.pop_back();
  }

  void saveEdge(n_t Prev, n_t /*Inst*/) {
    if (MaxUnroll == SIZE_MAX) {
      return;
    }

    if (const auto *Br = llvm::dyn_cast<llvm::BranchInst>(Prev);
        Br && Br->isUnconditional()) {
      auto Count = ++RestoreStack.back().BrCount[Br];
      if (Count > MaxUnroll) {
        RestoreStack.back().Valid = false;
      }
    }
  }

  [[nodiscard]] bool isValid() const {
    return MaxUnroll == SIZE_MAX || RestoreStack.back().Valid;
  }

  bool saveFinalEdge(n_t /*Prev*/, n_t /*FinalInst*/) { return isValid(); }

  size_t MaxUnroll{};

  // TODO: Optimize!!!

  llvm::SmallVector<State> RestoreStack = {State{}};
};

} // namespace

auto DefaultPathSensitivityManagerBase::filterAndFlattenRevDag(
    graph_type &RevDAG, vertex_t Leaf, n_t FinalInst,
    const PathSensitivityConfig &Config) const -> DefaultFlowPathSequence<n_t> {
  /// Here, we do the following:
  /// - Traversing the ReverseDAG in a simple DFS order and maintaining the
  ///   exact path reaching the current node.
  /// - On the fly constructing and updating a call-stack to regain
  ///   context-sensitivity by filtering out paths with invalid returns
  /// - Similarly on the fly constructing and solving Z3 Path Constraints and
  ///   filtering out all paths with unsatisfiable constraints
  /// - Saving all "surviving" paths that end at a leaf to the overall vector
  ///   that gets returned at the end

  DefaultFlowPathSequence<n_t> Ret;
  size_t CompletedCtr = 0;
  auto Filters = makePathFilterList(PathLengthFilter{Config.MaxPathLength},
                                    CallStackPathFilter{},
                                    UnrollFilter{Config.MaxUnrollFactor});

  llvm::SmallVector<n_t, 0> CurrPath;

  n_t Prev = nullptr;

  auto DoFilter = [FinalInst, &Prev, &Filters, &RevDAG, &CurrPath, &Ret,
                   &CompletedCtr, MaxNumPaths{Config.NumPathsThreshold},
                   Leaf](auto &DoFilter, vertex_t Vtx) {
    auto CurrPathSave = CurrPath.size();
    scope_exit RestoreCurrPath = [&CurrPath, CurrPathSave] {
      assert(CurrPathSave <= CurrPath.size());
      CurrPath.resize(CurrPathSave);
    };

    const auto *PrevSave = Prev;
    scope_exit RestorePrev = [PrevSave, &Prev] { Prev = PrevSave; };

    Filters.saveState();
    scope_exit RestoreFilters = [&Filters] { Filters.restoreState(); };

    for (const auto *Inst : llvm::reverse(graph_traits_t::node(RevDAG, Vtx))) {
      CurrPath.push_back(Inst);
      if (!Prev) {
        Prev = Inst;
        continue;
      }

      Filters.saveEdge(Prev, Inst);

      Prev = Inst;
    }

    if (Vtx == Leaf) {
      // llvm::errs() << "> Reached Leaf!\n";

      assert(!CurrPath.empty() && "Reported paths must not be empty!");

      /// Reached the end
      /// TODO: No need to add the final inst separately anymore. Now, it
      /// has its own PathNode and is handled implicitly
      if (Filters.saveFinalEdge(Prev, FinalInst)) {
        Ret.emplace_back(CurrPath);
        ++CompletedCtr;
      }
      // else {
      //   llvm::errs() << "Filters.saveFinalEdge(Prev, FinalInst) ==
      //   false:\n"; for (const auto *Inst : CurrPath) {
      //     llvm::errs() << "  " << llvmIRToString(Inst) << '\n';
      //   }
      //   llvm::errs() << '\n';
      // }

      return;
    }
    if (graph_traits_t::outDegree(RevDAG, Vtx) == 0) {
      llvm::report_fatal_error("Non-leaf node has no successors!");
    }
    if (CompletedCtr >= MaxNumPaths) {
      return;
    }
    if (!Filters.isValid()) {
      // llvm::errs() << "Filters.isValid() == false:\n";
      // for (const auto *Inst : CurrPath) {
      //   llvm::errs() << "  " << llvmIRToString(Inst) << '\n';
      // }
      // llvm::errs() << '\n';

      return;
    }
    /// TODO: Verify that we have no concurrent modification here and the
    /// iterator is never dangling!
    for (auto Edge : graph_traits_t::outEdges(RevDAG, Vtx)) {
      DoFilter(DoFilter, graph_traits_t::target(Edge));
    }
  };

  for (auto Rt : graph_traits_t::roots(RevDAG)) {
    DoFilter(DoFilter, Rt);
  }

  return Ret;
}

void DefaultPathSensitivityManagerBase::deduplicatePaths(
    DefaultFlowPathSequence<n_t> &Paths) {
  /// Some kind of lexical sort for being able to deduplicate the paths easily
  std::sort(
      Paths.begin(), Paths.end(),
      [](const DefaultFlowPath<n_t> &LHS, const DefaultFlowPath<n_t> &RHS) {
        return LHS.size() < RHS.size() ||
               (LHS.size() == RHS.size() &&
                std::lexicographical_compare(LHS.begin(), LHS.end(),
                                             RHS.begin(), RHS.end()));
      });

  Paths.erase(std::unique(Paths.begin(), Paths.end()), Paths.end());
}

} // namespace psr
