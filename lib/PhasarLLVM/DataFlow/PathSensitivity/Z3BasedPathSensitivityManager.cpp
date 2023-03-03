#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/LLVMPathConstraints.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/Z3BasedPathSensitvityManager.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"

namespace psr {
z3::expr Z3BasedPathSensitivityManagerBase::filterOutUnreachableNodes(
    graph_type &RevDAG, vertex_t Leaf,
    const Z3BasedPathSensitivityConfig &Config,
    LLVMPathConstraints &LPC) const {

  struct FilterContext {
    llvm::BitVector Visited{};
    z3::expr True;
    z3::expr False;
    llvm::SmallVector<z3::expr, 0> NodeConstraints{};
    size_t Ctr = 0;
    z3::solver Solver;

    explicit FilterContext(z3::context &Z3Ctx)
        : True(Z3Ctx.bool_val(true)), False(Z3Ctx.bool_val(false)),
          Solver(Z3Ctx) {}
  } Ctx(LPC.getContext());

  Ctx.Visited.resize(graph_traits_t::size(RevDAG));
  Ctx.NodeConstraints.resize(graph_traits_t::size(RevDAG), Ctx.True);

  size_t TotalNumEdges = 0;
  for (auto I : graph_traits_t::vertices(RevDAG)) {
    TotalNumEdges += graph_traits_t::outDegree(RevDAG, I);
  }

  if (Config.AdditionalConstraint) {
    Ctx.Solver.add(*Config.AdditionalConstraint);
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  auto doFilter = [&Ctx, &RevDAG, &LPC, Leaf](auto &doFilter,
                                              vertex_t Vtx) -> z3::expr {
    Ctx.Visited.set(Vtx);
    z3::expr X = Ctx.True;
    llvm::ArrayRef<n_t> PartialPath = graph_traits_t::node(RevDAG, Vtx);
    assert(!PartialPath.empty());

    for (size_t I = PartialPath.size() - 1; I; --I) {
      if (auto Constr =
              LPC.getConstraintFromEdge(PartialPath[I], PartialPath[I - 1])) {
        X = X && *Constr;
      }
    }

    llvm::SmallVector<z3::expr> Ys;

    for (auto Iter = graph_traits_t::outEdges(RevDAG, Vtx).begin();
         Iter != graph_traits_t::outEdges(RevDAG, Vtx).end();) {
      // NOLINTNEXTLINE(readability-qualified-auto, llvm-qualified-auto)
      auto It = Iter++;
      auto Edge = *It;
      auto Adj = graph_traits_t::target(Edge);
      if (!Ctx.Visited.test(Adj)) {
        doFilter(doFilter, Adj);
      }
      Ctx.Solver.push();
      Ctx.Solver.add(X);
      auto Y = Ctx.NodeConstraints[Adj];
      const auto &AdjPP = graph_traits_t::node(RevDAG, Adj);
      assert(!AdjPP.empty());
      if (auto Constr =
              LPC.getConstraintFromEdge(PartialPath.front(), AdjPP.back())) {
        Y = Y && *Constr;
      }

      Ctx.Solver.add(Y);

      auto Sat = Ctx.Solver.check();
      if (Sat == z3::check_result::unsat) {
        // llvm::errs() << "> Unsat: " << Ctx.Solver.to_smt2() << '\n';
        // llvm::errs() << ">> With X: " << X.to_string() << '\n';
        // llvm::errs() << ">> With Y: " << Y.to_string() << '\n';
        // llvm::errs() << ">> With NodeConstraints[" << Adj
        //              << "]: " << Ctx.NodeConstraints[Adj].to_string() <<
        //              '\n';
        // if (auto Constr =
        //         LPC.getConstraintFromEdge(PartialPath.front(), AdjPP.back()))
        //         {
        //   llvm::errs() << ">> With EdgeConstraint: " << Constr->to_string()
        //                << '\n';
        // } else {
        //   llvm::errs() << ">> Without EdgeConstraint\n";
        // }

        Iter = graph_traits_t::removeEdge(RevDAG, Vtx, It);
        Ctx.Ctr++;
      } else {
        Ys.push_back(std::move(Y));
      }

      Ctx.Solver.pop();
      // Idx++;
    }

    if (graph_traits_t::outDegree(RevDAG, Vtx) == 0) {
      return Ctx.NodeConstraints[Vtx] = Vtx == Leaf ? X : Ctx.False;
    }
    if (Ys.empty()) {
      llvm_unreachable("Adj nonempty and Ys empty is unexpected");
    }
    auto Y = Ys[0];
    for (const auto &Constr : llvm::makeArrayRef(Ys).drop_front()) {
      Y = Y || Constr;
    }
    return Ctx.NodeConstraints[Vtx] = (X && Y).simplify();
  };

  z3::expr Ret = LPC.getContext().bool_val(false);

  for (auto Iter = graph_traits_t::roots(RevDAG).begin();
       Iter != graph_traits_t::roots(RevDAG).end();) {
    // NOLINTNEXTLINE(readability-qualified-auto, llvm-qualified-auto)
    auto It = Iter++;
    auto Rt = *It;
    auto Res = doFilter(doFilter, Rt);
    Ret = Ret || Res;
    if (Rt != Leaf && RevDAG.Adj[Rt].empty()) {
      // llvm::errs() << "> Remove root " << Rt << "\n";
      Iter = graph_traits_t::removeRoot(RevDAG, It);
    }
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                       "> Filtered out " << Ctx.Ctr << " edges from the DAG");
  PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                       ">> " << (TotalNumEdges - Ctx.Ctr)
                             << " edges remaining");

  return Ret.simplify();
}

/// This could be a C++20 concept!
struct PathFilter {
  // void saveState();
  // void restoreState();
  // void saveEdge(n_t Prev, n_t Inst);
  // bool isValid() const;
  // bool saveFinalEdge(n_t Prev, n_t FinalInst);
};

template <typename... T> class PathFilterList : public std::tuple<T...> {
public:
  using n_t = const llvm::Instruction *;
  using std::tuple<T...>::tuple;

  void saveState() { saveStateImpl(std::make_index_sequence<sizeof...(T)>()); }

  void restoreState() {
    restoreStateImpl(std::make_index_sequence<sizeof...(T)>());
  }

  void saveEdge(n_t Prev, n_t Inst) {
    saveEdgeImpl(Prev, Inst, std::make_index_sequence<sizeof...(T)>());
  }

  [[nodiscard]] bool isValid() {
    return isValidImpl(std::make_index_sequence<sizeof...(T)>());
  }

  bool saveFinalEdge(n_t Prev, n_t Inst) {
    return saveFinalEdgeImpl(Prev, Inst,
                             std::make_index_sequence<sizeof...(T)>());
  }

private:
  template <size_t... I>
  void saveStateImpl(std::index_sequence<I...> /*unused*/) {
    (std::get<I>(*this).saveState(), ...);
  }
  template <size_t... I>
  void restoreStateImpl(std::index_sequence<I...> /*unused*/) {
    (std::get<I>(*this).restoreState(), ...);
  }
  template <size_t... I>
  void saveEdgeImpl(n_t Prev, n_t Inst, std::index_sequence<I...> /*unused*/) {
    (std::get<I>(*this).saveEdge(Prev, Inst), ...);
  }
  template <size_t... I>
  bool saveFinalEdgeImpl(n_t Prev, n_t Inst,
                         std::index_sequence<I...> /*unused*/) {
    return (std::get<I>(*this).saveFinalEdge(Prev, Inst) && ...);
  }
  template <size_t... I>
  bool isValidImpl(std::index_sequence<I...> /*unused*/) {
    return (std::get<I>(*this).isValid() && ...);
  }
};

template <typename... T>
static PathFilterList<T...> makePathFilterList(T &&...Filters) {
  return PathFilterList<T...>(std::forward<T>(Filters)...);
}

class CallStackPathFilter {
public:
  using n_t = const llvm::Instruction *;
  void saveState() { CallStackSafe.emplace_back(CallStackOwner.size(), TOS); }

