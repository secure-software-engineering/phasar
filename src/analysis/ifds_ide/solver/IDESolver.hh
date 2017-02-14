/*
 * IDESolver.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_IDESOLVER_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_IDESOLVER_HH_

#include <set>
#include <map>
#include <utility>
#include <memory>
#include <type_traits>
#include <chrono>

#include "PathEdge.hh"
#include "LinkedNode.hh"
#include "JoinHandlingNode.hh"
#include "../IDETabluationProblem.hh"
#include "../IDETabulationProblemWZeroedFF.hh"
#include "../JoinLattice.hh"
#include "../FlowFunctions.hh"
#include "../EdgeFunctions.hh"
#include "../EdgeFunction.hh"
#include "../edge_func/EdgeIdentity.hh"
#include "../solver/JumpFunctions.hh"
#include "../../../utils/Table.hh"
#include "../ZeroedFlowFunction.hh"

using namespace std;

/**
 * Solves the given {@link IDETabulationProblem} as described in the 1996 paper by Sagiv,
 * Horwitz and Reps. To solve the problem, call {@link #solve()}. Results can then be
 * queried by using {@link #resultAt(Object, Object)} and {@link #resultsAt(Object)}.
 *
 * Note that this solver and its data structures internally use mostly {@link java.util.LinkedHashSet}s
 * instead of normal {@link HashSet}s to fix the iteration order as much as possible. This
 * is to produce, as much as possible, reproducible benchmarking results. We have found
 * that the iteration order can matter a lot in terms of speed.
 *
 * @param <N> The type of nodes in the interprocedural control-flow graph.
 * @param <D> The type of data-flow facts to be computed by the tabulation problem.
 * @param <M> The type of objects used to represent methods.
 * @param <V> The type of values to be computed along flow edges.
 * @param <I> The type of inter-procedural control-flow graph being used.
 */
template<class N, class D, class M, class V, class I>
class IDESolver {
public:
	IDESolver(IDETabluationProblem<N,D,M,V,I>&& tabulationProblem) : ideTabluationProblem(
																		tabulationProblem.autoAddZero() ?
																		//IDETabluationProblem<N,D,M,V,I>(IDETabulationProblemWAutoZeroedFF<N,D,M,V,I>(tabulationProblem)) :
																		tabulationProblem :
																		tabulationProblem),
																	 recordEdges(tabulationProblem.recordEdges()),
																	 zeroValue(tabulationProblem.zeroValue()),
																	 icfg(tabulationProblem.interproceduralCFG()),
																	 computevalues(tabulationProblem.computeValues()),
																	 followReturnPastSeeds(tabulationProblem.followReturnsPastSeeds()),
																	 allTop(tabulationProblem.allTopFunction()),
																	 jumpFn(make_shared<JumpFunctions<N,D,V>>(allTop)),
																	 initialSeeds(tabulationProblem.initialSeeds())
	{
		cout << "called IDESolver ctor" << endl;
		// If user wishes to auto add zero value, promote IDETabulationProblem to an IDETabulationProblemWAutoZeroedFF
		if (tabulationProblem.autoAddZero()) {
		//	cout << "AUTO ADD ZERO: TRUE" << endl;
		//	ideTabluationProblem = IDETabulationProblemWAutoZeroedFF<N,D,M,V,I>(tabulationProblem);
		}
	}

	IDESolver(IDETabluationProblem<N,D,M,V,I>& tabulationProblem) : ideTabluationProblem(tabulationProblem),
																		 recordEdges(tabulationProblem.recordEdges()),
																		 zeroValue(tabulationProblem.zeroValue()),
																		 icfg(tabulationProblem.interproceduralCFG()),
																		 computevalues(tabulationProblem.computeValues()),
																		 followReturnPastSeeds(tabulationProblem.followReturnsPastSeeds()),
																		 allTop(tabulationProblem.allTopFunction()),
																		 jumpFn(make_shared<JumpFunctions<N,D,V>>(allTop)),
																		 initialSeeds(tabulationProblem.initialSeeds())
	{
		cout << "called IDESolver ctor" << endl;
		if (tabulationProblem.autoAddZero()) {
		//	cout << "AUTO ADD ZERO: TRUE" << endl;
		//	ideTabluationProblem = IDETabulationProblemWAutoZeroedFF<N,D,M,V,I>(tabulationProblem);
		}
	}

