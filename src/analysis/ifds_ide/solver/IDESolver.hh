/*
 * IDESolver.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_IDESOLVER_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_IDESOLVER_HH_

#include "../../../lib/LLVMShorthands.hh"
#include "../../../utils/Logger.hh"
#include "../../../utils/Table.hh"
#include "../EdgeFunction.hh"
#include "../EdgeFunctions.hh"
#include "../FlowEdgeFunctionCache.hh"
#include "../FlowFunctions.hh"
#include "../IDETabulationProblem.hh"
#include "../JoinLattice.hh"
#include "../ZeroedFlowFunction.hh"
#include "../edge_func/EdgeIdentity.hh"
#include "../solver/JumpFunctions.hh"
#include "JoinHandlingNode.hh"
#include "LinkedNode.hh"
#include "PathEdge.hh"
#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>

using namespace std;

/**
 * Solves the given IDETabulationProblem as described in the 1996 paper by
 * Sagiv, Horwitz and Reps. To solve the problem, call solve(). Results
 * can then be queried by using resultAt() and resultsAt().
 *
 * @param <N> The type of nodes in the interprocedural control-flow graph.
 * @param <D> The type of data-flow facts to be computed by the tabulation
 * problem.
 * @param <M> The type of objects used to represent methods.
 * @param <V> The type of values to be computed along flow edges.
 * @param <I> The type of inter-procedural control-flow graph being used.
 */
template <class N, class D, class M, class V, class I> class IDESolver {
public:
  IDESolver(IDETabulationProblem<N, D, M, V, I> &tabulationProblem)
      : ideTabulationProblem(tabulationProblem),
        cachedFlowEdgeFunctions(tabulationProblem),
        recordEdges(tabulationProblem.solver_config.recordEdges),
        zeroValue(tabulationProblem.zeroValue()),
        icfg(tabulationProblem.interproceduralCFG()),
        computevalues(tabulationProblem.solver_config.computeValues),
        autoAddZero(tabulationProblem.solver_config.autoAddZero),
        followReturnPastSeeds(
            tabulationProblem.solver_config.followReturnsPastSeeds),
        computePersistedSummaries(
            tabulationProblem.solver_config.computePersistedSummaries),
        allTop(tabulationProblem.allTopFunction()),
        jumpFn(make_shared<JumpFunctions<N, D, V>>(allTop)),
        initialSeeds(tabulationProblem.initialSeeds()) {
    cout << "called IDESolver ctor" << endl;
    cout << tabulationProblem.solver_config << endl;
  }

  IDESolver(IDETabulationProblem<N, D, M, V, I> &&tabulationProblem)
      : ideTabulationProblem(tabulationProblem),
        cachedFlowEdgeFunctions(tabulationProblem),
        recordEdges(tabulationProblem.solver_config.recordEdges),
        zeroValue(tabulationProblem.zeroValue()),
        icfg(tabulationProblem.interproceduralCFG()),
        computevalues(tabulationProblem.solver_config.computeValues),
        autoAddZero(tabulationProblem.solver_config.autoAddZero),
        followReturnPastSeeds(
            tabulationProblem.solver_config.followReturnsPastSeeds),
        computePersistedSummaries(
            tabulationProblem.solver_config.computePersistedSummaries),
        allTop(tabulationProblem.allTopFunction()),
        jumpFn(make_shared<JumpFunctions<N, D, V>>(allTop)),
        initialSeeds(tabulationProblem.initialSeeds()) {
    cout << "called IDESolver ctor" << endl;
    cout << tabulationProblem.solver_config << endl;
  }

  virtual ~IDESolver() = default;

  /**
   * @brief Runs the solver on the configured problem. This can take some time.
   */
  virtual void solve() {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, INFO) << "IDE solver is solving the specified problem";
    // computations starting here
    auto startFlowFunctionConstruction = chrono::high_resolution_clock::now();
    // We start our analysis and construct exploded supergraph
    BOOST_LOG_SEV(lg, INFO)
        << "Submit initial seeds, construct exploded super graph";
    submitInitalSeeds();
    auto endFlowFunctionConstruction = chrono::high_resolution_clock::now();
    durationFlowFunctionConstruction =
        chrono::duration_cast<chrono::milliseconds>(
            endFlowFunctionConstruction - startFlowFunctionConstruction);
    if (computevalues) {
      auto startFlowFunctionApplication = chrono::high_resolution_clock::now();
      // Computing the final values for the edge functions
      BOOST_LOG_SEV(lg, INFO)
          << "Compute the final values according to the edge functions";
      computeValues();
      auto endFlowFunctionApplication = chrono::high_resolution_clock::now();
      durationFlowFunctionApplication =
          chrono::duration_cast<chrono::milliseconds>(
              endFlowFunctionApplication - startFlowFunctionApplication);
    }
    BOOST_LOG_SEV(lg, INFO) << "Problems solved";
    BOOST_LOG_SEV(lg, INFO) << "Statistics:";
    BOOST_LOG_SEV(lg, INFO) << "@ flowFunctionsConstructionCount: "
                            << flowFunctionConstructionCount;
    BOOST_LOG_SEV(lg, INFO) << "@ flowFunctionsApplicationCount: "
                            << flowFunctionApplicationCount;
    BOOST_LOG_SEV(lg, INFO) << "@ specialFlowFunctionUsageCount: "
                            << specialSummaryFlowFunctionApplicationCount;
    BOOST_LOG_SEV(lg, INFO) << "@ propagationCount: " << propagationCount;
    BOOST_LOG_SEV(lg, INFO) << "@ flow function construction: "
                            << durationFlowFunctionConstruction.count()
                            << " ms";
    BOOST_LOG_SEV(lg, INFO) << "@ flow function application: "
                            << durationFlowFunctionApplication.count() << " ms";
    cachedFlowEdgeFunctions.print();
  }

  /**
   * Returns the V-type result for the given value at the given statement.
   * TOP values are never returned.
   */
  V resultAt(N stmt, D value) { return valtab.get(stmt, value); }

  /**
   * Returns the resulting environment for the given statement.
         * The artificial zero value can be automatically stripped.
         * TOP values are never returned.
   */
  unordered_map<D, V> resultsAt(N stmt, bool stripZero = false) {
    unordered_map<D, V> result = valtab.row(stmt);
    if (stripZero) {
      for (auto &pair : result) {
        if (ideTabulationProblem.isZeroValue(pair.first))
          result.erase(pair.first);
      }
    }
    return result;
  }

