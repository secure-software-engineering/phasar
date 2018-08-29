/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * InterMonoGeneralizedSolver.h
 *
 *  Created on: 15.06.2018
 *      Author: nicolas
 */

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_INTERMONOGENERALIZEDSOLVER_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_INTERMONOGENERALIZEDSOLVER_H_

#include <functional> // std::greater
#include <map>
#include <set>
#include <utility> // std::make_pair, std::pair
#include <vector>

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>
#include <phasar/PhasarLLVM/Mono/InterMonoProblem.h>

namespace psr {

/**
 *
 * @tparam IMP_temp InterMonoProblem type
 * @tparam Context type (must be a derived class of ContextBase<N, V, Context>)
 * @tparam EdgeOrdering class function that state the order of edge evaluation
 */
template <typename IMP_temp, typename Context, typename EdgeOrdering>
class InterMonoGeneralizedSolver {
public:
  using IMP_t = IMP_temp;
  using Context_t = Context;
  using Ordering_t = EdgeOrdering;

  using Node_t = typename IMP_t::Node_t;
  using Value_t = typename IMP_t::Domain_t;
  using Method_t = typename IMP_t::Method_t;
  using ICFG_t = typename IMP_t::ICFG_t;

  using analysis_t = MonoMap<Node_t, MonoMap<Context_t, Value_t>>;

private:
  void InterMonoGeneralizedSolver_check() {
    static_assert(std::is_base_of<ContextBase<Node_t, Value_t, Context_t>,
                                  Context_t>::value,
                  "Context_t must be a sub class of ContextBase<Node_t, "
                  "Value_t, Context_t>\n");
    static_assert(
        std::is_base_of<InterMonoProblem<Node_t, Value_t, Method_t, ICFG_t>,
                        IMP_t>::value,
        "IMP_t type must be a sub class of InterMonoProblem<Node_t, "
        "Value_t, Method_t, ICFG_t>\n");
  }

protected:
  using edge_t = std::pair<Node_t, Node_t>;
  using priority_t = unsigned int;
  using WorkListKey_t = std::pair<priority_t, Context_t>;
  using WorkListValue_t = std::set<edge_t, Ordering_t>;

  IMP_t &IMProblem;
  std::map<WorkListKey_t, WorkListValue_t, std::greater<WorkListKey_t>>
      Worklist;

  using WL_first_it_t = typename decltype(Worklist)::iterator;
  using WL_second_const_it_t =
      typename decltype(Worklist)::mapped_type::const_iterator;
  analysis_t Analysis;
  ICFG_t &ICFG;

  Context_t current_context;
  priority_t current_priority = 0;
  WL_first_it_t current_it_on_priority;
  WL_second_const_it_t current_it_on_edge;
  std::set<edge_t> call_edges;

  // TODO: initialize the Analysis map with different contexts
  // void initialize_with_context() {
  //   for ( const auto& seed : IMProblem.initialSeeds() ) {
  //     for ( const auto& context : seed.second ) {
  //       Analysis[seed.first][context.first]
  //         .insert(context.second.begin(), context.second.end());
  //     }
  //   } // Seeds
  // }

  void initialize() {
    for (const auto &seed : IMProblem.initialSeeds()) {
      Analysis[seed.first][current_context].insert(seed.second.begin(),
                                                   seed.second.end());
    }
  }

  virtual void analyse_function(Method_t method) {
    analyse_function(method, current_context, current_priority);
  }

  virtual void analyse_function(Method_t method, Context_t &new_context) {
    analyse_function(method, new_context, current_priority);
  }

  virtual void analyse_function(Method_t method, Context_t &new_context,
                                priority_t new_priority) {
    std::vector<edge_t> edges = ICFG.getAllControlFlowEdges(method);
    auto current_pair = std::make_pair(new_priority, new_context);
    Worklist[current_pair].insert(edges.begin(), edges.end());
  }

  bool isIntraEdge(const edge_t &edge) const {
    return ICFG.getMethodOf(edge.first) == ICFG.getMethodOf(edge.second);
  }

  bool isCallEdge(const edge_t &edge) const { return call_edges.count(edge); }

  bool isReturnEdge(const edge_t &edge) const {
    return !isIntraEdge(edge) && ICFG.isExitStmt(edge.first);
  }

  virtual void getNext() {
    // We assure before using it that WL is not empty
    current_it_on_priority = Worklist.begin();
    current_priority = current_it_on_priority->first.first;
    current_context = current_it_on_priority->first.second;
    current_it_on_edge = Worklist.cbegin()->second.cbegin();
  }

  virtual bool isWLempty() const noexcept {
    return Worklist.empty() || Worklist.cbegin()->second.empty();
  }

  virtual void eraseWL() {
    if (call_edges.count(*current_it_on_edge))
      call_edges.erase(*current_it_on_edge);

    auto &inside_set = current_it_on_priority->second;
    inside_set.erase(current_it_on_edge);

    if (inside_set.empty())
      Worklist.erase(current_it_on_priority);
  }

  virtual void insertSuccessor(Node_t dst) {
    for (auto nprimeprime : ICFG.getSuccsOf(dst)) {
      // NOTE: current_it_on_priority->second could be changed by
      //      Worklist[std::make_pair(current_priority,
      //      analysis_dst_it->first)]. That would make the context change for
      //      each context found in Analysis[dst], but there is almost 0 chance
      //      that there is more than 1 context for an intra-edge and using
      //      current_context reduce the overall number of edges inserted.
      current_it_on_priority->second.emplace(std::make_pair(dst, nprimeprime));
    }
  }