	virtual ~IDESolver() = default;

	/**
	 * Runs the solver on the configured problem. This can take some time.
	 */
	virtual void solve()
	{
		cout << "IDE solver is solving this shit!!!" << endl;
//		// test flow functions
//		auto normal = ideTabluationProblem.getNormalFlowFunction(0, 0);
//		auto call = ideTabluationProblem.getCallFlowFuntion(0, 0);
//		auto ret = ideTabluationProblem.getRetFlowFunction(0, 0, 0, 0);
//		auto calltoret = ideTabluationProblem.getCallToRetFlowFunction(0, 0);
//		// test edge functions
//		auto normal_edge = ideTabluationProblem.getNormalEdgeFunction(0, 0, 0, 0);
//		auto call_edge = ideTabluationProblem.getCallEdgeFunction(0, 0, 0, 0);
//		auto ret_edge = ideTabluationProblem.getReturnEdgeFunction(0, 0, 0, 0, 0, 0);
//		auto calltoret_edge = ideTabluationProblem.getCallToReturnEdgeFunction(0, 0, 0, 0);

		// computations starting here
		auto startFlowFunctionConstruction = chrono::high_resolution_clock::now();
		// we start our analysis and construct exploded supergraph
		submitInitalSeeds();
		cout << "AFTER INITIAL SEEDS" << endl;
		auto endFlowFunctionConstruction = chrono::high_resolution_clock::now();
		durationFlowFunctionConstruction = chrono::duration_cast<chrono::milliseconds>(endFlowFunctionConstruction-startFlowFunctionConstruction);
		if (computevalues) {
			auto startFlowFunctionApplication = chrono::high_resolution_clock::now();
			// apply the flow functions to compute the data flow facts
			computeValues();
			auto endFlowFunctionApplication = chrono::high_resolution_clock::now();
			durationFlowFunctionApplication = chrono::duration_cast<chrono::milliseconds>(endFlowFunctionApplication-startFlowFunctionApplication);
		}
		cout << "@ finished solving IDE problem" << endl;
		cout << "@ statistics" << endl;
		cout << "@ flowFunctionsConstructionCount: " << flowFunctionConstructionCount << endl;
		cout << "@ flowFunctionsApplicationCount: " << flowFunctionApplicationCount << endl;
		cout << "@ propagationCount: " << propagationCount << endl;
		cout << "@ flow function construction: " << durationFlowFunctionConstruction.count() << " ms" << endl;
		cout << "@ flow function application: " << durationFlowFunctionApplication.count() << " ms" << endl;
	}

	/**
	 * Returns the V-type result for the given value at the given statement.
	 * TOP values are never returned.
	 */
	V resultAt(N stmt, D value)
	{
		return valtab.get(stmt, value);
	}

	/**
	 * Returns the resulting environment for the given statement.
	 * The artificial zero value is automatically stripped. TOP values are
	 * never returned.
	 */
	unordered_map<D,V> resultsAt(N stmt)
	{
		unordered_map<D, V> result = valtab.row(stmt);
		for (auto pair : result) {
			if (pair.first == zeroValue)
				result.erase(pair.first);
		}
		return result;
	}

private:
	IDETabluationProblem<N,D,M,V,I>& ideTabluationProblem;
	bool recordEdges;
	size_t flowFunctionConstructionCount = 0;
	size_t flowFunctionApplicationCount = 0;
	size_t propagationCount = 0;
	chrono::milliseconds durationFlowFunctionConstruction;
	chrono::milliseconds durationFlowFunctionApplication;