  void restoreState() {
    auto [SaveCallStackSize, CSSave] = CallStackSafe.pop_back_val();
    assert(CallStackOwner.size() >= SaveCallStackSize);
    CallStackOwner.pop_back_n(CallStackOwner.size() - SaveCallStackSize);
    TOS = CSSave;
    Valid = true;
  }

  void saveEdge(n_t Prev, n_t Inst) {
    if (!Valid) {
      return;
    }
    if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(Prev);
        CS && !isDirectSuccessorOf(Inst, CS)) {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                           "Push CS: " << llvmIRToString(CS));
      pushCS(CS);

    } else if (llvm::isa<llvm::ReturnInst>(Prev) ||
               llvm::isa<llvm::ResumeInst>(Prev)) {
      /// Allow unbalanced returns
      if (!emptyCS()) {
        const auto *CS = popCS();

        PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                             "Pop CS: " << llvmIRToString(CS) << " at exit "
                                        << llvmIRToString(Prev)
                                        << " and ret-site "
                                        << llvmIRToString(Inst));

        if (!isDirectSuccessorOf(Inst, CS)) {
          /// Invalid return
          Valid = false;

          PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                               "> Invalid return");
        } else {
          PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter", "> Valid return");
        }
      } else {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                             "> Unbalanced return at exit "
                                 << llvmIRToString(Prev));
      }
      /// else: unbalanced return
    }
  }

  bool saveFinalEdge(n_t Prev, n_t FinalInst) {
    if (!Valid) {
      return false;
    }

    if (Prev && (llvm::isa<llvm::ReturnInst>(Prev) ||
                 llvm::isa<llvm::ResumeInst>(Prev))) {
      if (!emptyCS() && !isDirectSuccessorOf(FinalInst, popCS())) {
        /// Invalid return
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] bool isValid() const noexcept { return Valid; }

private:
  bool isDirectSuccessorOf(const llvm::Instruction *Succ,
                           const llvm::Instruction *Of) {

    while (const auto *Nxt = Of->getNextNode()) {
      if (Nxt == Succ) {
        return true;
      }
      Of = Nxt;

      if (Nxt->isTerminator()) {
        break;
      }
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Nxt);
          Call && Call->getCalledFunction() &&
          !Call->getCalledFunction()->isDeclaration()) {
        /// Don't skip function calls. We might call the same fun twice in the
        /// same BB, so we recognize invalid paths there as well!
        return false;
      }
    }

    assert(Of->isTerminator());

    return std::find_if(llvm::succ_begin(Of), llvm::succ_end(Of),
                        [&Succ](const llvm::BasicBlock *BB) {
                          return &BB->front() == Succ;
                        }) != llvm::succ_end(Of);
  }

  void pushCS(const llvm::CallBase *CS) {
    auto *NewNode = &CallStackOwner.emplace_back();
    NewNode->Prev = TOS;
    NewNode->CS = CS;
    TOS = NewNode;
  }

  const llvm::CallBase *popCS() noexcept {
    assert(TOS && "We should already have checked for nullness...");
    const auto *Ret = TOS->CS;
    TOS = TOS->Prev;
    /// Defer the deallocation, such that we still can rollback
    return Ret;
  }

  [[nodiscard]] bool emptyCS() const noexcept { return TOS == nullptr; }

  struct CallStackNode {
    CallStackNode *Prev = nullptr;
    const llvm::CallBase *CS = nullptr;
  };
  /// Now, filter directly on the reversed DAG

  StableVector<CallStackNode> CallStackOwner;
  llvm::SmallVector<std::pair<size_t, CallStackNode *>> CallStackSafe;
  CallStackNode *TOS = nullptr;
  bool Valid = true;
};

