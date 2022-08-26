#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/LLVMPathConstraints.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/Z3BasedPathSensitvityManager.h"

namespace psr {
z3::expr Z3BasedPathSensitivityManagerBase::filterOutUnreachableNodes(
    graph_type &RevDAG, const Z3BasedPathSensitivityConfig &Config,
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

  auto doFilter = [&Ctx, &RevDAG, &LPC](auto &doFilter,
                                        unsigned Vtx) -> z3::expr {
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
    llvm::SmallVector<unsigned> ToRemove;

    unsigned Idx = 0;
    for (auto Edge : graph_traits_t::outEdges(RevDAG, Vtx)) {
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
        ToRemove.push_back(Idx);
      } else {
        Ys.push_back(std::move(Y));
      }

      Ctx.Solver.pop();
      Idx++;
    }

    Ctx.Ctr += ToRemove.size();

    graph_traits_t::removeEdges(RevDAG, Vtx, ToRemove);
    // RevDAG.Adj[Vtx].erase(remove_by_index(RevDAG.Adj[Vtx], ToRemove),
    //                       RevDAG.Adj[Vtx].end());
    if (graph_traits_t::outDegree(RevDAG, Vtx) == 0) {
      return Ctx.NodeConstraints[Vtx] = Vtx == 0 ? X : Ctx.False;
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

  for (unsigned I = 0; I < RevDAG.Roots.size(); ++I) {
    auto Rt = RevDAG.Roots[I];
    auto Res = doFilter(doFilter, Rt);
    Ret = Ret || Res;
    if (Rt != RevDAG.Leaf && RevDAG.Adj[Rt].empty()) {
      std::swap(RevDAG.Roots[I], RevDAG.Roots.back());
      RevDAG.Roots.pop_back();
      --I;
    }
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "> Filtered out " << Ctx.Ctr << " edges from the DAG");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << ">> " << (TotalNumEdges - Ctx.Ctr) << " edges remaining");

  return Ret.simplify();
}
} // namespace psr