	void saveEdges(N sourceNode, N sinkStmt, D sourceVal, set<D> destVals, bool interP)
	{
		if (!recordEdges) return;
		Table<N, N, map<D, set<D>>>& tgtMap = (interP) ? computedInterPathEdges : computedIntraPathEdges;
		map<D, set<D>>& m = tgtMap.get(sourceNode, sinkStmt);
		if (m.empty()) {
			tgtMap.insert(sourceNode, sinkStmt, m);
		}
		m[sourceVal] = set<D>{destVals};
	}

	/**
	 * Lines 13-20 of the algorithm; processing a call site in the caller's context.
	 *
	 * For each possible callee, registers incoming call edges.
	 * Also propagates call-to-return flows and summarized callee flows within the caller.
	 *
	 * @param edge an edge whose target node resembles a method call
	 */
	void processCall(PathEdge<N,D> edge)
	{
		cout << "process call edge" << endl;
		D d1 = edge.factAtSource();
		N n = edge.getTarget(); // a call node; line 14...
		cout << "processing call to {}" << endl;
		n->dump();
		D d2 = edge.factAtTarget();
		shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
		set<N> returnSiteNs = icfg.getReturnSitesOfCallAt(n);

		//for each possible callee
		set<M> callees = icfg.getCalleesOfCallAt(n);
		for(M sCalledProcN : callees) { //still line 14

			//compute the call-flow function
			shared_ptr<FlowFunction<D>> function = ideTabluationProblem.getCallFlowFuntion(n, sCalledProcN);
			flowFunctionConstructionCount++;
			set<D> res = computeCallFlowFunction(function, d1, d2);
			//for each callee's start point(s)
			set<N> startPointsOf = icfg.getStartPointsOf(sCalledProcN);
			for(N sP : startPointsOf) {
				saveEdges(n, sP, d2, res, true);
				//for each result node of the call-flow function
				for(D d3 : res) {
					//create initial self-loop
					propagate(d3, sP, d3, EdgeIdentity<V>::v(), n, false); //line 15
					//register the fact that <sp,d3> has an incoming edge from <n,d2>
					set<typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell> endSumm;

					//line 15.1 of Naeem/Lhotak/Rodriguez
					addIncoming(sP,d3,n,d2);
					//line 15.2, copy to avoid concurrent modification exceptions by other threads
					endSumm = set<typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell>(endSummary(sP, d3));


					//still line 15.2 of Naeem/Lhotak/Rodriguez
					//for each already-queried exit value <eP,d4> reachable from <sP,d3>,
					//create new caller-side jump functions to the return sites
					//because we have observed a potentially new incoming edge into <sP,d3>
					for (typename Table<N, D, shared_ptr<EdgeFunction<V>>>::Cell entry : endSumm) {
						N eP = entry.getRowKey();
						D d4 = entry.getColumnKey();
						shared_ptr<EdgeFunction<V>> fCalleeSummary = entry.getValue();
						//for each return site
						for (N retSiteN : returnSiteNs) {
							//compute return-flow function
							shared_ptr<FlowFunction<D>> retFunction = ideTabluationProblem.getRetFlowFunction(n, sCalledProcN, eP, retSiteN);
							flowFunctionConstructionCount++;
							set<D> returnedFacts = computeReturnFlowFunction(retFunction, d3, d4, n, set<D>{d2});
							saveEdges(eP, retSiteN, d4, returnedFacts, true);
							//for each target value of the function
							for(D d5 : returnedFacts) {
								//update the caller-side summary function
								shared_ptr<EdgeFunction<V>> f4 = ideTabluationProblem.getCallEdgeFunction(n, d2, sCalledProcN, d3);
								shared_ptr<EdgeFunction<V>> f5 = ideTabluationProblem.getReturnEdgeFunction(n, sCalledProcN, eP, d4, retSiteN, d5);
								shared_ptr<EdgeFunction<V>> fPrime = f4->composeWith(fCalleeSummary)->composeWith(f5);
								D d5_restoredCtx = restoreContextOnReturnedFact(n, d2, d5);
								propagate(d1, retSiteN, d5_restoredCtx, f->composeWith(fPrime), n, false);
							}
						}
					}
				}
			}
		}
		//line 17-19 of Naeem/Lhotak/Rodriguez
		//process intra-procedural flows along call-to-return flow functions
		for (N returnSiteN : returnSiteNs) {
			shared_ptr<FlowFunction<D>> callToReturnFlowFunction = ideTabluationProblem.getCallToRetFlowFunction(n, returnSiteN);
			flowFunctionConstructionCount++;
			set<D> returnFacts = computeCallToReturnFlowFunction(callToReturnFlowFunction, d1, d2);
			saveEdges(n, returnSiteN, d2, returnFacts, false);
			for(D d3 : returnFacts) {
				shared_ptr<EdgeFunction<V>> edgeFnE = ideTabluationProblem.getCallToReturnEdgeFunction(n, d2, returnSiteN, d3);
				propagate(d1, returnSiteN, d3, f->composeWith(edgeFnE), n, false);
			}
		}
	}

