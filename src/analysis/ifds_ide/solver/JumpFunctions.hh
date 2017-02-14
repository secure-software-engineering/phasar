/*
 * JumpFunctions.hh
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_JUMPFUNCTIONS_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_JUMPFUNCTIONS_HH_

#include <map>
#include <memory>

#include "../../../utils/Table.hh"
#include "../EdgeFunction.hh"

using namespace std;

template<class N, class D, class L>
class JumpFunctions {
private:
	shared_ptr<EdgeFunction<L>> allTop;

protected:
	//mapping from target node and value to a list of all source values and associated functions
	//where the list is implemented as a mapping from the source value to the function
	//we exclude empty default functions
	Table<N, D, map<D, shared_ptr<EdgeFunction<L>>>> nonEmptyReverseLookup;
	//mapping from source value and target node to a list of all target values and associated functions
	//where the list is implemented as a mapping from the source value to the function
	//we exclude empty default functions
	Table<D, N, map<D, shared_ptr<EdgeFunction<L>>>> nonEmptyForwardLookup;
	//a mapping from target node to a list of triples consisting of source value,
	//target value and associated function; the triple is implemented by a table
	//we exclude empty default functions
	unordered_map<N, Table<D, D, shared_ptr<EdgeFunction<L>>>> nonEmptyLookupByTargetNode;

public:
	JumpFunctions(shared_ptr<EdgeFunction<L>> allTop) : allTop(allTop) { }

	virtual ~JumpFunctions() = default;

	/**
	  * Records a jump function. The source statement is implicit.
	  * @see PathEdge
	  */
	void addFunction(D sourceVal, N target, D targetVal, shared_ptr<EdgeFunction<L>> function)
	{
		cout << "ADDING NEW JUMP FUNCTION" << endl;
		cout << "Fact at source:" << endl;
		if (sourceVal) sourceVal->dump();
		cout << "Fact at target:" << endl;
		if (targetVal) targetVal->dump();
		cout << "Destination:" << endl;
		target->dump();
		cout << "EdgeFunction:" << endl;
		function->dump();
		// we do not store the default function (all-top)
		if (function->equalTo(allTop)) return;
		map<D,shared_ptr<EdgeFunction<L>>>& sourceValToFunc = nonEmptyReverseLookup.get(target, targetVal);
		sourceValToFunc.insert( {sourceVal, function} );
		printNonEmptyReverseLookup();
		map<D,shared_ptr<EdgeFunction<L>>>& targetValToFunc = nonEmptyForwardLookup.get(sourceVal, target);
		targetValToFunc.insert( {targetVal, function} );
		printNonEmptyForwardLookup();
		nonEmptyLookupByTargetNode[target].insert(sourceVal, targetVal, function);
		printNonEmptyLookupByTargetNode();
		cout << "DONE ADDING NEW JUMP FUNCTION" << endl;
	}

	/**
     * Returns, for a given target statement and value all associated
     * source values, and for each the associated edge function.
     * The return value is a mapping from source value to function.
	 */
	map<D, shared_ptr<EdgeFunction<L>>> reverseLookup(N target, D targetVal)
	{
		if (!nonEmptyReverseLookup.contains(target, targetVal))
			return map<D, shared_ptr<EdgeFunction<L>>>{};
		else
			return nonEmptyReverseLookup.get(target, targetVal);
	}

	/**
	 * Returns, for a given source value and target statement all
	 * associated target values, and for each the associated edge function.
     * The return value is a mapping from target value to function.
	 */
	map<D, shared_ptr<EdgeFunction<L>>> forwardLookup(D sourceVal, N target)
	{
		if (!nonEmptyForwardLookup.contains(sourceVal, target))
			return map<D, shared_ptr<EdgeFunction<L>>>{};
		else
			return nonEmptyForwardLookup.get(sourceVal, target);
	}

	/**
	 * Returns for a given target statement all jump function records with this target.
	 * The return value is a set of records of the form (sourceVal,targetVal,edgeFunction).
	 */
	Table<D, D, shared_ptr<EdgeFunction<L>>> lookupByTarget(N target)
	{
		//if (nonEmptyLookupByTargetNode.count(target))
		//	return Table<D, D, shared_ptr<EdgeFunction<L>>>{};
		//else
			return nonEmptyLookupByTargetNode[target];
	}

	/**
	 * Removes a jump function. The source statement is implicit.
	 * @see PathEdge
	 * @return True if the function has actually been removed. False if it was not
	 * there anyway.
	 */
	bool removeFunction(D sourceVal, N target, D targetVal)
	{

//			Map<D,EdgeFunction<L>> sourceValToFunc = nonEmptyReverseLookup.get(target, targetVal);
//			if (sourceValToFunc == null)
//				return false;
//			if (sourceValToFunc.remove(sourceVal) == null)
//				return false;
//			if (sourceValToFunc.isEmpty())
//				nonEmptyReverseLookup.remove(targetVal, targetVal);
//
//			Map<D, EdgeFunction<L>> targetValToFunc = nonEmptyForwardLookup.get(sourceVal, target);
//			if (targetValToFunc == null)
//				return false;
//			if (targetValToFunc.remove(targetVal) == null)
//				return false;
//			if (targetValToFunc.isEmpty())
//				nonEmptyForwardLookup.remove(sourceVal, target);
//
//			Table<D,D,EdgeFunction<L>> table = nonEmptyLookupByTargetNode.get(target);
//			if (table == null)
//				return false;
//			if (table.remove(sourceVal, targetVal) == null)
//				return false;
//			if (table.isEmpty())
//				nonEmptyLookupByTargetNode.remove(target);
//
//			return true;
	}

	/**
	 * Removes all jump functions
	 */
	void clear()
	{
		nonEmptyReverseLookup.clear();
		nonEmptyForwardLookup.clear();
		nonEmptyLookupByTargetNode.clear();
	}

	void printNonEmptyReverseLookup()
	{
		cout << "DUMP nonEmptyReverseLookup" << endl;
		cout << "Table<N, D, map<D, shared_ptr<EdgeFunction<L>>>>" << endl;
 		auto cellset = nonEmptyReverseLookup.cellSet();
		for (auto cell : cellset) {
			cell.r->dump();
			cell.c->dump();
			for (auto edgefunction : cell.v) {
				edgefunction.first->dump();
				edgefunction.second->dump();
			}
		}
	}

	void printNonEmptyForwardLookup()
	{
		cout << "DUMP nonEmptyForwardLookup" << endl;
		cout << "Table<D, N, map<D, shared_ptr<EdgeFunction<L>>>>" << endl;
		auto cellset = nonEmptyForwardLookup.cellSet();
		for (auto cell : cellset) {
			cell.r->dump();
			cell.c->dump();
			for (auto edgefunction : cell.v) {
				edgefunction.first->dump();
				edgefunction.second->dump();
			}
		}
	}

	void printNonEmptyLookupByTargetNode()
	{
		cout << "DUMP nonEmptyLookupByTargetNode" << endl;
		cout << "unordered_map<N, Table<D, D, shared_ptr<EdgeFunction<L>>>>" << endl;
		for (auto node : nonEmptyLookupByTargetNode) {
				node.first->dump();
			auto table = nonEmptyLookupByTargetNode[node.first];
			auto cellset = table.cellSet();
			for (auto cell : cellset) {
				cell.r->dump();
				cell.c->dump();
				cell.v->dump();
			}
		}
	}

};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_JUMPFUNCTIONS_HH_ */