class ConstraintPathFilter {
public:
  using n_t = const llvm::Instruction *;

  ConstraintPathFilter(LLVMPathConstraints &LPC,
                       const z3::expr &AdditionalConstraint,
                       size_t *CompletedCtr) noexcept
      : LPC(LPC), Solver(LPC.getContext()), CompletedCtr(*CompletedCtr) {
    Solver.add(AdditionalConstraint);
    Solver.push();
    NumAtomsStack.push_back(0);
  }

  void saveState() {
    NumAtomsStack.push_back(NumAtomsStack.back());
    Solver.push();
  }

  void restoreState() {
    Solver.pop();
    auto NumAtoms = NumAtomsStack.pop_back_val();
    assert(!NumAtomsStack.empty());
    auto Diff = NumAtoms - NumAtomsStack.back();
    for (size_t I = 0; I < Diff; ++I) {
      SymbolicAtoms.pop_back();
    }
    NeedSolverInvocation = false;
    LocalAtoms.clear();
    Model = std::nullopt;
  }

  void saveEdge(n_t Prev, n_t Inst) {

    if (auto ConstrAndVariables =
            LPC.getConstraintAndVariablesFromEdge(Prev, Inst)) {

      Solver.add(ConstrAndVariables->Constraint);

      LocalAtoms.append(ConstrAndVariables->Variables);
    }
  }

