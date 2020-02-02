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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JUMPFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JUMPFUNCTIONS_H_

#include <memory>
#include <ostream>
#include <unordered_map>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Table.h>

namespace psr {

// Forward declare the IDETabulationProblem as we require its toString
// functionality.
template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class IDETabulationProblem;

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class JumpFunctions {
private:
  std::shared_ptr<EdgeFunction<L>> allTop;
  const IDETabulationProblem<N, D, F, T, V, L, I> &problem;

protected:
  // mapping from target node and value to a list of all source values and
  // associated functions where the list is implemented as a mapping from
  // the source value to the function we exclude empty default functions
  Table<N, D, std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>>>
      nonEmptyReverseLookup;
  // mapping from source value and target node to a list of all target values
  // and associated functions where the list is implemented as a mapping from
  // the source value to the function we exclude empty default functions
  Table<D, N, std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>>>
      nonEmptyForwardLookup;
  // a mapping from target node to a list of triples consisting of source value,
  // target value and associated function; the triple is implemented by a table
  // we exclude empty default functions
  std::unordered_map<N, Table<D, D, std::shared_ptr<EdgeFunction<L>>>>
      nonEmptyLookupByTargetNode;

public:
  JumpFunctions(std::shared_ptr<EdgeFunction<L>> allTop,
                const IDETabulationProblem<N, D, F, T, V, L, I> &p)
      : allTop(allTop), problem(p) {}

  ~JumpFunctions() = default;

  JumpFunctions(const JumpFunctions &JFs) = default;

  JumpFunctions(JumpFunctions &&JFs) = default;

  /**
   * Records a jump function. The source statement is implicit.
   * @see PathEdge
   */
  void addFunction(D sourceVal, N target, D targetVal,
                   std::shared_ptr<EdgeFunction<L>> function) {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Start adding new jump function");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Fact at source : " << problem.DtoString(sourceVal));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Fact at target : " << problem.DtoString(targetVal));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Destination    : " << problem.NtoString(target));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Edge Function  : " << function->str());
    // we do not store the default function (all-top)
    if (function->equal_to(allTop)) {
      return;
    }
    // it is important that existing values in JumpFunctions are overwritten
    // (use operator[] instead of insert)
    std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>> &sourceValToFunc =
        nonEmptyReverseLookup.get(target, targetVal);
    sourceValToFunc[sourceVal] = function;
    std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>> &targetValToFunc =
        nonEmptyForwardLookup.get(sourceVal, target);
    targetValToFunc[targetVal] = function;
    // V Table::insert(R r, C c, V v) always overrides (see comments above)
    nonEmptyLookupByTargetNode[target].insert(sourceVal, targetVal, function);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "End adding new jump function");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  }

  /**
   * Returns, for a given target statement and value all associated
   * source values, and for each the associated edge function.
   * The return value is a mapping from source value to function.
   */
  std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>>
  reverseLookup(N target, D targetVal) {
    if (!nonEmptyReverseLookup.contains(target, targetVal))
      return std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>>{};
    else
      return nonEmptyReverseLookup.get(target, targetVal);
  }

  /**
   * Returns, for a given source value and target statement all
   * associated target values, and for each the associated edge function.
   * The return value is a mapping from target value to function.
   */
  std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>>
  forwardLookup(D sourceVal, N target) {
    if (!nonEmptyForwardLookup.contains(sourceVal, target))
      return std::unordered_map<D, std::shared_ptr<EdgeFunction<L>>>{};
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
    return nonEmptyLookupByTargetNode[target];
  }

  /**
   * Removes a jump function. The source statement is implicit.
   * @see PathEdge
   * @return True if the function has actually been removed. False if it was not
   * there anyway.
   */
  bool removeFunction(D sourceVal, N target, D targetVal) {
    nonEmptyReverseLookup.get(target, targetVal).erase(sourceVal);
    nonEmptyForwardLookup.get(sourceVal, target).erase(targetVal);
    return nonEmptyLookupByTargetNode.erase(target);
  }

  /**
   * Removes all jump functions
   */
  void clear() {
    nonEmptyReverseLookup.clear();
    nonEmptyForwardLookup.clear();
    nonEmptyLookupByTargetNode.clear();
  }

  void printJumpFunctions(std::ostream &os) {
    os << "\n******************************************************";
    os << "\n*              Print all Jump Functions              *";
    os << "\n******************************************************\n";
    for (auto &entry : nonEmptyLookupByTargetNode) {
      std::string nLabel = problem.NtoString(entry.first);
      os << "\nN: " << nLabel << "\n---" << std::string(nLabel.size(), '-')
         << '\n';
      for (auto cell : entry.second.cellSet()) {
        os << "D1: " << problem.DtoString(cell.r) << '\n'
           << "\tD2: " << problem.DtoString(cell.c) << '\n'
           << "\tEF: " << cell.v->str() << "\n\n";
      }
    }
  }

  void printNonEmptyReverseLookup(std::ostream &os) {
    os << "DUMP nonEmptyReverseLookup\nTable<N, D, std::unordered_map<D, "
          "std::shared_ptr<EdgeFunction<L>>>>\n";
    auto cellvec = nonEmptyReverseLookup.cellVec();
    for (auto cell : cellvec) {
      os << "N : " << problem.NtoString(cell.r)
         << "\nD1: " << problem.DtoString(cell.c) << '\n';
      for (auto D2ToEF : cell.v) {
        os << "D2: " << problem.DtoString(D2ToEF.first)
           << "\nEF: " << D2ToEF.second->str() << '\n';
      }
      os << '\n';
    }
  }

  void printNonEmptyForwardLookup(std::ostream &os) {
    os << "DUMP nonEmptyForwardLookup\nTable<D, N, std::unordered_map<D, "
          "std::shared_ptr<EdgeFunction<L>>>>\n";
    auto cellvec = nonEmptyForwardLookup.cellVec();
    for (auto cell : cellvec) {
      os << "D1: " << problem.DtoString(cell.r)
         << "\nN : " << problem.NtoString(cell.c) << '\n';
      for (auto D2ToEF : cell.v) {
        os << "D2: " << problem.DtoString(D2ToEF.first)
           << "\nEF: " << D2ToEF.second->str() << '\n';
      }
      os << '\n';
    }
  }

  void printNonEmptyLookupByTargetNode(std::ostream &os) {
    os << "DUMP nonEmptyLookupByTargetNode\nstd::unordered_map<N, Table<D, D, "
          "std::shared_ptr<EdgeFunction<L>>>>\n";
    for (auto node : nonEmptyLookupByTargetNode) {
      os << "\nN : " << problem.NtoString(node.first) << '\n';
      auto table = nonEmptyLookupByTargetNode[node.first];
      auto cellvec = table.cellVec();
      for (auto cell : cellvec) {
        os << "D1: " << problem.DtoString(cell.r)
           << "\nD2: " << problem.DtoString(cell.c) << "\nEF: " << cell.v->str()
           << '\n';
      }
      os << '\n';
    }
  }
};

} // namespace psr

#endif