  virtual void GenerateCallEdge(Node_t dst) {
    auto key = std::make_pair(current_priority + 1, current_context);
    for (auto callee : ICFG.getCalleesOfCallAt(dst)) {
      for (auto entry_point : ICFG.getStartPointsOf(callee)) {
        auto new_edge = std::make_pair(dst, entry_point);
        Worklist[key].insert(new_edge);
        call_edges.insert(std::move(new_edge));
      } // entry-points of callee (~ 1 entry_point)
    }   // callee of method
  }

  virtual void generateExitEdge(Node_t callSite, Node_t dst,
                                Context_t &dst_context) {
    for (auto exit_point : ICFG.getExitPointsOf(ICFG.getMethodOf(dst))) {
      Worklist[std::make_pair(current_priority, dst_context)].emplace(
          std::make_pair(exit_point, callSite));
    }
  }

public:
  InterMonoGeneralizedSolver(IMP_t &IMP, Context_t &context, Method_t method)
      : IMProblem(IMP), ICFG(IMP.getICFG()), current_context(context) {
    initialize();
    analyse_function(method);
  }

  ~InterMonoGeneralizedSolver() noexcept = default;
  InterMonoGeneralizedSolver(const InterMonoGeneralizedSolver &copy) = delete;
  InterMonoGeneralizedSolver(InterMonoGeneralizedSolver &&move) = delete;
  InterMonoGeneralizedSolver &
  operator=(const InterMonoGeneralizedSolver &copy) = delete;
  InterMonoGeneralizedSolver &
  operator=(InterMonoGeneralizedSolver &&move) = delete;

  analysis_t &getAnalysisResults() { return Analysis; }

  virtual void solve() {
    while (!isWLempty()) {
      getNext();
      auto &edge = *current_it_on_edge;

      const auto &src = edge.first;
      const auto &dst = edge.second;

      Value_t Out;

      Context_t src_context(current_context);
      Context_t dst_context(src_context);

      if (isCallEdge(edge)) {
        // Handle call and call-to-ret flow
        if (!isIntraEdge(edge)) {
          Out = IMProblem.callFlow(src, ICFG.getMethodOf(dst),
                                   Analysis[src][src_context]);
        } //  !isIntraEdge(edge)
        else {
          Out = IMProblem.callToRetFlow(src, dst, Analysis[src][src_context]);
          // NB: When dealing with a callToRetFlow, we could add an edge from
          // the exit statement of the function to the successor of the call, in
          // order to to have the result of the call propagating inside the
          // function.
        } // isIntraEdge(edge)

        // Even in a call-to-ret (like recursion) the context can change
        // (e.g. called with a different set of parameters)
        dst_context.enterFunction(src, dst, Analysis[src][src_context]);
      } // isCallEdge(edge)

      else if (ICFG.isExitStmt(src)) {
        // Handle return flow
        Out = IMProblem.returnFlow(dst, ICFG.getMethodOf(src), src,
                                   Analysis[src][src_context]);
        dst_context.exitFunction(src, dst, Analysis[src][src_context]);
      } // ICFG.isExitStmt(src)
      else {
        // Handle normal flow
        Out = IMProblem.normalFlow(src, Analysis[src][src_context]);
      }

      bool dst_context_already_exist = Analysis[dst].count(dst_context);

      // If there is no context equal to dst_context already in Analysis[dst]
      // we generate one so the next loop will work.
      if (!dst_context_already_exist)
        Analysis[dst][dst_context];

      // We can have multiple context that are similar to dst_context
      // if the Comparison (in general std::less) is not strick weak order.
      // In that case, equal_range works to get every key with a similar context
      //
      // WARNING: std::set::equal_range works here but it may be a bug from this
      // version of the lib. If it breaks, we should try to use a multiset to
      // keep the analysis results.
      auto dst_range = Analysis[dst].equal_range(dst_context);
      for (auto &analysis_dst_it = dst_range.first;
           analysis_dst_it != dst_range.second; ++analysis_dst_it) {
        // flowfactsstabilized = true <-> Same set & already visited once
        bool flowfactsstabilized =
            dst_context_already_exist
                ? IMProblem.sqSubSetEqual(Out, analysis_dst_it->second)
                : false;

        if (!flowfactsstabilized) {
          analysis_dst_it->second =
              IMProblem.join(analysis_dst_it->second, Out);

          if (isIntraEdge(edge)) {
            insertSuccessor(dst);
          }
        } // unstabilized flow fact

        if (isIntraEdge(edge)) {
          if (ICFG.isCallStmt(dst)) {
            // The dst is a call stmt, we generate a call edge from the dst node
            // to the entry points of the callee function
            GenerateCallEdge(dst);
          }
        } // Intra edge

        if (isCallEdge(edge)) {
          if (!flowfactsstabilized || dst_context.isUnsure() ||
              IMProblem.recompute(ICFG.getMethodOf(dst))) {
            // We never computed the function or the flow facts have changed or
            // in case we want to handle some side-effects, the context does not
            // assured perfect equality or any reason we would want to restart
            // the computation of the function
            // WARNING: Allowing recomputation can generate infinite recursion,
            // only activate it if your sure

            analyse_function(ICFG.getMethodOf(dst), dst_context);
          } // Compute a call
          // Computed or not, we called a callFlow or callToRetFlow so we
          // generate the exit edges to call the corresponding RetFlow

          generateExitEdge(src, dst, dst_context);
        } // Is a call edge

        // Nothing to do in particular if an Exit statement
      }

      eraseWL();
    } // WL not empty
  }
};

} // namespace psr

#endif