	/**
	 * Lines 33-37 of the algorithm.
	 * Simply propagate normal, intra-procedural flows.
	 * @param edge
	 */
	void processNormalFlow(PathEdge<N,D> edge)
	{
		cout << "@ process normal flow edge starting from: " << endl;
		if (edge.factAtSource() == nullptr)
			cout << "fact at source is nullptr" << endl;
		D d1 = edge.factAtSource();
		N n = edge.getTarget();
		D d2 = edge.factAtTarget();
		shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
		auto successorInst = icfg.getSuccsOf(n);
		for (auto m : successorInst) {
			cout << "@ successors: " << endl;
			m->dump();
			shared_ptr<FlowFunction<D>> flowFunction = ideTabluationProblem.getNormalFlowFunction(n,m);
			flowFunctionConstructionCount++;
			set<D> res = computeNormalFlowFunction(flowFunction, d1, d2);
			cout << "results" << endl;
			for (auto r : res)
				if (r)
					r->dump();
			saveEdges(n, m, d2, res, false);
			for (D d3 : res) {
				shared_ptr<EdgeFunction<V>> fprime = f->composeWith(ideTabluationProblem.getNormalEdgeFunction(n, d2, m, d3));
				propagate(d1, m, d3, fprime, nullptr, false);
			}
		}
	}

	/**
	 * Computes the final values for edge functions.
	 */
	void computeValues()
	{
		cout << "@ start computeValues()" << endl;
		//Phase II(i)
		map<N, set<D>> allSeeds(initialSeeds);
		for (N unbalancedRetSite : unbalancedRetSites) {
			set<D> seeds = allSeeds[unbalancedRetSite];
			if (seeds.empty()) {
				allSeeds.insert({unbalancedRetSite, seeds});
			}
			seeds.insert(zeroValue);
		}
		//do processing
		for (const auto& seed : allSeeds) {
			N startPoint = seed.first;
			for (D val : seed.second) {
				setVal(startPoint, val, ideTabluationProblem.bottomElement());
				pair<N,D> superGraphNode(startPoint, val);
				valuePropagationTask(superGraphNode);
			}
		}
		//Phase II(ii)
		//we create an array of all nodes and then dispatch fractions of this array to multiple threads
		set<N> allNonCallStartNodes = icfg.allNonCallStartNodes();
		vector<N> nonCallStartNodesArray(allNonCallStartNodes.size());
		size_t i = 0;
		for (N n : allNonCallStartNodes) {
			nonCallStartNodesArray[i] = n;
			i++;
		}
		valueComputationTask(nonCallStartNodesArray);
	}

	void propagateValueAtStart(pair<N,D> nAndD, N n)
	{
		D d = nAndD.second;
		M p = icfg.getMethodOf(n);
		for (N c : icfg.getCallsFromWithin(p)) {
			for (auto entry : jumpFn->forwardLookup(d,c)) {
				D dPrime = entry.first;
				shared_ptr<EdgeFunction<V>> fPrime = entry.second;
				N sP = n;
				V value = val(sP,d);
				propagateValue(c, dPrime, fPrime->computeTarget(value));
				flowFunctionApplicationCount++;
			}
		}
	}

