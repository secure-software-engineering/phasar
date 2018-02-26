/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * JumpFunctions.h
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_JUMPFUNCTIONS_H_
#define ANALYSIS_IFDS_IDE_SOLVER_JUMPFUNCTIONS_H_

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Table.h>
#include <map>
#include <memory>

using namespace std;

template <typename N, typename D, typename L>
class JumpFunctions {
 private:
  shared_ptr<EdgeFunction<L>> allTop;

 protected:
  // mapping from target node and value to a list of all source values and
  // associated functions
  // where the list is implemented as a mapping from the source value to the
  // function
  // we exclude empty default functions
  Table<N, D, map<D, shared_ptr<EdgeFunction<L>>>> nonEmptyReverseLookup;
  // mapping from source value and target node to a list of all target values
  // and associated functions
  // where the list is implemented as a mapping from the source value to the
  // function
  // we exclude empty default functions
  Table<D, N, map<D, shared_ptr<EdgeFunction<L>>>> nonEmptyForwardLookup;
  // a mapping from target node to a list of triples consisting of source value,
  // target value and associated function; the triple is implemented by a table
  // we exclude empty default functions
  unordered_map<N, Table<D, D, shared_ptr<EdgeFunction<L>>>>
      nonEmptyLookupByTargetNode;

 public:
  JumpFunctions(shared_ptr<EdgeFunction<L>> allTop) : allTop(allTop) {}

  virtual ~JumpFunctions() = default;

  /**
   * Records a jump function. The source statement is implicit.
   * @see PathEdge
   */
  void addFunction(D sourceVal, N target, D targetVal,
                   shared_ptr<EdgeFunction<L>> function) {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "Start adding new jump function";
    BOOST_LOG_SEV(lg, DEBUG)
        << "Fact at source: "
        << ((sourceVal) ? llvmIRToString(sourceVal) : "nullptr");
    BOOST_LOG_SEV(lg, DEBUG)
        << "Fact at target: "
        << ((targetVal) ? llvmIRToString(targetVal) : "nullptr");
    BOOST_LOG_SEV(lg, DEBUG) << "Destination: " << llvmIRToString(target);
    BOOST_LOG_SEV(lg, DEBUG) << "EdgeFunction: " << function->toString();
    // we do not store the default function (all-top)
    if (function->equalTo(allTop)) return;
    map<D, shared_ptr<EdgeFunction<L>>> &sourceValToFunc =
        nonEmptyReverseLookup.get(target, targetVal);
    sourceValToFunc.insert({sourceVal, function});
    //	printNonEmptyReverseLookup();
    map<D, shared_ptr<EdgeFunction<L>>> &targetValToFunc =
        nonEmptyForwardLookup.get(sourceVal, target);
    targetValToFunc.insert({targetVal, function});
    //	printNonEmptyForwardLookup();
    nonEmptyLookupByTargetNode[target].insert(sourceVal, targetVal, function);
    //	printNonEmptyLookupByTargetNode();
    BOOST_LOG_SEV(lg, DEBUG) << "End adding new jump function";
  }

  /**
   * Returns, for a given target statement and value all associated
   * source values, and for each the associated edge function.
   * The return value is a mapping from source value to function.
   */
  map<D, shared_ptr<EdgeFunction<L>>> reverseLookup(N target, D targetVal) {
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
  map<D, shared_ptr<EdgeFunction<L>>> forwardLookup(D sourceVal, N target) {
    if (!nonEmptyForwardLookup.contains(sourceVal, target))
      return map<D, shared_ptr<EdgeFunction<L>>>{};
    else
      return nonEmptyForwardLookup.get(sourceVal, target);
  }

  /**
   * Returns for a given target statement all jump function records with this
   * target.
   * The return value is a set of records of the form
   * (sourceVal,targetVal,edgeFunction).
   */
  Table<D, D, shared_ptr<EdgeFunction<L>>> lookupByTarget(N target) {
    // if (nonEmptyLookupByTargetNode.count(target))
    //	return Table<D, D, shared_ptr<EdgeFunction<L>>>{};
    // else
    return nonEmptyLookupByTargetNode[target];
  }

  /**
   * Removes a jump function. The source statement is implicit.
   * @see PathEdge
   * @return True if the function has actually been removed. False if it was not
   * there anyway.
   */
  bool removeFunction(D sourceVal, N target, D targetVal) {
    //			Map<D,EdgeFunction<L>> sourceValToFunc =
    // nonEmptyReverseLookup.get(target, targetVal);
    //			if (sourceValToFunc == null)
    //				return false;
    //			if (sourceValToFunc.remove(sourceVal) == null)
    //				return false;
    //			if (sourceValToFunc.empty())
    //				nonEmptyReverseLookup.remove(targetVal,
    // targetVal);
    //
    //			Map<D, EdgeFunction<L>> targetValToFunc =
    // nonEmptyForwardLookup.get(sourceVal, target);
    //			if (targetValToFunc == null)
    //				return false;
    //			if (targetValToFunc.remove(targetVal) == null)
    //				return false;
    //			if (targetValToFunc.empty())
    //				nonEmptyForwardLookup.remove(sourceVal, target);
    //
    //			Table<D,D,EdgeFunction<L>> table =
    // nonEmptyLookupByTargetNode.get(target);
    //			if (table == null)
    //				return false;
    //			if (table.remove(sourceVal, targetVal) == null)
    //				return false;
    //			if (table.empty())
    //				nonEmptyLookupByTargetNode.remove(target);
    //
    //			return true;
    return false;
  }

  /**
   * Removes all jump functions
   */
  void clear() {
    nonEmptyReverseLookup.clear();
    nonEmptyForwardLookup.clear();
    nonEmptyLookupByTargetNode.clear();
  }

  void printJumpFunctions() {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "Jump Functions:";
    for (auto &entry : nonEmptyLookupByTargetNode) {
      BOOST_LOG_SEV(lg, DEBUG) << "Node: " << llvmIRToString(entry.first);
      for (auto cell : entry.second.cellSet()) {
        BOOST_LOG_SEV(lg, DEBUG) << "fact at src: " << llvmIRToString(cell.r);
        BOOST_LOG_SEV(lg, DEBUG) << "fact at dst: " << llvmIRToString(cell.c);
        BOOST_LOG_SEV(lg, DEBUG) << "edge fnct: " << cell.v->toString();
      }
    }
  }

  void printNonEmptyReverseLookup() {
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

  void printNonEmptyForwardLookup() {
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

  void printNonEmptyLookupByTargetNode() {
    cout << "DUMP nonEmptyLookupByTargetNode" << endl;
    cout << "unordered_map<N, Table<D, D, shared_ptr<EdgeFunction<L>>>>"
         << endl;
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