private:
  IDETabulationProblem<N, D, M, V, I> &ideTabulationProblem;
  FlowEdgeFunctionCache<N, D, M, V, I> cachedFlowEdgeFunctions;
  bool recordEdges;
  size_t flowFunctionConstructionCount = 0;
  size_t flowFunctionApplicationCount = 0;
  size_t specialSummaryFlowFunctionApplicationCount = 0;
  size_t summaryFlowFunctionApplicationCount = 0;
  size_t propagationCount = 0;
  chrono::milliseconds durationFlowFunctionConstruction;
  chrono::milliseconds durationFlowFunctionApplication;

  void saveEdges(N sourceNode, N sinkStmt, D sourceVal, set<D> destVals,
                 bool interP) {
    if (!recordEdges)
      return;
    Table<N, N, map<D, set<D>>> &tgtMap =
        (interP) ? computedInterPathEdges : computedIntraPathEdges;
    tgtMap.get(sourceNode, sinkStmt)[sourceVal].insert(destVals.begin(),
                                                       destVals.end());
    // map<D, set<D>> &m = tgtMap.get(sourceNode, sinkStmt);
    // if (m.empty()) {
    //   tgtMap.insert(sourceNode, sinkStmt, m);
    // }
    // m[sourceVal] = set<D>{destVals};
  }

  /**
   * Lines 13-20 of the algorithm; processing a call site in the caller's
   * context.
   *
   * For each possible callee, registers incoming call edges.
   * Also propagates call-to-return flows and summarized callee flows within the
   * caller.
   *
   * 	The following cases must be considered and handled:
   *		1. Process as usual and just process the call
   *		2. Create a new summary for that function (which shall be done
   *       by the problem)
   *		3. Just use an existing summary provided by the problem
   *		4. If a special function is called, use a special summary
   *       function
   *
   * @param edge an edge whose target node resembles a method call
   */
  void processCall(PathEdge<N, D> edge) {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG)
        << "process call at target: "
        << ideTabulationProblem.N_to_string(edge.getTarget());
    D d1 = edge.factAtSource();
    N n = edge.getTarget(); // a call node; line 14...
    D d2 = edge.factAtTarget();
    shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
    set<N> returnSiteNs = icfg.getReturnSitesOfCallAt(n);
    set<M> callees = icfg.getCalleesOfCallAt(n);
    BOOST_LOG_SEV(lg, DEBUG) << "possible callees:";
    for (auto callee : callees) {
      BOOST_LOG_SEV(lg, DEBUG) << callee->getName().str();
    }
    BOOST_LOG_SEV(lg, DEBUG) << "possible return sites:";
    for (auto ret : returnSiteNs) {
      BOOST_LOG_SEV(lg, DEBUG) << ideTabulationProblem.N_to_string(ret);
    }
    // for each possible callee
    for (M sCalledProcN : callees) { // still line 14
      // check if a special summary for the called procedure exists
      shared_ptr<FlowFunction<D>> specialSum =
          cachedFlowEdgeFunctions.getSummaryFlowFunction(n, sCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (specialSum) {
        BOOST_LOG_SEV(lg, DEBUG) << "Found and process special summary";
        for (N returnSiteN : returnSiteNs) {
          summaryFlowFunctionApplicationCount++;
          set<D> res = computeSummaryFlowFunction(specialSum, d1, d2);
          saveEdges(n, returnSiteN, d2, res, false);
          for (D d3 : res) {
            shared_ptr<EdgeFunction<V>> sumEdgFnE =
                cachedFlowEdgeFunctions.getSummaryEdgeFunction(n, d2,
                                                               returnSiteN, d3);
            propagate(d1, returnSiteN, d3, f->composeWith(sumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        shared_ptr<FlowFunction<D>> function =
            cachedFlowEdgeFunctions.getCallFlowFunction(n, sCalledProcN);
        flowFunctionConstructionCount++;
        set<D> res = computeCallFlowFunction(function, d1, d2);
        // for each callee's start point(s)
        set<N> startPointsOf = icfg.getStartPointsOf(sCalledProcN);
        if (startPointsOf.empty()) {
          BOOST_LOG_SEV(lg, DEBUG) << "Start points of '" +
                                          icfg.getMethodName(sCalledProcN) +
                                          "' currently not available!";
        }
        // if startPointsOf is empty, the called function is a declaration
        for (N sP : startPointsOf) {
          saveEdges(n, sP, d2, res, true);
          // for each result node of the call-flow function
          for (D d3 : res) {
            // create initial self-loop
            propagate(d3, sP, d3, EdgeIdentity<V>::v(), n, false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            addIncoming(sP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            set<typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell>
                endSumm = set<
                    typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell>(
                    endSummary(sP, d3));
            // still line 15.2 of Naeem/Lhotak/Rodriguez
            // for each already-queried exit value <eP,d4> reachable from
            // <sP,d3>, create new caller-side jump functions to the return
            // sites because we have observed a potentially new incoming
            // edge into <sP,d3>
            for (typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell entry :
                 endSumm) {
              N eP = entry.getRowKey();
              D d4 = entry.getColumnKey();
              shared_ptr<EdgeFunction<V>> fCalleeSummary = entry.getValue();
              // for each return site
              for (N retSiteN : returnSiteNs) {
                // compute return-flow function
                shared_ptr<FlowFunction<D>> retFunction =
                    cachedFlowEdgeFunctions.getRetFlowFunction(n, sCalledProcN,
                                                               eP, retSiteN);
                flowFunctionConstructionCount++;
                set<D> returnedFacts = computeReturnFlowFunction(
                    retFunction, d3, d4, n, set<D>{d2});
                saveEdges(eP, retSiteN, d4, returnedFacts, true);
                // for each target value of the function
                for (D d5 : returnedFacts) {
                  // update the caller-side summary function
                  // get call edge function
                  shared_ptr<EdgeFunction<V>> f4 =
                      cachedFlowEdgeFunctions.getCallEdgeFunction(
                          n, d2, sCalledProcN, d3);
                  // get return edge function
                  shared_ptr<EdgeFunction<V>> f5 =
                      cachedFlowEdgeFunctions.getReturnEdgeFunction(
                          n, sCalledProcN, eP, d4, retSiteN, d5);
                  // compose call * calleeSummary * return edge functions
                  shared_ptr<EdgeFunction<V>> fPrime =
                      f4->composeWith(fCalleeSummary)->composeWith(f5);
                  D d5_restoredCtx = restoreContextOnReturnedFact(n, d2, d5);
                  // prpagte the effects of the entire call
                  propagate(d1, retSiteN, d5_restoredCtx,
                            f->composeWith(fPrime), n, false);
                }
              }
            }
          }
        }
      }
      // line 17-19 of Naeem/Lhotak/Rodriguez
      // process intra-procedural flows along call-to-return flow functions
      for (N returnSiteN : returnSiteNs) {
        shared_ptr<FlowFunction<D>> callToReturnFlowFunction =
            cachedFlowEdgeFunctions.getCallToRetFlowFunction(n, returnSiteN);
        flowFunctionConstructionCount++;
        set<D> returnFacts =
            computeCallToReturnFlowFunction(callToReturnFlowFunction, d1, d2);
        saveEdges(n, returnSiteN, d2, returnFacts, false);
        for (D d3 : returnFacts) {
          shared_ptr<EdgeFunction<V>> edgeFnE =
              cachedFlowEdgeFunctions.getCallToReturnEdgeFunction(
                  n, d2, returnSiteN, d3);
          propagate(d1, returnSiteN, d3, f->composeWith(edgeFnE), n, false);
        }
      }
    }
  }

  /**
   * Lines 33-37 of the algorithm.
   * Simply propagate normal, intra-procedural flows.
   * @param edge
   */
  void processNormalFlow(PathEdge<N, D> edge) {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG)
        << "process normal at target: "
        << ideTabulationProblem.N_to_string(edge.getTarget());
    if (edge.factAtSource() == nullptr)
      BOOST_LOG_SEV(lg, DEBUG) << "fact at source is nullptr";
    D d1 = edge.factAtSource();
    N n = edge.getTarget();
    D d2 = edge.factAtTarget();
    shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
    auto successorInst = icfg.getSuccsOf(n);
    for (auto m : successorInst) {
      shared_ptr<FlowFunction<D>> flowFunction =
          cachedFlowEdgeFunctions.getNormalFlowFunction(n, m);
      flowFunctionConstructionCount++;
      set<D> res = computeNormalFlowFunction(flowFunction, d1, d2);
      saveEdges(n, m, d2, res, false);
      for (D d3 : res) {
        shared_ptr<EdgeFunction<V>> fprime = f->composeWith(
            cachedFlowEdgeFunctions.getNormalEdgeFunction(n, d2, m, d3));
        propagate(d1, m, d3, fprime, nullptr, false);
      }
    }
  }

  /**
   * Computes the final values for edge functions.
   */
  void computeValues() {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "start computing values";
    // Phase II(i)
    map<N, set<D>> allSeeds(initialSeeds);
    for (N unbalancedRetSite : unbalancedRetSites) {
      if (allSeeds[unbalancedRetSite].empty()) {
        allSeeds.insert(make_pair(unbalancedRetSite, set<D>({zeroValue})));
      }
    }
    // do processing
    for (const auto &seed : allSeeds) {
      N startPoint = seed.first;
      for (D val : seed.second) {
        setVal(startPoint, val, ideTabulationProblem.bottomElement());
        pair<N, D> superGraphNode(startPoint, val);
        valuePropagationTask(superGraphNode);
      }
    }
    // Phase II(ii)
    // we create an array of all nodes and then dispatch fractions of this array
    // to multiple threads
    set<N> allNonCallStartNodes = icfg.allNonCallStartNodes();
    vector<N> nonCallStartNodesArray(allNonCallStartNodes.size());
    size_t i = 0;
    for (N n : allNonCallStartNodes) {
      nonCallStartNodesArray[i] = n;
      i++;
    }
    valueComputationTask(nonCallStartNodesArray);
  }

  void propagateValueAtStart(pair<N, D> nAndD, N n) {
    D d = nAndD.second;
    M p = icfg.getMethodOf(n);
    for (N c : icfg.getCallsFromWithin(p)) {
      for (auto entry : jumpFn->forwardLookup(d, c)) {
        D dPrime = entry.first;
        shared_ptr<EdgeFunction<V>> fPrime = entry.second;
        N sP = n;
        V value = val(sP, d);
        propagateValue(c, dPrime, fPrime->computeTarget(value));
        flowFunctionApplicationCount++;
      }
    }
  }

  void propagateValueAtCall(pair<N, D> nAndD, N n) {
    D d = nAndD.second;
    for (M q : icfg.getCalleesOfCallAt(n)) {
      shared_ptr<FlowFunction<D>> callFlowFunction =
          cachedFlowEdgeFunctions.getCallFlowFunction(n, q);
      flowFunctionConstructionCount++;
      for (D dPrime : callFlowFunction->computeTargets(d)) {
        shared_ptr<EdgeFunction<V>> edgeFn =
            cachedFlowEdgeFunctions.getCallEdgeFunction(n, d, q, dPrime);
        for (N startPoint : icfg.getStartPointsOf(q)) {
          propagateValue(startPoint, dPrime, edgeFn->computeTarget(val(n, d)));
          flowFunctionApplicationCount++;
        }
      }
    }
  }

  void propagateValue(N nHashN, D nHashD, V v) {
    V valNHash = val(nHashN, nHashD);
    V vPrime = joinValueAt(nHashN, nHashD, valNHash, v);
    if (!(vPrime == valNHash)) {
      setVal(nHashN, nHashD, vPrime);
      valuePropagationTask(pair<N, D>(nHashN, nHashD));
    }
  }

  V val(N nHashN, D nHashD) {
    if (valtab.contains(nHashN, nHashD)) {
      return valtab.get(nHashN, nHashD);
    } else {
      // implicitly initialized to top; see line [1] of Fig. 7 in SRH96 paper
      return ideTabulationProblem.topElement();
    }
  }

  void setVal(N nHashN, D nHashD, V l) {
    auto &lg = lg::get();
    // TOP is the implicit default value which we do not need to store.
    if (l == ideTabulationProblem.topElement()) {
      // do not store top values
      valtab.remove(nHashN, nHashD);
    } else {
      valtab.insert(nHashN, nHashD, l);
    }
    BOOST_LOG_SEV(lg, DEBUG)
        << "VALUE: " << icfg.getMethodOf(nHashN)->getName().str() << " "
        << "node: " << ideTabulationProblem.N_to_string(nHashN) << " "
        << "fact: " << ideTabulationProblem.D_to_string(nHashD) << " "
        << "val: " << ideTabulationProblem.V_to_string(l);
  }

  shared_ptr<EdgeFunction<V>> jumpFunction(PathEdge<N, D> edge) {
    if (!jumpFn->forwardLookup(edge.factAtSource(), edge.getTarget())
             .count(edge.factAtTarget())) {
      // JumpFn initialized to all-top, see line [2] in SRH96 paper
      return allTop;
    }
    return jumpFn->forwardLookup(edge.factAtSource(),
                                 edge.getTarget())[edge.factAtTarget()];
  }

  void addEndSummary(N sP, D d1, N eP, D d2, shared_ptr<EdgeFunction<V>> f) {
    // Table<N, D, shared_ptr<EdgeFunction<V>>> &summaries =
    //     endsummarytab.get(sP, d1);
    //     summaries.insert(eP, d2, f);
    // note: at this point we don't need to join with a potential previous f
    // because f is a jump function, which is already properly joined
    // within propagate(..)
    endsummarytab.get(sP, d1).insert(eP, d2, f);
  }

  // should be made a callable at some point
  void pathEdgeProcessingTask(PathEdge<N, D> edge) {
    auto &lg = lg::get();
    propagationCount++;
    bool isCall = icfg.isCallStmt(edge.getTarget());
    if (!isCall) {
      if (icfg.isExitStmt(edge.getTarget())) {
        processExit(edge);
      }
      if (!icfg.getSuccsOf(edge.getTarget()).empty()) {
        processNormalFlow(edge);
      }
    } else {
      processCall(edge);
    }
  }

  // should be made a callable at some point
  void valuePropagationTask(pair<N, D> nAndD) {
    N n = nAndD.first;
    // our initial seeds are not necessarily method-start points but here they
    // should be treated as such the same also for unbalanced return sites in
    // an unbalanced problem
    if (icfg.isStartPoint(n) || initialSeeds.count(n) ||
        unbalancedRetSites.count(n)) {
      propagateValueAtStart(nAndD, n);
    }
    if (icfg.isCallStmt(n)) {
      propagateValueAtCall(nAndD, n);
    }
  }

  // should be made a callable at some point
  void valueComputationTask(vector<N> values) {
    for (N n : values) {
      for (N sP : icfg.getStartPointsOf(icfg.getMethodOf(n))) {
        Table<D, D, shared_ptr<EdgeFunction<V>>> lookupByTarget;
        lookupByTarget = jumpFn->lookupByTarget(n);
        for (typename Table<D, D, shared_ptr<EdgeFunction<V>>>::Cell
                 sourceValTargetValAndFunction : lookupByTarget.cellSet()) {
          D dPrime = sourceValTargetValAndFunction.getRowKey();
          D d = sourceValTargetValAndFunction.getColumnKey();
          shared_ptr<EdgeFunction<V>> fPrime =
              sourceValTargetValAndFunction.getValue();
          V targetVal = val(sP, dPrime);
          setVal(n, d, ideTabulationProblem.join(
                           val(n, d), fPrime->computeTarget(targetVal)));
          flowFunctionApplicationCount++;
        }
      }
    }
  }

protected:
  D zeroValue;
  I icfg;
  bool computevalues;
  bool autoAddZero;
  bool followReturnPastSeeds;
  bool computePersistedSummaries;

  Table<N, N, map<D, set<D>>> computedIntraPathEdges;

  Table<N, N, map<D, set<D>>> computedInterPathEdges;

  shared_ptr<EdgeFunction<V>> allTop;

  shared_ptr<JumpFunctions<N, D, V>> jumpFn;

  // stores summaries that were queried before they were computed
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> endsummarytab;

  // edges going along calls
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<N, D, map<N, set<D>>> incomingtab;

  // stores the return sites (inside callers) to which we have unbalanced
  // returns if followReturnPastSeeds is enabled
  set<N> unbalancedRetSites;

  map<N, set<D>> initialSeeds;

  Table<N, D, V> valtab;

  /**
   * Schedules the processing of initial seeds, initiating the analysis.
   * Clients should only call this methods if performing synchronization on
   * their own. Normally, solve() should be called instead.
   */
  void submitInitalSeeds() {
    for (const auto &seed : initialSeeds) {
      N startPoint = seed.first;
      for (const D &value : seed.second) {
        startPoint->dump();
        value->dump();
        propagate(zeroValue, startPoint, value, EdgeIdentity<V>::v(), nullptr,
                  false);
      }
      jumpFn->addFunction(zeroValue, startPoint, zeroValue,
                          EdgeIdentity<V>::v());
    }
  }

  /**
   * Lines 21-32 of the algorithm.
   *
   * Stores callee-side summaries.
   * Also, at the side of the caller, propagates intra-procedural flows to
   * return sites using those newly computed summaries.
   *
   * @param edge an edge whose target node resembles a method exit
   */
  void processExit(PathEdge<N, D> edge) {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG)
        << "process exit at target: "
        << ideTabulationProblem.N_to_string(edge.getTarget());
    N n = edge.getTarget(); // an exit node; line 21...
    shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
    M methodThatNeedsSummary = icfg.getMethodOf(n);
    D d1 = edge.factAtSource();
    D d2 = edge.factAtTarget();
    // for each of the method's start points, determine incoming calls
    set<N> startPointsOf = icfg.getStartPointsOf(methodThatNeedsSummary);
    map<N, set<D>> inc;
    for (N sP : startPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      addEndSummary(sP, d1, n, d2, f);
      // copy to avoid concurrent modification exceptions by other threads
      for (auto entry : incoming(d1, sP))
        inc[entry.first] = set<D>{entry.second};
    }
    printEndSummaryTab();
    printIncomingTab();
    // for each incoming call edge already processed
    //(see processCall(..))
    for (auto entry : inc) {
      // line 22
      N c = entry.first;
      // for each return site
      for (N retSiteC : icfg.getReturnSitesOfCallAt(c)) {
        // compute return-flow function
        shared_ptr<FlowFunction<D>> retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                c, methodThatNeedsSummary, n, retSiteC);
        flowFunctionConstructionCount++;
        // for each incoming-call value
        for (D d4 : entry.second) {
          set<D> targets =
              computeReturnFlowFunction(retFunction, d1, d2, c, entry.second);
          saveEdges(n, retSiteC, d2, targets, true);
          // for each target value at the return site
          // line 23
          for (D d5 : targets) {
            // compute composed function
            // get call edge function
            shared_ptr<EdgeFunction<V>> f4 =
                cachedFlowEdgeFunctions.getCallEdgeFunction(
                    c, d4, icfg.getMethodOf(n), d1);
            // get return edge function
            shared_ptr<EdgeFunction<V>> f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
            // compose call function * function * return function
            shared_ptr<EdgeFunction<V>> fPrime =
                f4->composeWith(f)->composeWith(f5);
            // for each jump function coming into the call, propagate to return
            // site using the composed function
            for (auto valAndFunc : jumpFn->reverseLookup(c, d4)) {
              shared_ptr<EdgeFunction<V>> f3 = valAndFunc.second;
              if (!f3->equalTo(allTop)) {
                D d3 = valAndFunc.first;
                D d5_restoredCtx = restoreContextOnReturnedFact(c, d4, d5);
                propagate(d3, retSiteC, d5_restoredCtx, f3->composeWith(fPrime),
                          c, false);
              }
            }
          }
        }
      }
    }
    // handling for unbalanced problems where we return out of a method with a
    // fact for which we have no incoming flow.
    // note: we propagate that way only values that originate from ZERO, as
    // conditionally generated values should only
    // be propagated into callers that have an incoming edge for this condition
    if (followReturnPastSeeds && inc.empty() &&
        ideTabulationProblem.isZeroValue(d1)) {
      set<N> callers = icfg.getCallersOf(methodThatNeedsSummary);
      for (N c : callers) {
        for (N retSiteC : icfg.getReturnSitesOfCallAt(c)) {
          shared_ptr<FlowFunction<D>> retFunction =
              cachedFlowEdgeFunctions.getRetFlowFunction(
                  c, methodThatNeedsSummary, n, retSiteC);
          flowFunctionConstructionCount++;
          set<D> targets = computeReturnFlowFunction(retFunction, d1, d2, c,
                                                     set<D>{zeroValue});
          saveEdges(n, retSiteC, d2, targets, true);
          for (D d5 : targets) {
            shared_ptr<EdgeFunction<V>> f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
            propagteUnbalancedReturnFlow(retSiteC, d5, f->composeWith(f5), c);
            // register for value processing (2nd IDE phase)
            unbalancedRetSites.insert(retSiteC);
          }
        }
      }
      // in cases where there are no callers, the return statement would
      // normally not be processed at all; this might be undesirable if
      // the flow function has a side effect such as registering a taint;
      // instead we thus call the return flow function will a null caller
      if (callers.empty()) {
        shared_ptr<FlowFunction<D>> retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                nullptr, methodThatNeedsSummary, n, nullptr);
        flowFunctionConstructionCount++;
        retFunction->computeTargets(d2);
      }
    }
  }

  void propagteUnbalancedReturnFlow(N retSiteC, D targetVal,
                                    shared_ptr<EdgeFunction<V>> edgeFunction,
                                    N relatedCallSite) {
    propagate(zeroValue, retSiteC, targetVal, edgeFunction, relatedCallSite,
              true);
  }

  /**
   * This method will be called for each incoming edge and can be used to
   * transfer knowledge from the calling edge to the returning edge, without
   * affecting the summary edges at the callee.
   * @param callSite
   *
   * @param d4
   *            Fact stored with the incoming edge, i.e., present at the
   *            caller side
   * @param d5
   *            Fact that originally should be propagated to the caller.
   * @return Fact that will be propagated to the caller.
   */
  D restoreContextOnReturnedFact(N callSite, D d4, D d5) {
    // TODO support LinkedNode and JoinHandlingNode
    //		if (d5 instanceof LinkedNode) {
    //			((LinkedNode<D>) d5).setCallingContext(d4);
    //		}
    //		if(d5 instanceof JoinHandlingNode) {
    //			((JoinHandlingNode<D>) d5).setCallingContext(d4);
    //		}
    return d5;
  }

  /**
   * Computes the normal flow function for the given set of start and end
   * abstractions-
   * @param flowFunction The normal flow function to compute
   * @param d1 The abstraction at the method's start node
   * @param d2 The abstraction at the current node
   * @return The set of abstractions at the successor node
   */
  set<D> computeNormalFlowFunction(shared_ptr<FlowFunction<D>> flowFunction,
                                   D d1, D d2) {
    return flowFunction->computeTargets(d2);
  }

  /**
    * TODO: comment
    */
  set<D>
  computeSummaryFlowFunction(shared_ptr<FlowFunction<D>> SummaryFlowFunction,
                             D d1, D d2) {
    return SummaryFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the call flow function for the given call-site abstraction
   * @param callFlowFunction The call flow function to compute
   * @param d1 The abstraction at the current method's start node.
   * @param d2 The abstraction at the call site
   * @return The set of caller-side abstractions at the callee's start node
   */
  set<D> computeCallFlowFunction(shared_ptr<FlowFunction<D>> callFlowFunction,
                                 D d1, D d2) {
    return callFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the call-to-return flow function for the given call-site
   * abstraction
   * @param callToReturnFlowFunction The call-to-return flow function to
   * compute
   * @param d1 The abstraction at the current method's start node.
   * @param d2 The abstraction at the call site
   * @return The set of caller-side abstractions at the return site
   */
  set<D> computeCallToReturnFlowFunction(
      shared_ptr<FlowFunction<D>> callToReturnFlowFunction, D d1, D d2) {
    return callToReturnFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the return flow function for the given set of caller-side
   * abstractions.
   * @param retFunction The return flow function to compute
   * @param d1 The abstraction at the beginning of the callee
   * @param d2 The abstraction at the exit node in the callee
   * @param callSite The call site
   * @param callerSideDs The abstractions at the call site
   * @return The set of caller-side abstractions at the return site
   */
  set<D> computeReturnFlowFunction(shared_ptr<FlowFunction<D>> retFunction,
                                   D d1, D d2, N callSite,
                                   set<D> callerSideDs) {
    return retFunction->computeTargets(d2);
  }

  /**
   * Propagates the flow further down the exploded super graph, merging any edge
         * function that might already have been computed for targetVal at
   * target.
         *
   * @param sourceVal the source value of the propagated summary edge
   * @param target the target statement
   * @param targetVal the target value at the target statement
   * @param f the new edge function computed from (s0,sourceVal) to
   * (target,targetVal)
   * @param relatedCallSite for call and return flows the related call
         * statement, nullptr otherwise (this value is not used within this
         * implementation but may be useful for subclasses of IDESolver)
   * @param isUnbalancedReturn true if this edge is propagating an
         * unbalanced return (this value is not used within this implementation
         * but may be useful for subclasses of {@link IDESolver})
   */
  void
  propagate(D sourceVal, N target, D targetVal, shared_ptr<EdgeFunction<V>> f,
            /* deliberately exposed to clients */ N relatedCallSite,
            /* deliberately exposed to clients */ bool isUnbalancedReturn) {
    auto &lg = lg::get();
    shared_ptr<EdgeFunction<V>> jumpFnE;
    shared_ptr<EdgeFunction<V>> fPrime;
    jumpFnE = jumpFn->reverseLookup(target, targetVal)[sourceVal];
    if (jumpFnE == nullptr)
      jumpFnE = allTop; // jump function is initialized to all-top
    fPrime = jumpFnE->joinWith(f);
    bool newFunction = !(fPrime->equalTo(jumpFnE));
    if (newFunction) {
      jumpFn->addFunction(sourceVal, target, targetVal, fPrime);
      PathEdge<N, D> edge(sourceVal, target, targetVal);
      pathEdgeProcessingTask(edge);
      if (!ideTabulationProblem.isZeroValue(targetVal)) {
        BOOST_LOG_SEV(lg, DEBUG)
            << "EDGE: <" << target->getFunction()->getName().str() << ", "
            << ideTabulationProblem.D_to_string(sourceVal) << "> ---> <"
            << ideTabulationProblem.N_to_string(target) << ", "
            << ideTabulationProblem.D_to_string(targetVal) << ">";
      }
    }
  }

  V joinValueAt(N unit, D fact, V curr, V newVal) {
    return ideTabulationProblem.join(curr, newVal);
  }

  set<typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell>
  endSummary(N sP, D d3) {
    // Table<N, D, shared_ptr<EdgeFunction<V>>> &m = endsummarytab.get(sP, d3);
    // if (m.isEmpty())
    //   return set<typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell>{};
    // return m.cellSet();
    return endsummarytab.get(sP, d3).cellSet();
  }

  map<N, set<D>> incoming(D d1, N sP) {
    // map<N, set<D>> m = incomingtab.get(sP, d1);
    // if (m.empty())
    //   return map<N, set<D>>{};
    // return m;
    return incomingtab.get(sP, d1);
  }

  void addIncoming(N sP, D d3, N n, D d2) {
    // map<N, set<D>> summaries = incomingtab.get(sP, d3);
    // set<D> &s = summaries[n];
    // s.insert(d2);
    // incomingtab.insert(sP, d3, summaries);
    incomingtab.get(sP, d3)[n].insert(d2);
  }

  void printIncomingTab() {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "start incomingtab entry";
    for (auto cell : incomingtab.cellSet()) {
      BOOST_LOG_SEV(lg, DEBUG) << "sP: "
                               << ideTabulationProblem.N_to_string(cell.r);
      BOOST_LOG_SEV(lg, DEBUG) << "d3: "
                               << ideTabulationProblem.D_to_string(cell.c);
      for (auto entry : cell.v) {
        BOOST_LOG_SEV(lg, DEBUG)
            << "n: " << ideTabulationProblem.N_to_string(entry.first);
        for (auto fact : entry.second) {
          BOOST_LOG_SEV(lg, DEBUG) << "d2: "
                                   << ideTabulationProblem.D_to_string(fact);
        }
      }
      BOOST_LOG_SEV(lg, DEBUG) << "-----";
    }
    BOOST_LOG_SEV(lg, DEBUG) << "end incomingtab entry";
  }

  void printEndSummaryTab() {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "start endsummarytab entry";
    for (auto cell : endsummarytab.cellVec()) {
      BOOST_LOG_SEV(lg, DEBUG) << "sP: "
                               << ideTabulationProblem.N_to_string(cell.r);
      BOOST_LOG_SEV(lg, DEBUG) << "d1: "
                               << ideTabulationProblem.D_to_string(cell.c);
      for (auto inner_cell : cell.v.cellVec()) {
        BOOST_LOG_SEV(lg, DEBUG)
            << "eP: " << ideTabulationProblem.N_to_string(inner_cell.r);
        BOOST_LOG_SEV(lg, DEBUG)
            << "d2: " << ideTabulationProblem.D_to_string(inner_cell.c);
        BOOST_LOG_SEV(lg, DEBUG) << "edge fun: " << inner_cell.v->toString();
      }
      BOOST_LOG_SEV(lg, DEBUG) << "-----";
    }
    BOOST_LOG_SEV(lg, DEBUG) << "end endsummarytab entry";
  }
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_IDESOLVER_HH_ */