	void propagateValueAtCall(pair<N,D> nAndD, N n)
	{
		D d = nAndD.second;
		for (M q : icfg.getCalleesOfCallAt(n)) {
			shared_ptr<FlowFunction<D>> callFlowFunction = ideTabluationProblem.getCallFlowFuntion(n, q);
			flowFunctionConstructionCount++;
			for (D dPrime : callFlowFunction->computeTargets(d)) {
				shared_ptr<EdgeFunction<V>> edgeFn = ideTabluationProblem.getCallEdgeFunction(n, d, q, dPrime);
				for (N startPoint : icfg.getStartPointsOf(q)) {
					propagateValue(startPoint, dPrime, edgeFn->computeTarget(val(n,d)));
					flowFunctionApplicationCount++;
				}
			}
		}
	}

	void propagateValue(N nHashN, D nHashD, V v)
	{
		V valNHash = val(nHashN, nHashD);
		V vPrime = joinValueAt(nHashN, nHashD, valNHash,v);
		if(!(vPrime == valNHash)) {
			setVal(nHashN, nHashD, vPrime);
			valuePropagationTask(pair<N,D>(nHashN,nHashD));
		}
	}

	V val(N nHashN, D nHashD)
	{
		//V l = valtab.get(nHashN, nHashD);
		//if(l == nullptr) return ideTabluationProblem.topElement(); //implicitly initialized to top; see line [1] of Fig. 7 in SRH96 paper
		//return l;
		if (valtab.contains(nHashN, nHashD))
			return valtab.get(nHashN, nHashD);
		else
			return ideTabluationProblem.topElement();
	}

	void setVal(N nHashN, D nHashD, V l)
	{
		// TOP is the implicit default value which we do not need to store.
		if (l == ideTabluationProblem.topElement())     // do not store top values
			valtab.remove(nHashN, nHashD);
		else
			valtab.insert(nHashN, nHashD, l);
		// logger.debug("VALUE: {} {} {} {}", icfg.getMethodOf(nHashN, nHashN, nHashD, l));
	}

	shared_ptr<EdgeFunction<V>> jumpFunction(PathEdge<N,D> edge)
	{
		if (!jumpFn->forwardLookup(edge.factAtSource(), edge.getTarget()).count(edge.factAtTarget()))
			return allTop; //JumpFn initialized to all-top, see line [2] in SRH96 paper
		return jumpFn->forwardLookup(edge.factAtSource(), edge.getTarget())[edge.factAtTarget()];
	}

	void addEndSummary(N sP, D d1, N eP, D d2, shared_ptr<EdgeFunction<V>> f)
	{
		Table<N, D, shared_ptr<EdgeFunction<V>>>& summaries = endsummarytab.get(sP, d1);
		//note: at this point we don't need to join with a potential previous f
		//because f is a jump function, which is already properly joined
		//within propagate(..)
		summaries.insert(eP,d2,f);
	}

	void pathEdgeProcessingTask(PathEdge<N,D> edge) // should be made a callable at some point
	{
		propagationCount++;
		if (icfg.isCallStmt(edge.getTarget())) {
			cout << "@ process call" << endl;
			// TODO fix processing call correctly!
			processCall(edge);
		} else {
			if (icfg.isExitStmt(edge.getTarget())) {
				cout << "@ process exit" << endl;
				processExit(edge);
			}
			if (!icfg.getSuccsOf(edge.getTarget()).empty()) {
				cout << "@ process normal flow" << endl;
				processNormalFlow(edge);
			}
		}
	}

	void valuePropagationTask(pair<N,D> nAndD) // should be made a callable at some point
	{
		N n = nAndD.first;
		//our initial seeds are not necessarily method-start points but here they should be treated as such
		//the same also for unbalanced return sites in an unbalanced problem
		if (icfg.isStartPoint(n) || initialSeeds.count(n) || unbalancedRetSites.count(n)) {
			propagateValueAtStart(nAndD, n);
		}
		if (icfg.isCallStmt(n)) {
			propagateValueAtCall(nAndD, n);
		}
	}