  [[nodiscard]] bool isValid() {
    ++IsValidCalls;
    if (LocalAtoms.empty() && !NeedSolverInvocation) {
      /// Nothing (new) to check
      return true;
    }

    if (!checkLocalAtomsOverlap()) {
      return true;
    }

    NeedSolverInvocation = false;

    auto Res = Solver.check();
    ++Ctr;
#ifdef DYNAMIC_LOG
    if (Ctr % 10000 == 0) {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                           Ctr << " solver invocations so far...");
      PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                           ">> " << IsValidCalls << " calls to isValid()");
      PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                           ">> " << RejectedCtr << " paths rejected");
      PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                           ">> " << CompletedCtr << " paths completed");
    }
#endif

    auto Ret = Res != z3::check_result::unsat;
    if (!Ret) {
      ++RejectedCtr;
    } else {
      Model = Solver.get_model();
    }

    return Ret;
  }

  bool saveFinalEdge(n_t Prev, n_t FinalInst) {
    saveEdge(Prev, FinalInst);
    NeedSolverInvocation = true;
    return isValid();
  }

  z3::expr getPathConstraints() {
    auto Vec = Solver.assertions();
    if (Vec.empty()) {
      return LPC.getContext().bool_val(true);
    }

    auto Ret = Vec[0];
    for (int I = 1, End = int(Vec.size()); I != End; ++I) {
      Ret = Ret && Vec[I];
    }
    return Ret.simplify();
  }

  z3::model getModel() { return Model.value(); }

  [[nodiscard]] size_t getNumSolverInvocations() const noexcept { return Ctr; }

private:
  [[nodiscard]] bool checkLocalAtomsOverlap() {
    if (NeedSolverInvocation) {
      return true;
    }

    std::sort(LocalAtoms.begin(), LocalAtoms.end());
    LocalAtoms.erase(std::unique(LocalAtoms.begin(), LocalAtoms.end()),
                     LocalAtoms.end());
    size_t NumLocalAtoms = LocalAtoms.size();
    size_t OldSize = SymbolicAtoms.size();
    SymbolicAtoms.insert(LocalAtoms.begin(), LocalAtoms.end());
    size_t NewSize = SymbolicAtoms.size();
    LocalAtoms.clear();
    /// The newly added atoms don't overlap with the atoms from the
    /// already seen constraints. So, there can be no contradicion (would
    /// have already been filtered out in the previous step)
    return NewSize - OldSize != NumLocalAtoms;
  }

  LLVMPathConstraints &LPC;
  z3::solver Solver;
  llvm::SmallSetVector<const llvm::Value *, 8> SymbolicAtoms;
  llvm::SmallVector<unsigned> NumAtomsStack;
  llvm::SmallVector<const llvm::Value *> LocalAtoms;
  std::optional<z3::model> Model;
  bool NeedSolverInvocation = false;

  size_t Ctr = 0;
  size_t RejectedCtr = 0;
  size_t IsValidCalls = 0;
  size_t &CompletedCtr;
};

