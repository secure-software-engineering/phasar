/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IDESolver.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IDESOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IDESOLVER_H_

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include <curl/curl.h>
#include <json.hpp>

#include <boost/algorithm/string/trim.hpp>

#include <llvm/Support/raw_ostream.h>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowEdgeFunctionCache.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/JoinLattice.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/IFDSToIDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/JoinHandlingNode.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/JumpFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LinkedNode.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/PathEdge.h>
#include <phasar/PhasarLLVM/IfdsIde/ZeroedFlowFunction.h>

#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/PAMMMacros.h>
#include <phasar/Utils/Table.h>

namespace psr {

using json = nlohmann::json;
// Forward declare the Transformation
template <typename N, typename D, typename M, typename I>
class IFDSToIDETabulationProblem;

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
template <typename N, typename D, typename M, typename V, typename I>
class IDESolver {
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
        PathEdgeCount(0), allTop(tabulationProblem.allTopFunction()),
        jumpFn(std::make_shared<JumpFunctions<N, D, M, V, I>>(
            allTop, ideTabulationProblem)),
        initialSeeds(tabulationProblem.initialSeeds()) {
    // std::cout << "called IDESolver::IDESolver() ctor with IDEProblem"
    //           << std::endl;
  }

  virtual ~IDESolver() = default;

