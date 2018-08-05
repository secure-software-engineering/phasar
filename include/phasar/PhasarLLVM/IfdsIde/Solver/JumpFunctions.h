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

#include <map>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Table.h>

namespace psr {

// Forward declare the IDETabulationProblem as we require its toString
// functionality.
template <typename N, typename D, typename M, typename V, typename I>
class IDETabulationProblem;

template <typename N, typename D, typename M, typename L, typename I>
class JumpFunctions {
private:
  std::shared_ptr<EdgeFunction<L>> allTop;
  const IDETabulationProblem<N, D, M, L, I> &problem;

protected:
  // mapping from target node and value to a list of all source values and
  // associated functions
  // where the list is implemented as a mapping from the source value to the
  // function
  // we exclude empty default functions
  Table<N, D, std::map<D, std::shared_ptr<EdgeFunction<L>>>>
      nonEmptyReverseLookup;
  // mapping from source value and target node to a list of all target values
  // and associated functions
  // where the list is implemented as a mapping from the source value to the
  // function
  // we exclude empty default functions
  Table<D, N, std::map<D, std::shared_ptr<EdgeFunction<L>>>>
      nonEmptyForwardLookup;
  // a mapping from target node to a list of triples consisting of source value,
  // target value and associated function; the triple is implemented by a table
  // we exclude empty default functions
  std::unordered_map<N, Table<D, D, std::shared_ptr<EdgeFunction<L>>>>
      nonEmptyLookupByTargetNode;

public:
  JumpFunctions(std::shared_ptr<EdgeFunction<L>> allTop,
                const IDETabulationProblem<N, D, M, L, I> &p)
      : allTop(allTop), problem(p) {}

  virtual ~JumpFunctions() = default;

  /**
   * Records a jump function. The source statement is implicit.
   * @see PathEdge
   */
  void addFunction(D sourceVal, N target, D targetVal,
                   std::shared_ptr<EdgeFunction<L>> function) {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "Start adding new jump function";
    BOOST_LOG_SEV(lg, DEBUG)
        << "Fact at source : " << problem.DtoString(sourceVal);
    BOOST_LOG_SEV(lg, DEBUG)
        << "Fact at target : " << problem.DtoString(targetVal);
    BOOST_LOG_SEV(lg, DEBUG)
        << "Destination    : " << problem.NtoString(target);
    BOOST_LOG_SEV(lg, DEBUG) << "Edge Function  : " << function->str();
    // we do not store the default function (all-top)
    if (function->equal_to(allTop))
      return;
    std::map<D, std::shared_ptr<EdgeFunction<L>>> &sourceValToFunc =
        nonEmptyReverseLookup.get(target, targetVal);
    sourceValToFunc.insert({sourceVal, function});
    //	printNonEmptyReverseLookup();
    std::map<D, std::shared_ptr<EdgeFunction<L>>> &targetValToFunc =
        nonEmptyForwardLookup.get(sourceVal, target);
    targetValToFunc.insert({targetVal, function});
    //	printNonEmptyForwardLookup();
    nonEmptyLookupByTargetNode[target].insert(sourceVal, targetVal, function);
    //	printNonEmptyLookupByTargetNode();
    BOOST_LOG_SEV(lg, DEBUG) << "End adding new jump function";
    BOOST_LOG_SEV(lg, DEBUG) << ' ';
  }

  /**
   * Returns, for a given target statement and value all associated
   * source values, and for each the associated edge function.
   * The return value is a mapping from source value to function.
   */
  std::map<D, std::shared_ptr<EdgeFunction<L>>> reverseLookup(N target,
                                                              D targetVal) {
    if (!nonEmptyReverseLookup.contains(target, targetVal))
      return std::map<D, std::shared_ptr<EdgeFunction<L>>>{};
    else
      return nonEmptyReverseLookup.get(target, targetVal);
  }

  /**
   * Returns, for a given source value and target statement all
   * associated target values, and for each the associated edge function.
   * The return value is a mapping from target value to function.
   */
  std::map<D, std::shared_ptr<EdgeFunction<L>>> forwardLookup(D sourceVal,
                                                              N target) {
    if (!nonEmptyForwardLookup.contains(sourceVal, target))
      return std::map<D, std::shared_ptr<EdgeFunction<L>>>{};
    else
      return nonEmptyForwardLookup.get(sourceVal, target);
  }

  /**
   * Returns for a given target statement all jump function records with this
   * target.
   * The return value is a set of records of the form
   * (sourceVal,targetVal,edgeFunction).
   */
  Table<D, D, std::shared_ptr<EdgeFunction<L>>> lookupByTarget(N target) {
    // if (nonEmptyLookupByTargetNode.count(target))
    //	return Table<D, D, std::shared_ptr<EdgeFunction<L>>>{};
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
      BOOST_LOG_SEV(lg, DEBUG) << "Node: " << problem.NtoString(entry.first);
      for (auto cell : entry.second.cellSet()) {
        BOOST_LOG_SEV(lg, DEBUG)
            << "fact at src: " << problem.DtoString(cell.r);
        BOOST_LOG_SEV(lg, DEBUG)
            << "fact at dst: " << problem.DtoString(cell.c);
        BOOST_LOG_SEV(lg, DEBUG) << "edge fnct: " << cell.v->str();
      }
    }
  }

  void printNonEmptyReverseLookup() {
    std::cout << "DUMP nonEmptyReverseLookup" << std::endl;
    std::cout << "Table<N, D, std::map<D, std::shared_ptr<EdgeFunction<L>>>>"
              << std::endl;
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
    std::cout << "DUMP nonEmptyForwardLookup" << std::endl;
    std::cout << "Table<D, N, std::map<D, std::shared_ptr<EdgeFunction<L>>>>"
              << std::endl;
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
    std::cout << "DUMP nonEmptyLookupByTargetNode" << std::endl;
    std::cout << "std::unordered_map<N, Table<D, D, "
                 "std::shared_ptr<EdgeFunction<L>>>>"
              << std::endl;
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

} // namespace psr

#endif /* ANALYSIS_IFDS_IDE_SOLVER_JUMPFUNCTIONS_HH_ */