	void valueComputationTask(vector<N> values) // should be made a callable at some point
	{
		for(N n : values) {
			for(N sP: icfg.getStartPointsOf(icfg.getMethodOf(n))) {
				Table<D, D, shared_ptr<EdgeFunction<V>>> lookupByTarget;
				lookupByTarget = jumpFn->lookupByTarget(n);
				for(typename Table<D, D, shared_ptr<EdgeFunction<V>>>::Cell sourceValTargetValAndFunction : lookupByTarget.cellSet()) {
					D dPrime = sourceValTargetValAndFunction.getRowKey();
					D d = sourceValTargetValAndFunction.getColumnKey();
					shared_ptr<EdgeFunction<V>> fPrime = sourceValTargetValAndFunction.getValue();
					V targetVal = val(sP, dPrime);
					setVal(n,d,ideTabluationProblem.join(val(n,d),fPrime->computeTarget(targetVal)));
					flowFunctionApplicationCount++;
				}
			}
		}
	}

protected:
	D zeroValue;
	I icfg;
	bool computevalues;
	bool followReturnPastSeeds;

	Table<N,N,map<D,set<D>>> computedIntraPathEdges;

	Table<N,N,map<D,set<D>>> computedInterPathEdges;

	shared_ptr<EdgeFunction<V>> allTop;

	shared_ptr<JumpFunctions<N,D,V>> jumpFn;

	//stores summaries that were queried before they were computed
	//see CC 2010 paper by Naeem, Lhotak and Rodriguez
	Table<N,D,Table<N,D,shared_ptr<EdgeFunction<V>>>> endsummarytab;

	//edges going along calls
	//see CC 2010 paper by Naeem, Lhotak and Rodriguez
	Table<N,D,map<N,set<D>>> incomingtab;

	//stores the return sites (inside callers) to which we have unbalanced returns
	//if followReturnPastSeeds is enabled
	set<N> unbalancedRetSites;

	map<N,set<D>> initialSeeds;

	Table<N,D,V> valtab;

	/**
	 * Schedules the processing of initial seeds, initiating the analysis.
	 * Clients should only call this methods if performing synchronization on
	 * their own. Normally, {@link #solve()} should be called instead.
	 */
	void submitInitalSeeds()
	{
		cout << "@ submitInitialSeeds()" << endl;
		for (const auto& seed : initialSeeds) {
			N startPoint = seed.first;
			for (const D& value : seed.second) {
				startPoint->dump();
				value->dump();
				propagate(zeroValue, startPoint, value, EdgeIdentity<V>::v(), nullptr, false);
			}
			jumpFn->addFunction(zeroValue, startPoint, zeroValue, EdgeIdentity<V>::v());
		}
	}