  json getAsJson() {
    const static std::string DataFlowID = "DataFlow";
    json J;
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      J[DataFlowID] = "EMPTY";
    } else {
      std::vector<typename Table<N, D, V>::Cell> cells;
      for (auto cell : results) {
        cells.push_back(cell);
      }
      sort(cells.begin(), cells.end(),
           [](typename Table<N, D, V>::Cell a,
              typename Table<N, D, V>::Cell b) { return a.r < b.r; });
      N curr;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
        std::string n = ideTabulationProblem.NtoString(cells[i].r);
        boost::algorithm::trim(n);
        std::string node =
            icfg.getMethodName(icfg.getMethodOf(curr)) + "::" + n;
        J[DataFlowID][node];
        std::string fact = ideTabulationProblem.DtoString(cells[i].c);
        boost::algorithm::trim(fact);
        std::string value = ideTabulationProblem.VtoString(cells[i].v);
        boost::algorithm::trim(value);
        J[DataFlowID][node]["Facts"] += {fact, value};
      }
    }
    return J;
  }

  std::unordered_set<std::string> methodSet;
  std::unordered_set<std::string> stmtSet;
  json graph;

  void sendGraphToServer() {
    std::ofstream o("myJsonGraph.json");
    o << graph << std::endl;
    o.close();

    CURL *curl;
    CURLcode res;

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";

    curl_global_init(CURL_GLOBAL_ALL);

    /* Fill in the file upload field */
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "sendfile",
                 CURLFORM_FILE, "myJsonGraph.json", CURLFORM_END);

    /* Fill in the filename field */
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename",
                 CURLFORM_COPYCONTENTS, "myJsonGraph.json", CURLFORM_END);

    /* Fill in the submit field too, even if this is rarely needed */
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit",
                 CURLFORM_COPYCONTENTS, "send", CURLFORM_END);

    curl = curl_easy_init();
    /* initalize custom header list (stating that Expect: 100-continue is not
     wanted */
    headerlist = curl_slist_append(headerlist, buf);
    if (curl) {
      /* what URL that receives this POST */
      curl_easy_setopt(curl, CURLOPT_URL,
                       "http://localhost:3000/api/framework/addGraph");
      // if ( (argc == 2) && (!strcmp(argv[1], "noexpectheader")) )
      //   /* only disable 100-continue header if explicitly requested */
      //   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
      curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

      /* Perform the request, res will get the return code */
      res = curl_easy_perform(curl);
      /* Check for errors */
      if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

      /* always cleanup */
      curl_easy_cleanup(curl);

      /* then cleanup the formpost chain */
      curl_formfree(formpost);
      /* free slist */
      curl_slist_free_all(headerlist);
    }

    if (remove("myJsonGraph.json") != 0)
      std::cout << "Error deleting file" << std::endl;
    else
      std::cout << "File successfully deleted" << std::endl;
  }

  void exportJson(std::string graphId) {
    std::cout << "new export for graph " << graphId << std::endl;
    std::vector<json> methods;
    std::vector<json> statements;
    std::vector<json> dataflowfacts;

    graph = {{"id", graphId},
             {"methods", methods},
             {"statements", statements},
             {"dataflowFacts", dataflowfacts}};

    for (auto seed : initialSeeds) {
      graph["methods"].push_back(
          {{"methodName",
            ideTabulationProblem.MtoString(icfg.getMethodOf(seed.first))}});
      iterateMethod(icfg.getSuccsOf(seed.first));
    }

    sendGraphToServer();
  }

  json getStatementJson(N succ) {
    auto currentId = icfg.getStatementId(succ);
    auto currentMethodName =
        ideTabulationProblem.MtoString(icfg.getMethodOf(succ));
    auto content = ideTabulationProblem.NtoString(succ);

    auto dVMap = resultsAt(succ);
    std::vector<std::string> dffIds;
    int i = 0;
    for (auto it : dVMap) {
      std::string dfId = currentId + "_dff_" + std::to_string(i);
      i++;
      json dfFact = {{"id", dfId},
                     {"content", ideTabulationProblem.DtoString(it.first)},
                     {"value", ideTabulationProblem.VtoString(it.second)},
                     {"statementId", currentId},
                     {"type", 5}};
      dffIds.push_back(dfId);
      graph["dataflowFacts"].push_back(dfFact);
    }

    auto next = icfg.getSuccsOf(succ);
    std::vector<std::string> succIds;

    for (auto stmt : next) {
      succIds.push_back(icfg.getStatementId(stmt));
    }

    json statement = {{"id", currentId},         {"method", currentMethodName},
                      {"content", content},      {"successors", succIds},
                      {"dataflowFacts", dffIds}, {"type", 0}};
    return statement;
  }

  void iterateMethod(std::vector<N> succs) {
    for (auto succ : succs) {
      auto currentId = icfg.getStatementId(succ);
      if (stmtSet.find(currentId) == stmtSet.end()) {
        stmtSet.insert(currentId);
        json statement = getStatementJson(succ);

        if (icfg.isCallStmt(succ)) {
          // if statement is call statement create call and returnsite
          // connect statement with callsite, callsite with returnsite,
          // returnsite with return statement annotate callsite and returnsite
          // with method name (name is unique)

          // called methods
          auto calledMethods = icfg.getCalleesOfCallAt(succ);
          std::vector<std::string> targetMethods;
          statement["type"] = 1;
          for (auto method : calledMethods) {
            auto methodName = ideTabulationProblem.MtoString(method);
            statement["successors"].push_back(methodName);

            targetMethods.push_back(methodName);
            if (methodSet.find(methodName) == methodSet.end()) {
              graph["methods"].push_back({{"methodName", methodName}});
              // start points of called method
              auto nodeSet = icfg.getStartPointsOf(method);
              for (auto tmp : nodeSet) {
                methodSet.insert(methodName);
                iterateMethod(icfg.getSuccsOf(tmp));
              }
            }
          }
          statement["targetMethods"] = targetMethods;
          auto returnsites = icfg.getReturnSitesOfCallAt(succ);

          for (auto returnsite : returnsites) {
            auto returnsiteId = icfg.getStatementId(returnsite);

            json returnsiteStmt = getStatementJson(returnsite);
            returnsiteStmt["type"] = 2;
            for (auto m : targetMethods) {
              returnsiteStmt["successors"].push_back(m);
            }

            graph["statements"].push_back(returnsiteStmt);
            stmtSet.insert(returnsiteId);
            iterateMethod(icfg.getSuccsOf(returnsite));
          }
        }

        graph["statements"].push_back(statement);
        iterateMethod(icfg.getSuccsOf(succ));
      }
    }
  }

  /**
   * @brief Runs the solver on the configured problem. This can take some time.
   */
  virtual void solve() {
    PAMM_GET_INSTANCE;
    REG_COUNTER("Gen facts", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Kill facts", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Summary-reuse", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Intra Path Edges", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Inter Path Edges", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("FF Queries", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("EF Queries", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Value Propagation", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Value Computation", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("SpecialSummary-FF Application", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("SpecialSummary-EF Queries", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("JumpFn Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Process Call", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Process Normal", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Process Exit", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("[Calls] getPointsToSet", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_HISTOGRAM("Data-flow facts", PAMM_SEVERITY_LEVEL::Full);
    REG_HISTOGRAM("Points-to", PAMM_SEVERITY_LEVEL::Full);

    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "IDE solver is solving the specified problem");
    // computations starting here
    START_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    // We start our analysis and construct exploded supergraph
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "Submit initial seeds, construct exploded super graph");
    submitInitalSeeds();
    STOP_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    if (computevalues) {
      START_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
      // Computing the final values for the edge functions
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg, INFO)
          << "Compute the final values according to the edge functions");
      computeValues();
      STOP_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Problem solved");
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      computeAndPrintStatistics();
    }
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
  std::unordered_map<D, V> resultsAt(N stmt, bool stripZero = false) {
    std::unordered_map<D, V> result = valtab.row(stmt);
    if (stripZero) {
      for (auto it = result.begin(); it != result.end();) {
        if (ideTabulationProblem.isZeroValue(it->first)) {
          it = result.erase(it);
        } else {
          ++it;
        }
      }
    }
    return result;
  }

private:
  std::unique_ptr<IFDSToIDETabulationProblem<N, D, M, I>> transformedProblem;
  IDETabulationProblem<N, D, M, V, I> &ideTabulationProblem;
  FlowEdgeFunctionCache<N, D, M, V, I> cachedFlowEdgeFunctions;
  bool recordEdges;

  void saveEdges(N sourceNode, N sinkStmt, D sourceVal, std::set<D> destVals,
                 bool interP) {
    if (!recordEdges)
      return;
    Table<N, N, std::map<D, std::set<D>>> &tgtMap =
        (interP) ? computedInterPathEdges : computedIntraPathEdges;
    tgtMap.get(sourceNode, sinkStmt)[sourceVal].insert(destVals.begin(),
                                                       destVals.end());
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
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Call", 1, PAMM_SEVERITY_LEVEL::Full);
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process call at target: "
                  << ideTabulationProblem.NtoString(edge.getTarget()));
    D d1 = edge.factAtSource();
    N n = edge.getTarget(); // a call node; line 14...
    D d2 = edge.factAtTarget();
    std::shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
    std::set<N> returnSiteNs = icfg.getReturnSitesOfCallAt(n);
    std::set<M> callees = icfg.getCalleesOfCallAt(n);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Possible callees:");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << callee->getName().str());
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Possible return sites:");
    for (auto ret : returnSiteNs) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << ideTabulationProblem.NtoString(ret));
    }
    // for each possible callee
    for (M sCalledProcN : callees) { // still line 14
      // check if a special summary for the called procedure exists
      std::shared_ptr<FlowFunction<D>> specialSum =
          cachedFlowEdgeFunctions.getSummaryFlowFunction(n, sCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (specialSum) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found and process special summary");
        for (N returnSiteN : returnSiteNs) {
          std::set<D> res = computeSummaryFlowFunction(specialSum, d1, d2);
          INC_COUNTER("SpecialSummary-FF Application", 1,
                      PAMM_SEVERITY_LEVEL::Full);
          ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, returnSiteN, d2, res, false);
          for (D d3 : res) {
            std::shared_ptr<EdgeFunction<V>> sumEdgFnE =
                cachedFlowEdgeFunctions.getSummaryEdgeFunction(n, d2,
                                                               returnSiteN, d3);
            INC_COUNTER("SpecialSummary-EF Queries", 1,
                        PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << sumEdgFnE->str() << " * "
                          << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
            propagate(d1, returnSiteN, d3, f->composeWith(sumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        std::shared_ptr<FlowFunction<D>> function =
            cachedFlowEdgeFunctions.getCallFlowFunction(n, sCalledProcN);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<D> res = computeCallFlowFunction(function, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        // for each callee's start point(s)
        std::set<N> startPointsOf = icfg.getStartPointsOf(sCalledProcN);
        if (startPointsOf.empty()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Start points of '" +
                               icfg.getMethodName(sCalledProcN) +
                               "' currently not available!");
        }
        // if startPointsOf is empty, the called function is a declaration
        for (N sP : startPointsOf) {
          saveEdges(n, sP, d2, res, true);
          // for each result node of the call-flow function
          for (D d3 : res) {
            // create initial self-loop
            propagate(d3, sP, d3, EdgeIdentity<V>::getInstance(), n,
                      false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            addIncoming(sP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            std::set<
                typename Table<N, D, std::shared_ptr<EdgeFunction<V>>>::Cell>
                endSumm = std::set<typename Table<
                    N, D, std::shared_ptr<EdgeFunction<V>>>::Cell>(
                    endSummary(sP, d3));
            // std::cout << "ENDSUMM" << std::endl;
            // std::cout << "Size: " << endSumm.size() << std::endl;
            // std::cout << "sP: " << ideTabulationProblem.NtoString(sP)
            //           << "\nd3: " << ideTabulationProblem.DtoString(d3)
            //           << std::endl;
            // printEndSummaryTab();
            // still line 15.2 of Naeem/Lhotak/Rodriguez
            // for each already-queried exit value <eP,d4> reachable from
            // <sP,d3>, create new caller-side jump functions to the return
            // sites because we have observed a potentially new incoming
            // edge into <sP,d3>
            for (typename Table<N, D, std::shared_ptr<EdgeFunction<V>>>::Cell
                     entry : endSumm) {
              N eP = entry.getRowKey();
              D d4 = entry.getColumnKey();
              std::shared_ptr<EdgeFunction<V>> fCalleeSummary =
                  entry.getValue();
              // for each return site
              for (N retSiteN : returnSiteNs) {
                // compute return-flow function
                std::shared_ptr<FlowFunction<D>> retFunction =
                    cachedFlowEdgeFunctions.getRetFlowFunction(n, sCalledProcN,
                                                               eP, retSiteN);
                INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
                std::set<D> returnedFacts = computeReturnFlowFunction(
                    retFunction, d3, d4, n, std::set<D>{d2});
                ADD_TO_HISTOGRAM("Data-flow facts", returnedFacts.size(), 1,
                                 PAMM_SEVERITY_LEVEL::Full);
                saveEdges(eP, retSiteN, d4, returnedFacts, true);
                // for each target value of the function
                for (D d5 : returnedFacts) {
                  // update the caller-side summary function
                  // get call edge function
                  std::shared_ptr<EdgeFunction<V>> f4 =
                      cachedFlowEdgeFunctions.getCallEdgeFunction(
                          n, d2, sCalledProcN, d3);
                  // get return edge function
                  std::shared_ptr<EdgeFunction<V>> f5 =
                      cachedFlowEdgeFunctions.getReturnEdgeFunction(
                          n, sCalledProcN, eP, d4, retSiteN, d5);
                  INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
                  // compose call * calleeSummary * return edge functions
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "Compose: " << f5->str() << " * "
                                << fCalleeSummary->str() << " * " << f4->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "         (return * calleeSummary * call)");
                  std::shared_ptr<EdgeFunction<V>> fPrime =
                      f4->composeWith(fCalleeSummary)->composeWith(f5);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "       = " << fPrime->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
                  D d5_restoredCtx = restoreContextOnReturnedFact(n, d2, d5);
                  // propagte the effects of the entire call
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "Compose: " << fPrime->str() << " * "
                                << f->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
        std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction =
            cachedFlowEdgeFunctions.getCallToRetFlowFunction(n, returnSiteN,
                                                             callees);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<D> returnFacts =
            computeCallToReturnFlowFunction(callToReturnFlowFunction, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", returnFacts.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        saveEdges(n, returnSiteN, d2, returnFacts, false);
        for (D d3 : returnFacts) {
          std::shared_ptr<EdgeFunction<V>> edgeFnE =
              cachedFlowEdgeFunctions.getCallToRetEdgeFunction(
                  n, d2, returnSiteN, d3, callees);
          INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Compose: " << edgeFnE->str() << " * " << f->str());
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Normal", 1, PAMM_SEVERITY_LEVEL::Full);
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process normal at target: "
                  << ideTabulationProblem.NtoString(edge.getTarget()));
    D d1 = edge.factAtSource();
    N n = edge.getTarget();
    D d2 = edge.factAtTarget();
    std::shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
    auto successorInst = icfg.getSuccsOf(n);
    for (auto m : successorInst) {
      std::shared_ptr<FlowFunction<D>> flowFunction =
          cachedFlowEdgeFunctions.getNormalFlowFunction(n, m);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      std::set<D> res = computeNormalFlowFunction(flowFunction, d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      saveEdges(n, m, d2, res, false);
      for (D d3 : res) {
        std::shared_ptr<EdgeFunction<V>> g =
            cachedFlowEdgeFunctions.getNormalEdgeFunction(n, d2, m, d3);
        std::shared_ptr<EdgeFunction<V>> fprime = f->composeWith(g);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Compose: " << g->str() << " * " << f->str());
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        propagate(d1, m, d3, fprime, nullptr, false);
      }
    }
  }

  void propagateValueAtStart(std::pair<N, D> nAndD, N n) {
    PAMM_GET_INSTANCE;
    D d = nAndD.second;
    M p = icfg.getMethodOf(n);
    for (N c : icfg.getCallsFromWithin(p)) {
      for (auto entry : jumpFn->forwardLookup(d, c)) {
        D dPrime = entry.first;
        std::shared_ptr<EdgeFunction<V>> fPrime = entry.second;
        N sP = n;
        V value = val(sP, d);
        INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
        propagateValue(c, dPrime, fPrime->computeTarget(value));
      }
    }
  }

  void propagateValueAtCall(std::pair<N, D> nAndD, N n) {
    PAMM_GET_INSTANCE;
    D d = nAndD.second;
    for (M q : icfg.getCalleesOfCallAt(n)) {
      std::shared_ptr<FlowFunction<D>> callFlowFunction =
          cachedFlowEdgeFunctions.getCallFlowFunction(n, q);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      for (D dPrime : callFlowFunction->computeTargets(d)) {
        std::shared_ptr<EdgeFunction<V>> edgeFn =
            cachedFlowEdgeFunctions.getCallEdgeFunction(n, d, q, dPrime);
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        for (N startPoint : icfg.getStartPointsOf(q)) {
          INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
          propagateValue(startPoint, dPrime, edgeFn->computeTarget(val(n, d)));
        }
      }
    }
  }

  void propagateValue(N nHashN, D nHashD, V v) {
    V valNHash = val(nHashN, nHashD);
    V vPrime = joinValueAt(nHashN, nHashD, valNHash, v);
    if (!(vPrime == valNHash)) {
      setVal(nHashN, nHashD, vPrime);
      valuePropagationTask(std::pair<N, D>(nHashN, nHashD));
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
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Function : "
                  << icfg.getMethodOf(nHashN)->getName().str());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Inst.    : " << ideTabulationProblem.NtoString(nHashN));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Fact     : " << ideTabulationProblem.DtoString(nHashD));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Value    : " << ideTabulationProblem.VtoString(l));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  }

  std::shared_ptr<EdgeFunction<V>> jumpFunction(PathEdge<N, D> edge) {
    if (!jumpFn->forwardLookup(edge.factAtSource(), edge.getTarget())
             .count(edge.factAtTarget())) {
      // JumpFn initialized to all-top, see line [2] in SRH96 paper
      return allTop;
    }
    return jumpFn->forwardLookup(edge.factAtSource(),
                                 edge.getTarget())[edge.factAtTarget()];
  }

  void addEndSummary(N sP, D d1, N eP, D d2,
                     std::shared_ptr<EdgeFunction<V>> f) {
    // note: at this point we don't need to join with a potential previous f
    // because f is a jump function, which is already properly joined
    // within propagate(..)
    endsummarytab.get(sP, d1).insert(eP, d2, f);
  }

  // should be made a callable at some point
  void pathEdgeProcessingTask(PathEdge<N, D> edge) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    INC_COUNTER("JumpFn Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg, DEBUG)
        << "-------------------------------------------- " << PathEdgeCount
        << ". Path Edge --------------------------------------------");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process " << PathEdgeCount << ". path edge:");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "< D source: "
                  << ideTabulationProblem.DtoString(edge.factAtSource())
                  << " ;");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "  N target: "
                  << ideTabulationProblem.NtoString(edge.getTarget()) << " ;");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "  D target: "
                  << ideTabulationProblem.DtoString(edge.factAtTarget())
                  << " >");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
  void valuePropagationTask(std::pair<N, D> nAndD) {
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
  void valueComputationTask(std::vector<N> values) {
    PAMM_GET_INSTANCE;
    for (N n : values) {
      for (N sP : icfg.getStartPointsOf(icfg.getMethodOf(n))) {
        Table<D, D, std::shared_ptr<EdgeFunction<V>>> lookupByTarget;
        lookupByTarget = jumpFn->lookupByTarget(n);
        for (typename Table<D, D, std::shared_ptr<EdgeFunction<V>>>::Cell
                 sourceValTargetValAndFunction : lookupByTarget.cellSet()) {
          D dPrime = sourceValTargetValAndFunction.getRowKey();
          D d = sourceValTargetValAndFunction.getColumnKey();
          std::shared_ptr<EdgeFunction<V>> fPrime =
              sourceValTargetValAndFunction.getValue();
          V targetVal = val(sP, dPrime);
          setVal(n, d,
                 ideTabulationProblem.join(val(n, d),
                                           fPrime->computeTarget(targetVal)));
          INC_COUNTER("Value Computation", 1, PAMM_SEVERITY_LEVEL::Full);
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
  unsigned PathEdgeCount;

  Table<N, N, std::map<D, std::set<D>>> computedIntraPathEdges;

  Table<N, N, std::map<D, std::set<D>>> computedInterPathEdges;

  std::shared_ptr<EdgeFunction<V>> allTop;

  std::shared_ptr<JumpFunctions<N, D, M, V, I>> jumpFn;

  // stores summaries that were queried before they were computed
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<N, D, Table<N, D, std::shared_ptr<EdgeFunction<V>>>> endsummarytab;

  // edges going along calls
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<N, D, std::map<N, std::set<D>>> incomingtab;

  // stores the return sites (inside callers) to which we have unbalanced
  // returns if followReturnPastSeeds is enabled
  std::set<N> unbalancedRetSites;

  std::map<N, std::set<D>> initialSeeds;

  Table<N, D, V> valtab;

  std::map<std::pair<N, D>, size_t> fSummaryReuse;

  // When transforming an IFDSTabulationProblem into an IDETabulationProblem,
  // we need to allocate dynamically, otherwise the objects lifetime runs out -
  // as a modifiable r-value reference created here that should be stored in a
  // modifiable l-value reference within the IDESolver implementation leads to
  // (massive) undefined behavior (and nightmares):
  // https://stackoverflow.com/questions/34240794/understanding-the-warning-binding-r-value-to-l-value-reference
  IDESolver(IFDSTabulationProblem<N, D, M, I> &tabulationProblem)
      : transformedProblem(
            std::make_unique<IFDSToIDETabulationProblem<N, D, M, I>>(
                tabulationProblem)),
        ideTabulationProblem(*transformedProblem),
        cachedFlowEdgeFunctions(ideTabulationProblem),
        recordEdges(ideTabulationProblem.solver_config.recordEdges),
        zeroValue(ideTabulationProblem.zeroValue()),
        icfg(ideTabulationProblem.interproceduralCFG()),
        computevalues(ideTabulationProblem.solver_config.computeValues),
        autoAddZero(ideTabulationProblem.solver_config.autoAddZero),
        followReturnPastSeeds(
            ideTabulationProblem.solver_config.followReturnsPastSeeds),
        computePersistedSummaries(
            ideTabulationProblem.solver_config.computePersistedSummaries),
        PathEdgeCount(0), allTop(ideTabulationProblem.allTopFunction()),
        jumpFn(std::make_shared<JumpFunctions<N, D, M, V, I>>(
            allTop, ideTabulationProblem)),
        initialSeeds(ideTabulationProblem.initialSeeds()) {
    // std::cout << "called IDESolver::IDESolver() ctor with IFDSProblem" <<
    // std::endl;
  }

  /**
   * Computes the final values for edge functions.
   */
  void computeValues() {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Start computing values");
    // Phase II(i)
    std::map<N, std::set<D>> allSeeds(initialSeeds);
    for (N unbalancedRetSite : unbalancedRetSites) {
      if (allSeeds[unbalancedRetSite].empty()) {
        allSeeds.insert(make_pair(unbalancedRetSite, std::set<D>({zeroValue})));
      }
    }
    // do processing
    for (const auto &seed : allSeeds) {
      N startPoint = seed.first;
      for (D val : seed.second) {
        setVal(startPoint, val, ideTabulationProblem.bottomElement());
        std::pair<N, D> superGraphNode(startPoint, val);
        valuePropagationTask(superGraphNode);
      }
    }
    // Phase II(ii)
    // we create an array of all nodes and then dispatch fractions of this array
    // to multiple threads
    std::set<N> allNonCallStartNodes = icfg.allNonCallStartNodes();
    std::vector<N> nonCallStartNodesArray(allNonCallStartNodes.size());
    size_t i = 0;
    for (N n : allNonCallStartNodes) {
      nonCallStartNodesArray[i] = n;
      i++;
    }
    valueComputationTask(nonCallStartNodesArray);
  }

  /**
   * Schedules the processing of initial seeds, initiating the analysis.
   * Clients should only call this methods if performing synchronization on
   * their own. Normally, solve() should be called instead.
   */
  void submitInitalSeeds() {
    auto &lg = lg::get();
    PAMM_GET_INSTANCE;
    for (const auto &seed : initialSeeds) {
      N startPoint = seed.first;
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Start point: "
                    << ideTabulationProblem.NtoString(startPoint));
      for (const D &value : seed.second) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "      Value: "
                      << ideTabulationProblem.DtoString(value));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        if (!ideTabulationProblem.isZeroValue(value)) {
          INC_COUNTER("Gen facts", 1, PAMM_SEVERITY_LEVEL::Core);
        }
        propagate(zeroValue, startPoint, value, EdgeIdentity<V>::getInstance(),
                  nullptr, false);
      }
      jumpFn->addFunction(zeroValue, startPoint, zeroValue,
                          EdgeIdentity<V>::getInstance());
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
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Exit", 1, PAMM_SEVERITY_LEVEL::Full);
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process exit at target: "
                  << ideTabulationProblem.NtoString(edge.getTarget()));
    N n = edge.getTarget(); // an exit node; line 21...
    std::shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
    M methodThatNeedsSummary = icfg.getMethodOf(n);
    D d1 = edge.factAtSource();
    D d2 = edge.factAtTarget();
    // for each of the method's start points, determine incoming calls
    std::set<N> startPointsOf = icfg.getStartPointsOf(methodThatNeedsSummary);
    std::map<N, std::set<D>> inc;
    for (N sP : startPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      addEndSummary(sP, d1, n, d2, f);
      for (auto entry : incoming(d1, sP)) {
        inc[entry.first] = std::set<D>{entry.second};
      }
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
        std::shared_ptr<FlowFunction<D>> retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                c, methodThatNeedsSummary, n, retSiteC);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        // for each incoming-call value
        for (D d4 : entry.second) {
          std::set<D> targets =
              computeReturnFlowFunction(retFunction, d1, d2, c, entry.second);
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, retSiteC, d2, targets, true);
          // for each target value at the return site
          // line 23
          for (D d5 : targets) {
            // compute composed function
            // get call edge function
            std::shared_ptr<EdgeFunction<V>> f4 =
                cachedFlowEdgeFunctions.getCallEdgeFunction(
                    c, d4, icfg.getMethodOf(n), d1);
            // get return edge function
            std::shared_ptr<EdgeFunction<V>> f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
            INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
            // compose call function * function * return function
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << f5->str() << " * " << f->str()
                          << " * " << f4->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "         (return * function * call)");
            std::shared_ptr<EdgeFunction<V>> fPrime =
                f4->composeWith(f)->composeWith(f5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "       = " << fPrime->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
            // for each jump function coming into the call, propagate to return
            // site using the composed function
            for (auto valAndFunc : jumpFn->reverseLookup(c, d4)) {
              std::shared_ptr<EdgeFunction<V>> f3 = valAndFunc.second;
              if (!f3->equal_to(allTop)) {
                D d3 = valAndFunc.first;
                D d5_restoredCtx = restoreContextOnReturnedFact(c, d4, d5);
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                              << "Compose: " << fPrime->str() << " * "
                              << f3->str());
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
      std::set<N> callers = icfg.getCallersOf(methodThatNeedsSummary);
      for (N c : callers) {
        for (N retSiteC : icfg.getReturnSitesOfCallAt(c)) {
          std::shared_ptr<FlowFunction<D>> retFunction =
              cachedFlowEdgeFunctions.getRetFlowFunction(
                  c, methodThatNeedsSummary, n, retSiteC);
          INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          std::set<D> targets = computeReturnFlowFunction(
              retFunction, d1, d2, c, std::set<D>{zeroValue});
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, retSiteC, d2, targets, true);
          for (D d5 : targets) {
            std::shared_ptr<EdgeFunction<V>> f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
            INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << f5->str() << " * " << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
        std::shared_ptr<FlowFunction<D>> retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                nullptr, methodThatNeedsSummary, n, nullptr);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        retFunction->computeTargets(d2);
      }
    }
  }

  void
  propagteUnbalancedReturnFlow(N retSiteC, D targetVal,
                               std::shared_ptr<EdgeFunction<V>> edgeFunction,
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
  std::set<D>
  computeNormalFlowFunction(std::shared_ptr<FlowFunction<D>> flowFunction, D d1,
                            D d2) {
    return flowFunction->computeTargets(d2);
  }

  /**
   * TODO: comment
   */
  std::set<D> computeSummaryFlowFunction(
      std::shared_ptr<FlowFunction<D>> SummaryFlowFunction, D d1, D d2) {
    return SummaryFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the call flow function for the given call-site abstraction
   * @param callFlowFunction The call flow function to compute
   * @param d1 The abstraction at the current method's start node.
   * @param d2 The abstraction at the call site
   * @return The set of caller-side abstractions at the callee's start node
   */
  std::set<D>
  computeCallFlowFunction(std::shared_ptr<FlowFunction<D>> callFlowFunction,
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
  std::set<D> computeCallToReturnFlowFunction(
      std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction, D d1, D d2) {
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
  std::set<D>
  computeReturnFlowFunction(std::shared_ptr<FlowFunction<D>> retFunction, D d1,
                            D d2, N callSite, std::set<D> callerSideDs) {
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
  propagate(D sourceVal, N target, D targetVal,
            std::shared_ptr<EdgeFunction<V>> f,
            /* deliberately exposed to clients */ N relatedCallSite,
            /* deliberately exposed to clients */ bool isUnbalancedReturn) {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Propagate flow");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Source value  : "
                  << ideTabulationProblem.DtoString(sourceVal));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Target        : "
                  << ideTabulationProblem.NtoString(target));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Target value  : "
                  << ideTabulationProblem.DtoString(targetVal));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Edge function : " << f.get()->str()
                  << " (result of previous compose)");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    std::shared_ptr<EdgeFunction<V>> jumpFnE = nullptr;
    std::shared_ptr<EdgeFunction<V>> fPrime;
    if (!jumpFn->reverseLookup(target, targetVal).empty()) {
      jumpFnE = jumpFn->reverseLookup(target, targetVal)[sourceVal];
    }
    if (jumpFnE == nullptr) {
      jumpFnE = allTop; // jump function is initialized to all-top
    }
    fPrime = jumpFnE->joinWith(f);
    bool newFunction = !(fPrime->equal_to(jumpFnE));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Join: " << jumpFnE->str() << " & " << f.get()->str()
                  << (jumpFnE->equal_to(f) ? " (EF's are equal)" : " "));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "    = " << fPrime->str()
                  << (newFunction ? " (new jump func)" : " "));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    if (newFunction) {
      jumpFn->addFunction(sourceVal, target, targetVal, fPrime);
      PathEdge<N, D> edge(sourceVal, target, targetVal);
      PathEdgeCount++;
      pathEdgeProcessingTask(edge);
      if (!ideTabulationProblem.isZeroValue(targetVal)) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "EDGE: <F: " << target->getFunction()->getName().str()
                      << ", D: " << ideTabulationProblem.DtoString(sourceVal)
                      << ">");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << " ---> <N: " << ideTabulationProblem.NtoString(target)
                      << ",");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "       D: "
                      << ideTabulationProblem.DtoString(targetVal) << ">");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      }
    } else {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "PROPAGATE: No new function!");
    }
  }

  V joinValueAt(N unit, D fact, V curr, V newVal) {
    return ideTabulationProblem.join(curr, newVal);
  }

  std::set<typename Table<N, D, std::shared_ptr<EdgeFunction<V>>>::Cell>
  endSummary(N sP, D d3) {
    if (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      auto key = std::make_pair(sP, d3);
      auto findND = fSummaryReuse.find(key);
      if (findND == fSummaryReuse.end()) {
        fSummaryReuse.insert(std::make_pair(key, 0));
      } else {
        fSummaryReuse[key] += 1;
      }
    }
    return endsummarytab.get(sP, d3).cellSet();
  }

  std::map<N, std::set<D>> incoming(D d1, N sP) {
    return incomingtab.get(sP, d1);
  }

  void addIncoming(N sP, D d3, N n, D d2) {
    incomingtab.get(sP, d3)[n].insert(d2);
  }

  void printIncomingTab() {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Start of incomingtab entry");
    for (auto cell : incomingtab.cellSet()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "sP: " << ideTabulationProblem.NtoString(cell.r));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "d3: " << ideTabulationProblem.DtoString(cell.c));
      for (auto entry : cell.v) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  n: "
                      << ideTabulationProblem.NtoString(entry.first));
        for (auto fact : entry.second) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "  d2: " << ideTabulationProblem.DtoString(fact));
        }
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "---------------");
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "End of incomingtab entry");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  }

  void printEndSummaryTab() {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Start of endsummarytab entry");
    for (auto cell : endsummarytab.cellVec()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "sP: " << ideTabulationProblem.NtoString(cell.r));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "d1: " << ideTabulationProblem.DtoString(cell.c));
      for (auto inner_cell : cell.v.cellVec()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  eP: "
                      << ideTabulationProblem.NtoString(inner_cell.r));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  d2: "
                      << ideTabulationProblem.DtoString(inner_cell.c));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  EF: " << inner_cell.v->str());
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "---------------");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "End of endsummarytab entry");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  }

  /**
   * The invariant for computing the number of generated (#gen) and killed
   * (#kill) facts:
   *   (1) #Valid facts at the last statement <= #gen - #kill
   *   (2) #gen >= #kill
   *
   * The total number of valid facts can be smaller than the difference of
   * generated and killed facts, due to set semantics, i.e. a fact can be
   * generated multiple times but appears only once.
   *
   * Zero value is not counted!
   *
   * @brief Computes and prints statistics of the analysis run, e.g. number of
   * generated/killed facts, number of summary-reuses etc.
   */
  void computeAndPrintStatistics() {
    auto &lg = lg::get();
    PAMM_GET_INSTANCE;
    // Stores all valid facts at return site in caller context; return-site is
    // key
    std::unordered_map<N, std::set<D>> ValidInCallerContext;
    std::size_t genFacts = 0, killFacts = 0, intraPathEdges = 0,
                interPathEdges = 0;
    /* --- Intra-procedural Path Edges ---
     * d1 --> d2-Set
     * Case 1: d1 in d2-Set
     * Case 2: d1 not in d2-Set, i.e. d1 was killed. d2-Set could be empty.
     */
    for (auto cell : computedIntraPathEdges.cellSet()) {
      auto Edge = std::make_pair(cell.r, cell.c);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N1: " << ideTabulationProblem.NtoString(Edge.first));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N2: " << ideTabulationProblem.NtoString(Edge.second));
      for (auto D1ToD2Set : cell.v) {
        auto D1 = D1ToD2Set.first;
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "d1: " << ideTabulationProblem.DtoString(D1));
        auto D2Set = D1ToD2Set.second;
        intraPathEdges += D2Set.size();
        // Case 1
        if (D2Set.find(D1) != D2Set.end()) {
          genFacts += D2Set.size() - 1;
        }
        // Case 2
        else {
          genFacts += D2Set.size();
          // We ignore the zero value
          if (!ideTabulationProblem.isZeroValue(D1)) {
            killFacts++;
          }
        }
        // Store all valid facts after call-to-return flow
        if (icfg.isCallStmt(Edge.first)) {
          ValidInCallerContext[Edge.second].insert(D2Set.begin(), D2Set.end());
        }
        for (auto D2 : D2Set) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "d2: " << ideTabulationProblem.DtoString(D2));
        }
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----");
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    }

    // Stores all pairs of (Startpoint, Fact) for which a summary was applied
    std::set<std::pair<N, D>> ProcessSummaryFacts;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "==============================================");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "INTER PATH EDGES");
    for (auto cell : computedInterPathEdges.cellSet()) {
      auto Edge = std::make_pair(cell.r, cell.c);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N1: " << ideTabulationProblem.NtoString(Edge.first));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N2: " << ideTabulationProblem.NtoString(Edge.second));
      /* --- Call-flow Path Edges ---
       * Case 1: d1 --> empty set
       *   Can be ignored, since killing a fact in the caller context will
       *   actually happen during  call-to-return.
       *
       * Case 2: d1 --> d2-Set
       *   Every fact d_i != zeroValue in d2-set will be generated in the callee
       * context, thus counts as a new fact. Even if d1 is passed as it is, it
       * will count as a new fact. The reason for this is, that d1 can be
       * killed in the callee context, but still be valid in the caller
       * context.
       *
       * Special Case: Summary was applied for a particular call
       *   Process the summary's #gen and #kill.
       */
      if (icfg.isCallStmt(Edge.first)) {
        for (auto D1ToD2Set : cell.v) {
          auto D1 = D1ToD2Set.first;
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "d1: " << ideTabulationProblem.DtoString(D1));
          auto DSet = D1ToD2Set.second;
          interPathEdges += DSet.size();
          for (auto D2 : DSet) {
            if (!ideTabulationProblem.isZeroValue(D2)) {
              genFacts++;
            }
            // Special case
            if (ProcessSummaryFacts.find(std::make_pair(Edge.second, D2)) !=
                ProcessSummaryFacts.end()) {
              std::multiset<D> SummaryDMultiSet =
                  endsummarytab.get(Edge.second, D2).columnKeySet();
              // remove duplicates from multiset
              std::set<D> SummaryDSet(SummaryDMultiSet.begin(),
                                      SummaryDMultiSet.end());
              // Process summary just as an intra-procedural edge
              if (SummaryDSet.find(D2) != SummaryDSet.end()) {
                genFacts += SummaryDSet.size() - 1;
              } else {
                genFacts += SummaryDSet.size();
                // We ignore the zero value
                if (!ideTabulationProblem.isZeroValue(D1)) {
                  killFacts++;
                }
              }
            } else {
              ProcessSummaryFacts.insert(std::make_pair(Edge.second, D2));
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "d2: " << ideTabulationProblem.DtoString(D2));
          }
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----");
        }
      }
      /* --- Return-flow Path Edges ---
       * Since every fact passed to the callee was counted as a new fact, we
       * have to count every fact propagated to the caller as a kill to satisfy
       * our invariant. Obviously, every fact not propagated to the caller will
       * count as a kill. If an actual new fact is propagated to the caller, we
       * have to increase the number of generated facts by one. Zero value does
       * not count towards generated/killed facts.
       */
      if (icfg.isExitStmt(cell.r)) {
        for (auto D1ToD2Set : cell.v) {
          auto D1 = D1ToD2Set.first;
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "d1: " << ideTabulationProblem.DtoString(D1));
          auto DSet = D1ToD2Set.second;
          interPathEdges += DSet.size();
          auto CallerFacts = ValidInCallerContext[Edge.second];
          for (auto D2 : DSet) {
            // d2 not valid in caller context
            if (CallerFacts.find(D2) == CallerFacts.end()) {
              genFacts++;
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "d2: " << ideTabulationProblem.DtoString(D2));
          }
          if (!ideTabulationProblem.isZeroValue(D1)) {
            killFacts++;
          }
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----");
        }
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    }

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "SUMMARY REUSE");
    std::size_t TotalSummaryReuse = 0;
    for (auto entry : fSummaryReuse) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N1: "
                    << ideTabulationProblem.NtoString(entry.first.first));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "D1: "
                    << ideTabulationProblem.DtoString(entry.first.second));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "#Reuse: " << entry.second);
      TotalSummaryReuse += entry.second;
    }

    INC_COUNTER("Gen facts", genFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Kill facts", killFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Summary-reuse", TotalSummaryReuse, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Intra Path Edges", intraPathEdges, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Inter Path Edges", interPathEdges, PAMM_SEVERITY_LEVEL::Core);

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "----------------------------------------------");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "=== Solver Statistics ===");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Facts generated : " << GET_COUNTER("Gen facts"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Facts killed    : " << GET_COUNTER("Kill facts"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Summary-reuse   : " << GET_COUNTER("Summary-reuse"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Intra Path Edges: " << GET_COUNTER("Intra Path Edges"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Inter Path Edges: " << GET_COUNTER("Inter Path Edges"));
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Flow function query count: "
                                            << GET_COUNTER("FF Queries"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Edge function query count: "
                                            << GET_COUNTER("EF Queries"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Data-flow value propagation count: "
                    << GET_COUNTER("Value Propagation"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Data-flow value computation count: "
                    << GET_COUNTER("Value Computation"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Special flow function usage count: "
                    << GET_COUNTER("SpecialSummary-FF Application"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Jump function construciton count: "
                    << GET_COUNTER("JumpFn Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Phase I duration: " << PRINT_TIMER("DFA Phase I"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Phase II duration: " << PRINT_TIMER("DFA Phase II"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "----------------------------------------------");
      cachedFlowEdgeFunctions.print();
    }
  }
};

} // namespace psr

#endif