auto Z3BasedPathSensitivityManagerBase::filterAndFlattenRevDag(
    graph_type &RevDAG, vertex_t Leaf, n_t FinalInst,
    const Z3BasedPathSensitivityConfig &Config, LLVMPathConstraints &LPC) const
    -> FlowPathSequence<n_t> {
  /// Here, we do the following:
  /// - Traversing the ReverseDAG in a simple DFS order and maintaining the
  ///   exact path reaching the current node.
  /// - On the fly constructing and updating a call-stack to regain
  ///   context-sensitivity by filtering out paths with invalid returns
  /// - Similarly on the fly constructing and solving Z3 Path Constraints and
  ///   filtering out all paths with unsatisfiable constraints
  /// - Saving all "surviving" paths that end at a leaf to the overall vector
  ///   that gets returned at the end

  /// Problem: We still have way too many Z3 solver invocations (> 900000 for
  /// all teatcases)
  /// Solution idea: In contrast to the context sensitivity check, the Path
  /// constraints are context-independent. So, it might be beneficial to
  /// compute the end-reachability constraints of each _node_ in a bottom-up
  /// fashion leading to PathNodeOwner.size() solver invocations. We then
  /// still need a subsequent DFS order traversal to collect all remaining
  /// satisfiable paths, so there we can still apply the context-sensitivity
  /// check OTF.
  /// NOTE: This is implemented now in filterOutUnreachableNodes()

  FlowPathSequence<n_t> Ret;
  size_t CompletedCtr = 0;
  auto Filters = makePathFilterList(
      CallStackPathFilter{},
      ConstraintPathFilter{
          LPC,
          Config.AdditionalConstraint.value_or(LPC.getContext().bool_val(true)),
          &CompletedCtr});

  llvm::SmallVector<n_t, 0> CurrPath;

  n_t Prev = nullptr;

  auto doFilter = [FinalInst, &Prev, &Filters, &RevDAG, &CurrPath, &Ret,
                   &CompletedCtr, MaxNumPaths{Config.NumPathsThreshold},
                   Leaf](auto &doFilter, vertex_t Vtx) {
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
        auto Model = std::get<1>(Filters).getModel();
        Ret.emplace_back(CurrPath, std::get<1>(Filters).getPathConstraints(),
                         Model);
        ++CompletedCtr;
      }

      return;
    }
    if (graph_traits_t::outDegree(RevDAG, Vtx) == 0) {
      llvm::report_fatal_error("Non-leaf node has no successors!");
    }

    if (CompletedCtr >= MaxNumPaths) {
      return;
    }
    if (!Filters.isValid()) {
      return;
    }
    /// TODO: Verify that we have no concurrent modification here and the
    /// iterator is never dangling!
    for (auto Edge : graph_traits_t::outEdges(RevDAG, Vtx)) {
      doFilter(doFilter, graph_traits_t::target(Edge));
    }
  };

  for (auto Rt : graph_traits_t::roots(RevDAG)) {
    doFilter(doFilter, Rt);
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                       "Num Solver invocations: "
                           << std::get<1>(Filters).getNumSolverInvocations());

  return Ret;
}

void Z3BasedPathSensitivityManagerBase::deduplicatePaths(
    FlowPathSequence<n_t> &Paths) {
  /// Some kind of lexical sort for being able to deduplicate the paths easily
  std::sort(Paths.begin(), Paths.end(),
            [](const FlowPath<n_t> &LHS, const FlowPath<n_t> &RHS) {
              return LHS.size() < RHS.size() ||
                     (LHS.size() == RHS.size() &&
                      std::lexicographical_compare(LHS.begin(), LHS.end(),
                                                   RHS.begin(), RHS.end()));
            });

  Paths.erase(std::unique(Paths.begin(), Paths.end()), Paths.end());
}

} // namespace psr