	/**
	 * Lines 21-32 of the algorithm.
	 *
	 * Stores callee-side summaries.
	 * Also, at the side of the caller, propagates intra-procedural flows to return sites
	 * using those newly computed summaries.
	 *
	 * @param edge an edge whose target node resembles a method exits
	 */
	void processExit(PathEdge<N,D> edge)
	{
		cout << "@ process exit edge" << endl;

		N n = edge.getTarget(); // an exit node; line 21...
		shared_ptr<EdgeFunction<V>> f = jumpFunction(edge);
		M methodThatNeedsSummary = icfg.getMethodOf(n);

		D d1 = edge.factAtSource();
		D d2 = edge.factAtTarget();

		//for each of the method's start points, determine incoming calls
		set<N> startPointsOf = icfg.getStartPointsOf(methodThatNeedsSummary);
		map<N,set<D>> inc;
		for(N sP : startPointsOf) {
			//line 21.1 of Naeem/Lhotak/Rodriguez
			//register end-summary
			addEndSummary(sP, d1, n, d2, f);
			//copy to avoid concurrent modification exceptions by other threads
			for (auto entry : incoming(d1, sP))
					inc[entry.first] = set<D>{entry.second};
		}

		//for each incoming call edge already processed
		//(see processCall(..))
		for (auto entry : inc) {
			//line 22
			N c = entry.first;
			//for each return site
			for(N retSiteC : icfg.getReturnSitesOfCallAt(c)) {
				//compute return-flow function
				shared_ptr<FlowFunction<D>> retFunction = ideTabluationProblem.getRetFlowFunction(c, methodThatNeedsSummary, n, retSiteC);
				flowFunctionConstructionCount++;
				//for each incoming-call value
				for(D d4 : entry.second) {
					set<D> targets = computeReturnFlowFunction(retFunction, d1, d2, c, entry.second);
					saveEdges(n, retSiteC, d2, targets, true);
					//for each target value at the return site
					//line 23
					for(D d5: targets) {
						//compute composed function
						shared_ptr<EdgeFunction<V>> f4 = ideTabluationProblem.getCallEdgeFunction(c, d4, icfg.getMethodOf(n), d1);
						shared_ptr<EdgeFunction<V>> f5 = ideTabluationProblem.getReturnEdgeFunction(c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
						shared_ptr<EdgeFunction<V>> fPrime = f4->composeWith(f)->composeWith(f5);
						//for each jump function coming into the call, propagate to return site using the composed function
						for (auto valAndFunc : jumpFn->reverseLookup(c, d4)) {
							shared_ptr<EdgeFunction<V>> f3 = valAndFunc.second;
							if(!f3->equalTo(allTop)) {
									D d3 = valAndFunc.first;
									D d5_restoredCtx = restoreContextOnReturnedFact(c, d4, d5);
									propagate(d3, retSiteC, d5_restoredCtx, f3->composeWith(fPrime), c, false);
							}
						}
					}
				}
			}
		}

		//handling for unbalanced problems where we return out of a method with a fact for which we have no incoming flow
		//note: we propagate that way only values that originate from ZERO, as conditionally generated values should only
		//be propagated into callers that have an incoming edge for this condition
		if(followReturnPastSeeds && inc.empty() && d1 == zeroValue) {
		// only propagate up if we
			set<N> callers = icfg.getCallersOf(methodThatNeedsSummary);
				for(N c : callers) {
					for(N retSiteC: icfg.getReturnSitesOfCallAt(c)) {
						shared_ptr<FlowFunction<D>> retFunction = ideTabluationProblem.getRetFlowFunction(c, methodThatNeedsSummary,n,retSiteC);
						flowFunctionConstructionCount++;
						set<D> targets = computeReturnFlowFunction(retFunction, d1, d2, c, set<D>{zeroValue});
						saveEdges(n, retSiteC, d2, targets, true);
						for(D d5 : targets) {
							shared_ptr<EdgeFunction<V>> f5 = ideTabluationProblem.getReturnEdgeFunction(c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
							propagteUnbalancedReturnFlow(retSiteC, d5, f->composeWith(f5), c);
							//register for value processing (2nd IDE phase)
							unbalancedRetSites.insert(retSiteC);
						}
					}
				}
				//in cases where there are no callers, the return statement would normally not be processed at all;
				//this might be undesirable if the flow function has a side effect such as registering a taint;
				//instead we thus call the return flow function will a null caller
				if(callers.empty()) {
					shared_ptr<FlowFunction<D>> retFunction = ideTabluationProblem.getRetFlowFunction(nullptr, methodThatNeedsSummary, n, nullptr);
					flowFunctionConstructionCount++;
					retFunction->computeTargets(d2);
				}
		}
	}

	void propagteUnbalancedReturnFlow(N retSiteC,
									  D targetVal,
									  shared_ptr<EdgeFunction<V>> edgeFunction,
									  N relatedCallSite)
	{
		propagate(zeroValue, retSiteC, targetVal, edgeFunction, relatedCallSite, true);
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
	D restoreContextOnReturnedFact(N callSite, D d4, D d5)
	{
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
	set<D> computeNormalFlowFunction(shared_ptr<FlowFunction<D>> flowFunction, D d1, D d2)
	{
		return flowFunction->computeTargets(d2);
	}

	/**
	 * Computes the call flow function for the given call-site abstraction
	 * @param callFlowFunction The call flow function to compute
	 * @param d1 The abstraction at the current method's start node.
	 * @param d2 The abstraction at the call site
	 * @return The set of caller-side abstractions at the callee's start node
	 */
	set<D> computeCallFlowFunction(shared_ptr<FlowFunction<D>> callFlowFunction, D d1, D d2)
	{
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
	set<D> computeCallToReturnFlowFunction(shared_ptr<FlowFunction<D>> callToReturnFlowFunction, D d1, D d2)
	{
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
	set<D> computeReturnFlowFunction(shared_ptr<FlowFunction<D>> retFunction, D d1, D d2, N callSite, set<D> callerSideDs)
	{
		return retFunction->computeTargets(d2);
	}

	/**
	 * Propagates the flow further down the exploded super graph, merging any edge function that might
	 * already have been computed for targetVal at target.
	 * @param sourceVal the source value of the propagated summary edge
	 * @param target the target statement
	 * @param targetVal the target value at the target statement
	 * @param f the new edge function computed from (s0,sourceVal) to (target,targetVal)
	 * @param relatedCallSite for call and return flows the related call statement, <code>null</code> otherwise
	 *        (this value is not used within this implementation but may be useful for subclasses of {@link IDESolver})
	 * @param isUnbalancedReturn <code>true</code> if this edge is propagating an unbalanced return
	 *        (this value is not used within this implementation but may be useful for subclasses of {@link IDESolver})
	 */
	void propagate(D sourceVal,
				   N target,
				   D targetVal,
				   shared_ptr<EdgeFunction<V>> f,
/* deliberately exposed to clients */ N relatedCallSite,
/* deliberately exposed to clients */ bool isUnbalancedReturn)
	{
		shared_ptr<EdgeFunction<V>> jumpFnE;
		shared_ptr<EdgeFunction<V>> fPrime;
		jumpFnE = jumpFn->reverseLookup(target, targetVal)[sourceVal];
		if (jumpFnE == nullptr)
			jumpFnE = allTop; // jump function is initialized to all-top
		fPrime = jumpFnE->joinWith(f);
		bool newFunction = !(fPrime->equalTo(jumpFnE));
		if (newFunction) {
			jumpFn->addFunction(sourceVal, target, targetVal, fPrime);
			PathEdge<N,D> edge(sourceVal, target, targetVal);
			pathEdgeProcessingTask(edge);
            if(targetVal!=zeroValue) {
            	cout << "EDGE: targetVal != zeroValue" << endl;
//            	cout << icfg.getMethodOf(target)->getName() << endl;
//            	sourceVal->dump();
//            	target->dump();
//            	targetVal->dump();
                //cout << "{} - EDGE: <{},{}> -> <{},{}> - {}" << icfg.getMethodOf(target), sourceVal, target, targetVal, fPrime << endl;
            }
		}
	}

	V joinValueAt(N unit, D fact, V curr, V newVal)
	{
		return ideTabluationProblem.join(curr, newVal);
	}

	set<typename Table<N,D,shared_ptr<EdgeFunction<V>>>::Cell> endSummary(N sP, D d3)
	{
		Table<N, D, shared_ptr<EdgeFunction<V>>>& m = endsummarytab.get(sP, d3);
		if(m.isEmpty()) return set<typename Table<N,D,shared_ptr<EdgeFunction<V>>>::Cell>{};
			return m.cellSet();
	}

	map<N,set<D>> incoming(D d1, N sP)
	{
		map<N, set<D>> m = incomingtab.get(sP, d1);
		if(m.empty()) return map<N,set<D>>{};
		return m;

	}

	void addIncoming(N sP, D d3, N n, D d2)
	{
		map<N, set<D>> summaries = incomingtab.get(sP, d3);
		set<D>& s = summaries[n];
		s.insert(d2);
		incomingtab.insert(sP, d3, summaries);
	}

};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_IDESOLVER_HH_ */
